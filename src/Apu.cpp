
#include "Apu.h"
#include "Cpu.h"
#include "Dmc.h"
#include "Noise.h"
#include "Square.h"
#include "Triangle.h"

#include "Nes.h"

#include "AudioResampler.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// note: DMC and DPCM may be used interchangeably

namespace nes::apu {

bool debug_audio_muted = false;	

namespace {

// Bit masks used by the APU status/control register.
typedef enum {
	ENABLE_SQUARE1 = 	0x1,
	ENABLE_SQUARE2 = 	0x2,
	ENABLE_TRIANGLE = 	0x4,
	ENABLE_NOISE = 		0x8,
	ENABLE_DMC = 		0x10,
	FRAME_IRQ = 		0x40,
	DMC_IRQ = 			0x80
} apu_status;


// Represents the APU frame-counter control register ($4017).
union apu_frame_counter_t {
	uint8_t raw;

	// Bit 6: suppresses frame IRQ generation when set.
	BitField<uint8_t, 6> inhibit_frame_irq;

	// Bit 7: selects 5-step mode when set, 4-step mode when clear.
	BitField<uint8_t, 7> mode;
};

constexpr double kCPUFrequency = 1789772.7272; // 1.7897727272MHz


// -----------------------------------------------------------------------------
// Triangle debugger mute smoothing.
//
// The gain applies only to the Triangle contribution entering the native
// TND mixer. It does not alter the emulated Triangle DAC or waveform state.
//
// Five milliseconds provides a short, click-reducing transition.
// -----------------------------------------------------------------------------
constexpr double kTriangleMuteFadeSeconds = 0.020;
constexpr double kTriangleMuteGainStep = 1.0 / (kCPUFrequency * kTriangleMuteFadeSeconds);
double sTriangleMuteGain = 1.0;

// DAC level held while the Triangle debugger mute is active.
double sTriangleMuteHeldDAC = 15.0;

// Previous debugger mute state, used to detect mute transitions.
bool sTriangleWasMuted = false;


// -----------------------------------------------------------------------------
// DMC debugger mute smoothing.
//
// Muting holds the DMC's current DAC contribution rather than forcing it
// to zero. This prevents an artificial DC transition when a sample is not
// playing.
//
// The actual DMC hardware state continues updating independently.
// -----------------------------------------------------------------------------

constexpr double kDmcMuteFadeSeconds = 0.020;
constexpr double kDmcMuteGainStep = 1.0 / (kCPUFrequency * kDmcMuteFadeSeconds);
double sDmcMuteGain = 1.0;
double sDmcMuteHeldDAC = 0.0;
bool sDmcWasMuted = false;


// -----------------------------------------------------------------------------
// Shared band-limited audio resampler.
//
// Receives the combined native APU and expansion-audio output once per
// emulated CPU cycle.
//
// Generates filtered host audio at kOutputFrequency without changing the
// timing or state of the underlying audio channels.
// -----------------------------------------------------------------------------
AudioResampler audio_resampler(kCPUFrequency, static_cast<double>(kOutputFrequency), 20000.0);


// Global APU frame-sequencer timing state.
auto apu_cycles_ = static_cast<uint64_t>(-1);

// APU cycle on which the next frame-sequencer event will occur.
auto next_clock_ = static_cast<uint64_t>(-1);

// Next internal frame-sequencer step to execute.
uint8_t clock_step_ = 0;

// Current $4017 frame-counter control state.
apu_frame_counter_t frame_counter_ = {0};

// Most recently written $4017 value, preserved across soft reset.
uint8_t last_frame_counter_ = 0;


// Raw APU register values retained for the APU Explorer debugger view.
uint8_t explorer_square1_[4] = {};
uint8_t explorer_square2_[4] = {};

uint8_t explorer_triangle0_ = 0;
uint8_t explorer_triangle2_ = 0;
uint8_t explorer_triangle3_ = 0;

uint8_t explorer_noise0_ = 0;
uint8_t explorer_noise2_ = 0;
uint8_t explorer_noise3_ = 0;

uint8_t explorer_dmc_[4] = {};

// Most recently written $4015 control value.
uint8_t explorer_status_ = 0;


// Previous input/output samples retained by the one-pole DC-blocking filter.
double sDCBlockPreviousInput = 0.0;
double sDCBlockPreviousOutput = 0.0;

}


// Active APU channel objects and global $4015-style status state.
Square<0> square_0;
Square<1> square_1;
Triangle triangle;
Noise noise;
DMC dmc;
apu_status_t status = {0};


// Circular host-audio sample buffer and its read/write positions.
uint8_t sample_buffer_[kBufferSize];
size_t sample_buffer_start = 0;
size_t sample_buffer_end   = 0;

// Rolling APU register-write log and ring-buffer bookkeeping.
static nes::apu::write_log_entry_t write_log_[nes::apu::APU_WRITE_LOG_CAPACITY];
static uint32_t write_log_next_ = 0;
static uint32_t write_log_count_ = 0;

// Monotonically increasing sequence number assigned to each logged write.
static uint32_t write_log_write_index_ = 0;


// Rolling APU oscilloscope sample buffer and ring-buffer bookkeeping.
static scope_sample_t scope_samples_[APU_SCOPE_SAMPLE_CAPACITY];
static uint32_t scope_sample_next_ = 0;
static uint32_t scope_sample_count_ = 0;


// Rolling APU frame-sequencer event history and ring-buffer bookkeeping.
static frame_event_t frame_events_[APU_FRAME_EVENT_CAPACITY];
static uint32_t frame_event_next_ = 0;
static uint32_t frame_event_count_ = 0;

// Records one frame-sequencer event in the rolling debugger history.
static void log_frame_event(apu_frame_event_type type);


// -----------------------------------------------------------------------------
// log_write
//
// Appends one native or expansion-audio register write to the rolling write
// log. Native APU writes reconstruct timer state locally. Expansion-audio
// writes receive any mapper-owned timer and frequency-control state from the
// mapper at the moment the write occurs.
// -----------------------------------------------------------------------------
void
log_write (uint16_t address, uint8_t value, write_log_source_t source, uint16_t externalTimerPeriod, 
		   bool hasExternalTimerPeriod, uint8_t externalFrequencyControl, bool hasExternalFrequencyControl)
{
	write_log_entry_t &entry = write_log_[write_log_next_];

	entry.cycle = apu_cycles_;
	entry.address = address;
	entry.value = value;
	entry.write_index = write_log_write_index_++;
	entry.source = source;

	entry.timer_period = 0;
	entry.has_timer_period = false;

	entry.previous_enable_mask = 0;
	entry.has_previous_enable_mask = false;

	entry.external_frequency_control = 0;
	entry.has_external_frequency_control = false;

	if (source == write_log_source_t::NATIVE_APU) {
		switch (address) {
			case 0x4002:
				entry.timer_period = (square_0.debug_timer_period() & 0x0700) | 
									 static_cast<uint16_t>(value);
				entry.has_timer_period = true;
				break;

			case 0x4003:
				entry.timer_period = (square_0.debug_timer_period() & 0x00ff) | 
									 (static_cast<uint16_t>(value & 0x07) << 8);
				entry.has_timer_period = true;
				break;

			case 0x4006:
				entry.timer_period = (square_1.debug_timer_period() & 0x0700) |
									 static_cast<uint16_t>(value);
				entry.has_timer_period = true;
				break;

			case 0x4007:
				entry.timer_period = (square_1.debug_timer_period() & 0x00ff) |
								 	 (static_cast<uint16_t>(value & 0x07) << 8);
				entry.has_timer_period = true;
				break;

			case 0x400a:
				entry.timer_period = (triangle.debug_timer_period() & 0x0700) |
									 static_cast<uint16_t>(value);
				entry.has_timer_period = true;
				break;

			case 0x400b:
				entry.timer_period = (triangle.debug_timer_period() & 0x00ff) |
								 	 (static_cast<uint16_t>(value & 0x07) << 8);
				entry.has_timer_period = true;
				break;

			case 0x4015:
			{
				uint8_t previousMask = 0x00;

				if (square_0.enabled()) {
					previousMask |= 0x01;
				}

				if (square_1.enabled()) {
					previousMask |= 0x02;
				}

				if (triangle.enabled()) {
					previousMask |= 0x04;
				}

				if (noise.enabled()) {
					previousMask |= 0x08;
				}

				if (dmc.debug_enabled()) {
					previousMask |= 0x10;
				}

				entry.previous_enable_mask = previousMask;
				entry.has_previous_enable_mask = true;
			}
			break;

			default:
				break;
		}
	} else {
		entry.timer_period = externalTimerPeriod;
		entry.has_timer_period = hasExternalTimerPeriod;

		entry.external_frequency_control = externalFrequencyControl;
		entry.has_external_frequency_control = hasExternalFrequencyControl;
	}

	write_log_next_ = (write_log_next_ + 1) % APU_WRITE_LOG_CAPACITY;

	if (write_log_count_ < APU_WRITE_LOG_CAPACITY) {
		write_log_count_++;
	}
}


// -----------------------------------------------------------------------------
// log_write
//
// Native-APU convenience wrapper used by the existing $4000-$4017 register
// write handlers.
// -----------------------------------------------------------------------------
void
log_write (uint16_t address, uint8_t value)
{
	log_write(address, value, write_log_source_t::NATIVE_APU, 0, false, 0, false);
}


// -----------------------------------------------------------------------------
// nes::apu::log_external_audio_write
//
// Adds one mapper expansion-audio write to the same chronological history used
// by native APU writes. Mapper-owned state is supplied by the mapper so the log
// preserves the state that existed at the exact time of the write.
// -----------------------------------------------------------------------------
void
log_external_audio_write (uint16_t address, uint8_t value, uint16_t timerPeriod, bool hasTimerPeriod,
					  	  uint8_t frequencyControl)
{
	log_write(address, value, write_log_source_t::EXTERNAL_AUDIO, timerPeriod, hasTimerPeriod,
			  frequencyControl, true);
}


// -----------------------------------------------------------------------------
// capture_scope_sample
//
// Captures one debugger oscilloscope sample containing the instantaneous
// output levels of all five native NES APU channels, the individual
// mapper-provided expansion-audio channels, the mapper's combined
// expansion-audio output, and the final mixed host PCM sample.
//
// Native and expansion channel levels are captured directly from their
// current emulated state, before FIR resampling and DC blocking.
//
// The mixed PCM sample is supplied by the normal audio-generation path
// after FIR low-pass filtering, resampling, DC blocking, and conversion
// to unsigned eight-bit PCM.
//
// This function does not independently generate or filter audio. In
// particular, it does not call mix_channels(), ensuring that the
// stateful DC-blocking filter is advanced only once per host sample.
//
// The captured data is used exclusively for debugger visualization
// and does not modify APU channel timing or audio output.
//
// Parameters:
//   mixedSample - Final unsigned eight-bit PCM sample generated by
//                 the normal APU audio pipeline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
capture_scope_sample (uint8_t mixedSample)
{
	scope_sample_t &sample = scope_samples_[scope_sample_next_];

	sample.square1 = square_0.debug_output();
	sample.square2 = square_1.debug_output();
	sample.triangle = triangle.output();
	sample.noise = noise.debug_output();
	sample.dmc = dmc.output();


	/*
	 * Clear mapper-provided channel data before capturing the current sample.
	 *
	 * The scope history is a ring buffer, so this slot may contain expansion
	 * channels from an earlier sample or even from a previously loaded mapper.
	 */
	for (size_t index = 0; index < APU_SCOPE_EXPANSION_CHANNEL_CAPACITY; ++index) {
		sample.expansion[index] = 0;
	}

	sample.expansion_channel_count = 0;
	sample.expansion_total = 0;

	if (nes::cart.mapper() != nullptr) {
		const mapper_audio_scope_sample_t expansionSample = nes::cart.mapper()->debug_audio_scope_sample();
		size_t channelCount = expansionSample.channel_count;

		if (channelCount > APU_SCOPE_EXPANSION_CHANNEL_CAPACITY) {
			channelCount = APU_SCOPE_EXPANSION_CHANNEL_CAPACITY;
		}

		for (size_t index = 0; index < channelCount; ++index) {
			sample.expansion[index] = expansionSample.channels[index];
		}

		sample.expansion_channel_count = channelCount;

		/*
		 * Capture the mapper's current combined audible expansion-audio
		 * output at the same sampling instant as the individual channels.
		 */
		sample.expansion_total = nes::cart.mapper()->audio_output();
	}


	sample.mixed = static_cast<float>(mixedSample);

	scope_sample_next_ = (scope_sample_next_ + 1) % APU_SCOPE_SAMPLE_CAPACITY;

	if (scope_sample_count_ < APU_SCOPE_SAMPLE_CAPACITY) {
		scope_sample_count_++;
	}
}


// -----------------------------------------------------------------------------
// nes::apu::debug_scope_snapshot
//
// Copies the current APU oscilloscope sample history into caller-provided
// storage.
//
// Samples are returned in chronological order from oldest to newest.  If the
// caller supplies less storage than the number of available samples, the most
// recent samples are returned.
//
// Parameters:
//   samples  - Destination sample array.
//   capacity - Maximum number of samples that may be copied.
//
// Returns:
//   Number of samples copied.
// -----------------------------------------------------------------------------
uint32_t
debug_scope_snapshot (scope_sample_t *samples, uint32_t capacity)
{
	if (!samples || capacity == 0) {
		return 0;
	}

	const uint32_t available = scope_sample_count_;
	const uint32_t count = (available < capacity) ? available : capacity;

	if (count == 0) {
		return 0;
	}

	uint32_t start = 0;

	if (available == APU_SCOPE_SAMPLE_CAPACITY) {
		start = scope_sample_next_;
	}

	if (count < available) {
		start = (start + (available - count)) % APU_SCOPE_SAMPLE_CAPACITY;
	}

	for (uint32_t index = 0; index < count; index++) {
		const uint32_t physicalIndex = (start + index) % APU_SCOPE_SAMPLE_CAPACITY;
		samples[index] = scope_samples_[physicalIndex];
	}

	return count;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_clear_scope
//
// Clears the debugger oscilloscope sample history.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_scope()
{
	scope_sample_next_ = 0;
	scope_sample_count_ = 0;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_frame_event_snapshot
//
// Copies the rolling APU frame-sequencer event history into caller-provided
// storage in chronological order.
//
// Parameters:
//   events   - Destination event array.
//   capacity - Maximum number of events to copy.
//
// Returns:
//   Number of events copied.
// -----------------------------------------------------------------------------
uint32_t
debug_frame_event_snapshot (frame_event_t *events, uint32_t capacity)
{
	if (!events || capacity == 0) {
		return 0;
	}

	const uint32_t available = frame_event_count_;
	const uint32_t count = (available < capacity) ? available : capacity;

	if (count == 0) {
		return 0;
	}

	uint32_t start = 0;

	if (available == APU_FRAME_EVENT_CAPACITY) {
		start = frame_event_next_;
	}

	if (count < available) {
		start = (start + (available - count)) % APU_FRAME_EVENT_CAPACITY;
	}

	for (uint32_t index = 0; index < count; index++) {
		const uint32_t physicalIndex = (start + index) % APU_FRAME_EVENT_CAPACITY;
		events[index] = frame_events_[physicalIndex];
	}

	return count;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_clear_frame_events
//
// Clears the debugger frame-sequencer event history.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_frame_events()
{
	frame_event_next_ = 0;
	frame_event_count_ = 0;
}


// -----------------------------------------------------------------------------
// clock_linear
//
// Clocks the APU components that advance on each quarter-frame event.
//
// This updates the triangle linear counter and the envelope units for both
// square channels and the noise channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
clock_linear()
{
	triangle.linear_counter.clock();

	square_0.envelope.clock();
	square_1.envelope.clock();
	noise.envelope.clock();
}


// -----------------------------------------------------------------------------
// clock_length
//
// Clocks the APU components that advance on each half-frame event.
//
// This updates the length counters for the square, triangle, and noise channels,
// and advances the sweep units for both square channels.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_length()
{
	square_0.length_counter.clock();
	square_1.length_counter.clock();
	triangle.length_counter.clock();
	noise.length_counter.clock();

	square_0.sweep.clock();
	square_1.sweep.clock();
}


// -----------------------------------------------------------------------------
// clock_frame_mode_0
//
// Advances the APU frame counter through its 4-step sequence.
//
// Quarter-frame events clock envelopes and the triangle linear counter.
// Half-frame events additionally clock length counters and square-channel
// sweeps. Frame IRQ state is asserted at the appropriate sequence points when
// frame IRQ generation is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_frame_mode_0()
{
	// 4 step sequence
	switch (clock_step_) {
		case 0:
			log_frame_event(APU_FRAME_EVENT_QUARTER);

			clock_linear();
			next_clock_ += 7456;
			break;

		case 1:
			log_frame_event(APU_FRAME_EVENT_QUARTER_HALF);

			clock_linear();
			clock_length();
			next_clock_ += 7458;
			break;

		case 2:
			log_frame_event(APU_FRAME_EVENT_QUARTER);

			clock_linear();
			next_clock_ += 7457;
			break;

		case 3:
			log_frame_event(APU_FRAME_EVENT_IRQ);

			if (!(frame_counter_.inhibit_frame_irq)) {
				status.frame_irq = true;
			}

			++next_clock_;
			break;

		case 4:
			log_frame_event(APU_FRAME_EVENT_QUARTER_HALF_IRQ);

			clock_linear();
			clock_length();

			if (!(frame_counter_.inhibit_frame_irq)) {
				status.frame_irq = true;
			}

			++next_clock_;
			break;

		case 5:
			log_frame_event(APU_FRAME_EVENT_IRQ);

			if (!(frame_counter_.inhibit_frame_irq)) {
				status.frame_irq = true;
			}

			next_clock_ += 7457;
			break;
	}

	clock_step_ = (clock_step_ + 1) % 6;
}


// -----------------------------------------------------------------------------
// clock_frame_mode_1
//
// Advances the APU frame counter through its 5-step sequence.
//
// Quarter-frame events clock envelopes and the triangle linear counter.
// Half-frame events additionally clock length counters and square-channel
// sweeps.  Unlike mode 0, this sequence does not generate a frame IRQ.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
clock_frame_mode_1()
{
	// 5 step sequence
	switch (clock_step_) {
	case 0:
		log_frame_event(APU_FRAME_EVENT_QUARTER_HALF);
		
		clock_linear();
		clock_length();
		next_clock_ += 7458;
		break;

	case 1:
		log_frame_event(APU_FRAME_EVENT_QUARTER);

		clock_linear();
		next_clock_ += 7456;
		break;

	case 2:
		log_frame_event(APU_FRAME_EVENT_QUARTER_HALF);

		clock_linear();
		clock_length();
		next_clock_ += 7458;
		break;

	case 3:
		log_frame_event(APU_FRAME_EVENT_QUARTER);

		clock_linear();
		next_clock_ += 7456;
		break;

	case 4:
		next_clock_ += 7454;
		break;
	}
	
	clock_step_ = (clock_step_ + 1) % 5;
}




// -----------------------------------------------------------------------------
// mix_native_channels_raw
//
// Computes the instantaneous native NES APU output using the nonlinear
// pulse and triangle/noise/DMC mixing equations.
//
// These equations approximate the nonlinear DAC mixing characteristics
// of the original NES hardware.
//
// This function excludes cartridge expansion audio and does not modify
// the FIR resampler, DC-blocking filter, or host PCM buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Native APU output on the nominal 0-255 scale.
// -----------------------------------------------------------------------------
static double
mix_native_channels_raw()
{
    // Read the instantaneous native APU channel outputs.
    const int32_t square1Out = square_0.output();
    const int32_t square2Out = square_1.output();
    const double triangleOut = sTriangleMuteHeldDAC + sTriangleMuteGain *
							   (static_cast<double>(triangle.raw_output()) - sTriangleMuteHeldDAC);
    const int32_t noiseOut = noise.output();

    // Blend between the live DMC DAC and the debugger-held DAC level.
	const double dmcOut = sDmcMuteHeldDAC + sDmcMuteGain * (static_cast<double>(dmc.raw_output()) - sDmcMuteHeldDAC);

    // -------------------------------------------------------------------------
    // Nonlinear pulse-channel mixing.
    //
    // The two pulse channels share a nonlinear DAC mixing circuit.
    // -------------------------------------------------------------------------

    double squareOut = 0.0;
    const int32_t pulseSum = square1Out + square2Out;

    if (pulseSum > 0) {
        squareOut = 95.88 / (8128.0 / static_cast<double>(pulseSum) + 100.0);
    }

    // -------------------------------------------------------------------------
    // Nonlinear triangle/noise/DMC mixing.
    //
    // These three channels share a separate nonlinear mixing circuit.
    // -------------------------------------------------------------------------

    const double tndInput = triangleOut / 8227.0 + static_cast<double>(noiseOut) / 12241.0 +
    						static_cast<double>(dmcOut) / 22638.0;
    double tndOut = 0.0;

    if (tndInput > 0.0) {
        tndOut = 159.79 / (1.0 / tndInput + 100.0);
    }

    // Convert the normalized native APU output to the existing
    // nominal 0-255 mixer scale.
    return (squareOut + tndOut) * 255.0;
}




// -----------------------------------------------------------------------------
// mix_channels_raw
//
// Returns the instantaneous, unfiltered output of the native NES APU
// and cartridge expansion audio.
//
// Native APU mixing is calculated by mix_native_channels_raw().
//
// Expansion audio is adjusted for the mapper's output bias and combined
// using the emulator's existing polarity and scaling conventions.
//
// This function does not advance filter state, quantize the sample,
// or write to the host audio buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Combined native APU and expansion-audio amplitude.
// -----------------------------------------------------------------------------
static double
mix_channels_raw()
{
    // Calculate the native APU output.
    const double nativeOutput = mix_native_channels_raw();
    double expansionOutput = 0.0;

    if (nes::cart.mapper() != nullptr) {
        const int32_t mapperOutput = nes::cart.mapper()->audio_output();
        const int32_t mapperBias = nes::cart.mapper()->audio_output_bias();

        expansionOutput = 0.00752 * static_cast<double>(mapperOutput - mapperBias);
    }

    // Preserve the existing expansion-audio polarity and gain.
    return nativeOutput - (expansionOutput * 255.0);
}


// -----------------------------------------------------------------------------
// nes::apu::mix_channels
//
// Converts the FIR-resampled mixed APU output into unsigned 8-bit PCM.
//
// A one-pole high-pass filter removes DC offset before the sample is
// centered around unsigned 8-bit PCM silence.
//
// The filter state is advanced exactly once per generated host sample.
//
// The final PCM conversion uses rounding rather than truncation to prevent
// tiny floating-point variations from becoming audible quantization tones.
//
// Parameters:
//   input - Band-limited mixed-audio amplitude.
//
// Returns:
//   Mixed unsigned 8-bit PCM sample.
// -----------------------------------------------------------------------------
uint8_t
mix_channels(double input)
{
    constexpr double R = 0.995;

    const double output = input - sDCBlockPreviousInput + R * sDCBlockPreviousOutput;

    sDCBlockPreviousInput = input;
    sDCBlockPreviousOutput = output;

    const int32_t sample = static_cast<int32_t>(std::lround(output + 128.0));

    return static_cast<uint8_t>(std::clamp(sample, 0, 255));
}




// -----------------------------------------------------------------------------
// nes::apu::reset
//
// Resets APU timing, channel state, sample-output state, queued host samples,
// and the DC-blocking filter.
//
// Parameters:
//   reset_type - Hard or soft reset mode.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
reset (reset_type type)
{
	status.raw     = 0;
	frame_counter_ = {0};
	apu_cycles_    = 0;
	next_clock_    = 0;
	clock_step_    = 0;

	/*
	 * Discard any PCM generated before the reset.  Otherwise the next host
	 * audio block can begin with stale pre-reset samples followed immediately
	 * by newly generated post-reset samples, creating an audible discontinuity.
	 */
	sample_buffer_start = 0;
	sample_buffer_end   = 0;

	sDCBlockPreviousInput  = 0.0;
	sDCBlockPreviousOutput = 0.0;
	
	debug_clear_scope();
	debug_clear_frame_events();

	if (type == reset_type::hard) {
		last_frame_counter_ = 0;
	}
	
	triangle.reset();
	sTriangleWasMuted = triangle.debug_muted();
	sTriangleMuteHeldDAC = static_cast<double>(triangle.raw_output());
	sTriangleMuteGain = sTriangleWasMuted ? 0.0 : 1.0;

   // Initialize DMC debugger mute state.
	sDmcWasMuted = dmc.debug_muted();
	sDmcMuteHeldDAC = static_cast<double>(dmc.raw_output());
	sDmcMuteGain = sDmcWasMuted ? 0.0 : 1.0;

	write4017(last_frame_counter_);
	write4015(0x00);

	// square 1
	write4000(10);
	write4001(00);
	write4002(00);
	write4003(00);

	// square 2
	write4004(10);
	write4005(00);
	write4006(00);
	write4007(00);

	// triangle
	//write4008(10); // triangle is unaffected..
	write400A(00);
	write400B(00);

	// noise
	write400C(10);
	write400E(00);
	write400F(00);

	// dmc
	write4010(10);

	// OK, the APU is supposed to act as if it has run for approximately 9
	// cycles by the time the reset is complete. I believe that the first 7
	// of these cycles are the 7 cycles of the reset itself. So we run the
	// APU manually for an extra 2 ticks.
	//
	// Blargg says it is as if this happens
	//
	//       lda   #$00
	//       sta   $4017       ; 1
	//       lda   <0          ; 9 delay
	//       nop
	//       nop
	//       nop
	//     reset:
	if (type == reset_type::hard) {
		exec<2>();
	}

	/*
 	 * Initialize the resampler history from the current mixed signal.
 	 *
 	 * This prevents an existing, held DAC level from appearing as a
 	 * sudden transition when audio generation begins.
 	 */
	const double initialAudioLevel = mix_channels_raw();

	audio_resampler.reset(initialAudioLevel);

	/*
	 * Initialize the DC blocker at the same signal level.
 	 *
 	 * The resampler has unity DC gain, so a constant signal should
 	 * initially produce silence after DC blocking.
 	 */
	sDCBlockPreviousInput = initialAudioLevel;
	sDCBlockPreviousOutput = 0.0;

	std::cout << "APU Reset complete" << std::endl;
}


// -----------------------------------------------------------------------------
// write4000
//
// Writes the first square channel's control register.
//
// Parameters:
//   value - Value written to APU register $4000.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4000 (uint8_t value)
{
	log_write(0x4000, value);
	explorer_square1_[0] = value;
	square_0.write_reg0(value);
}


// -----------------------------------------------------------------------------
// write4001
//
// Writes the first square channel's sweep register.
//
// Parameters:
//   value - Value written to APU register $4001.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
write4001 (uint8_t value)
{
	log_write(0x4001, value);
	explorer_square1_[1] = value;
	square_0.write_reg1(value);
}


// -----------------------------------------------------------------------------
// write4002
//
// Writes the low timer byte for the first square channel.
//
// Parameters:
//   value - Value written to APU register $4002.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4002 (uint8_t value)
{
	log_write(0x4002, value);
	explorer_square1_[2] = value;
	square_0.write_reg2(value);
}


// -----------------------------------------------------------------------------
// write4003
//
// Writes the high timer bits and length-counter load value for the first square
// channel.
//
// Parameters:
//   value - Value written to APU register $4003.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4003 (uint8_t value)
{
	log_write(0x4003, value);
	explorer_square1_[3] = value;
	square_0.write_reg3(value);
}


// -----------------------------------------------------------------------------
// write4004
//
// Writes the second square channel's control register.
//
// Parameters:
//   value - Value written to APU register $4004.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4004 (uint8_t value)
{
	log_write(0x4004, value);
	explorer_square2_[0] = value;
	square_1.write_reg0(value);
}


// -----------------------------------------------------------------------------
// write4005
//
// Writes the second square channel's sweep register.
//
// Parameters:
//   value - Value written to APU register $4005.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4005(uint8_t value)
{
	log_write(0x4005, value);
	explorer_square2_[1] = value;
	square_1.write_reg1(value);
}


// -----------------------------------------------------------------------------
// write4006
//
// Writes the low timer byte for the second square channel.
//
// Parameters:
//   value - Value written to APU register $4006.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write4006 (uint8_t value)
{
	log_write(0x4006, value);
	explorer_square2_[2] = value;
	square_1.write_reg2(value);
}


// -----------------------------------------------------------------------------
// write4007
//
// Writes the high timer bits and length-counter load value for the second square
// channel.
//
// Parameters:
//   value - Value written to APU register $4007.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4007 (uint8_t value)
{
	log_write(0x4007, value);
	explorer_square2_[3] = value;
	square_1.write_reg3(value);
}


// -----------------------------------------------------------------------------
// write4008
//
// Writes the triangle channel's linear-counter control register.
//
// Parameters:
//   value - Value written to APU register $4008.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4008 (uint8_t value)
{
	log_write(0x4008, value);
	explorer_triangle0_ = value;
	triangle.write_reg0(value);
}


// -----------------------------------------------------------------------------
// write400A
//
// Writes the low timer byte for the triangle channel.
//
// Parameters:
//   value - Value written to APU register $400A.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write400A (uint8_t value)
{
	log_write(0x400a, value);
	explorer_triangle2_ = value;
	triangle.write_reg2(value);
}


// -----------------------------------------------------------------------------
// write400B
//
// Writes the high timer bits and length-counter load value for the triangle
// channel.
//
// Parameters:
//   value - Value written to APU register $400B.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write400B (uint8_t value)
{
	log_write(0x400b, value);
	explorer_triangle3_ = value;
	triangle.write_reg3(value);
}


// -----------------------------------------------------------------------------
// write400C
//
// Writes the noise channel's envelope and length-counter control register.
//
// Parameters:
//   value - Value written to APU register $400C.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write400C (uint8_t value)
{
	log_write(0x400c, value);
	explorer_noise0_ = value;
	noise.write_reg0(value);
}


// -----------------------------------------------------------------------------
// write400E
//
// Writes the noise channel's mode and timer-period register.
//
// Parameters:
//   value - Value written to APU register $400E.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write400E (uint8_t value)
{
	log_write(0x400e, value);
	explorer_noise2_ = value;
	noise.write_reg2(value);
}


// -----------------------------------------------------------------------------
// write400F
//
// Writes the noise channel's length-counter load register.
//
// Parameters:
//   value - Value written to APU register $400F.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write400F (uint8_t value)
{
	log_write(0x400f, value);
	explorer_noise3_ = value;
	noise.write_reg3(value);
}


// -----------------------------------------------------------------------------
// write4010
//
// Writes the DMC control register.
//
// Parameters:
//   value - Value written to APU register $4010.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4010 (uint8_t value)
{
	log_write(0x4010, value);
	explorer_dmc_[0] = value;
	dmc.write_reg0(value);
}


// -----------------------------------------------------------------------------
// write4011
//
// Writes the DMC direct-load output register.
//
// Parameters:
//   value - Value written to APU register $4011.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
write4011 (uint8_t value)
{
	log_write(0x4011, value);
	explorer_dmc_[1] = value;
	dmc.write_reg1(value);
}


// -----------------------------------------------------------------------------
// write4012
//
// Writes the DMC sample start-address register.
//
// Parameters:
//   value - Value written to APU register $4012.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4012 (uint8_t value)
{
	log_write(0x4012, value);
	explorer_dmc_[2] = value;
	dmc.write_reg2(value);
}


// -----------------------------------------------------------------------------
// write4013
//
// Writes the DMC sample-length register.
//
// Parameters:
//   value - Value written to APU register $4013.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4013 (uint8_t value)
{
	log_write(0x4013, value);
	explorer_dmc_[3] = value;
	dmc.write_reg3(value);
}


// -----------------------------------------------------------------------------
// write4015
//
// Writes the APU channel-enable and DMC control register.
//
// This enables or disables each audio channel according to the corresponding
// status bits.  Writing this register also clears the DMC interrupt flag and,
// when no other APU interrupt remains active, clears the CPU APU IRQ source.
//
// Parameters:
//   value - Value written to APU register $4015.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4015 (uint8_t value)
{
	log_write(0x4015, value);

	explorer_status_ = value;

	// writing to this register clears the DMC interrupt flag.
	status.dmc_irq = false;

	square_0.set_enabled(value & apu_status::ENABLE_SQUARE1);
	square_1.set_enabled(value & apu_status::ENABLE_SQUARE2);
	triangle.set_enabled(value & apu_status::ENABLE_TRIANGLE);
	noise.set_enabled(value & apu_status::ENABLE_NOISE);
	dmc.set_enabled(value & apu_status::ENABLE_DMC);

	if (!status.irq_firing) {
		cpu::clear_irq(cpu::APU_IRQ);
	}
}


// -----------------------------------------------------------------------------
// read4015
//
// Reads the APU status register.
//
// The returned value reports active channel length/sample state together with
// the DMC and frame interrupt flags.  Reading this register clears the frame IRQ
// flag and clears the CPU APU IRQ source when no APU interrupt remains active.
//
// Parameters:
//   None.
//
// Returns:
//   Current APU status-register value.
// -----------------------------------------------------------------------------
uint8_t 
read4015()
{
	uint8_t ret = status.raw & (apu_status::DMC_IRQ | apu_status::FRAME_IRQ);

	// reading this register clears the Frame interrupt flag.
	status.frame_irq = false;

	if (square_0.length_counter.value() > 0) {
		ret |= apu_status::ENABLE_SQUARE1;
	}

	if (square_1.length_counter.value() > 0) {
		ret |= apu_status::ENABLE_SQUARE2;
	}

	if (triangle.length_counter.value() > 0) {
		ret |= apu_status::ENABLE_TRIANGLE;
	}

	if (noise.length_counter.value() > 0) {
		ret |= apu_status::ENABLE_NOISE;
	}

	if (dmc.bytes_remaining() > 0) {
		ret |= apu_status::ENABLE_DMC;
	}

	if (!status.irq_firing) {
		cpu::clear_irq(cpu::APU_IRQ);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// write4017
//
// Writes the APU frame-counter control register.
//
// This selects the frame-counter sequence mode, controls frame IRQ inhibition,
// resets the frame-sequence step, and schedules the next frame-counter event
// according to the current APU-cycle parity.
//
// Parameters:
//   value - Value written to APU register $4017.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write4017 (uint8_t value)
{
	log_write(0x4017, value);
	
	frame_counter_.raw  = value;
	last_frame_counter_ = value;

	if (frame_counter_.inhibit_frame_irq) {
		status.frame_irq = false;

		if (!status.irq_firing) {
			cpu::clear_irq(cpu::APU_IRQ);
		}
	}

	next_clock_ = apu_cycles_ + (apu_cycles_ & 1) + 1;

	clock_step_ = 0;

	if (!frame_counter_.mode) {
		next_clock_ += 7458;
	}
}


// -----------------------------------------------------------------------------
// nes::apu::tick
//
// Advances the APU by one emulated CPU cycle.
//
// Handles frame-counter timing, interrupt generation, FIR audio resampling,
// debugger oscilloscope capture, host PCM buffering, and channel clocking.
//
// The shared FIR resampler receives one raw mixed-audio value per CPU cycle.
// When a host-rate sample becomes available, the DC blocker and unsigned
// eight-bit PCM conversion are applied exactly once.
//
// Triangle debugger muting is smoothed over approximately five milliseconds.
// The smoothing gain is advanced once per emulated CPU cycle, independently
// of debugger refreshes and host audio sample generation.
//
// This does not modify the Triangle's internal timer, waveform sequencer,
// length counter, linear counter, or hardware DAC state.
//
// The circular PCM buffer uses equal read/write positions to indicate an
// empty buffer. One slot is therefore reserved to distinguish a full buffer
// from an empty one.
//
// If the buffer becomes full, the oldest unread PCM sample is discarded
// before the newly generated sample is queued.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
tick()
{
    // -------------------------------------------------------------------------
    // Handle pending frame-counter interrupts.
    // -------------------------------------------------------------------------

    if (!(frame_counter_.inhibit_frame_irq) && status.frame_irq) {
        cpu::irq(cpu::APU_IRQ);
    }

    // -------------------------------------------------------------------------
    // Advance the APU frame sequencer when its next event is due.
    // -------------------------------------------------------------------------

    if (apu_cycles_ == next_clock_) {
        if (frame_counter_.mode) {
            clock_frame_mode_1();
        } else {
            clock_frame_mode_0();
        }
    }

    // -------------------------------------------------------------------------
	// Advance Triangle debugger mute smoothing.
	//
	// On a new mute request, capture the current effective Triangle DAC level.
	// This becomes the constant output held while debugger-muted.
	//
	// Holding the DAC level prevents artificial DC steps while removing the
	// Triangle waveform's time-varying contribution.
	//
	// The mute gain transitions smoothly between:
	//   1.0 - Live Triangle waveform.
	//   0.0 - Held Triangle DAC level.
	//
	// The actual Triangle sequencer continues running independently.
	// -------------------------------------------------------------------------

	const bool triangleMuted = triangle.debug_muted();

	if (triangleMuted && !sTriangleWasMuted) {
		const double rawDAC = static_cast<double>(triangle.raw_output());

		// Capture the current effective DAC level to maintain continuity,
		// even if a previous fade was interrupted.
		sTriangleMuteHeldDAC += sTriangleMuteGain * (rawDAC - sTriangleMuteHeldDAC);
	}

	sTriangleWasMuted = triangleMuted;

	if (triangleMuted) {
		sTriangleMuteGain = std::max(0.0, sTriangleMuteGain - kTriangleMuteGainStep);
	} else {
		sTriangleMuteGain = std::min(1.0, sTriangleMuteGain + kTriangleMuteGainStep);
	}

	// -------------------------------------------------------------------------
	// Advance DMC debugger mute smoothing.
	//
	// Capture the effective DAC level when debugger muting begins.
	//
	// As the gain approaches zero, the mixer transitions from the live
	// DMC DAC to a held constant level.
	//
	// This prevents abrupt removal of the DMC's retained DAC voltage.
	//
	// DMC sample decoding, DMA, timers, and interrupts are unaffected.
	// -------------------------------------------------------------------------

	const bool dmcMuted = dmc.debug_muted();

	if (dmcMuted && !sDmcWasMuted) {
		const double rawDAC = static_cast<double>(dmc.raw_output());

		// Capture the current effective output, including any incomplete fade.
		sDmcMuteHeldDAC += sDmcMuteGain * (rawDAC - sDmcMuteHeldDAC);
	}

	sDmcWasMuted = dmcMuted;

	if (dmcMuted) {
		sDmcMuteGain = std::max(0.0, sDmcMuteGain - kDmcMuteGainStep);
	} else {
		sDmcMuteGain = std::min(1.0, sDmcMuteGain + kDmcMuteGainStep);
	}

    // -------------------------------------------------------------------------
    // Feed the instantaneous mixed output into the FIR resampler.
    //
    // The mapper has already advanced through CPU synchronization.
    // Native channel state is sampled before the channel clocks below,
    // preserving the existing timing relationship.
    // -------------------------------------------------------------------------

    double filteredSample = 0.0;

    if (audio_resampler.clock(mix_channels_raw(), filteredSample)) {

        // ---------------------------------------------------------------------
        // Apply DC blocking and convert to unsigned 8-bit host PCM.
        //
        // This must occur exactly once per generated host sample.
        // ---------------------------------------------------------------------

        const uint8_t mixedSample = mix_channels(filteredSample);

        // ---------------------------------------------------------------------
        // Capture the generated audio sample for the debugger oscilloscope.
        //
        // Capturing the existing PCM sample avoids advancing the
        // stateful DC-blocking filter a second time.
        // ---------------------------------------------------------------------

        capture_scope_sample(mixedSample);

        // ---------------------------------------------------------------------
        // Apply debugger-controlled host audio muting.
        //
        // Muting affects the queued PCM output, not the underlying APU
        // channel clocks or FIR resampler.
        // ---------------------------------------------------------------------

        const uint8_t outputSample = debug_audio_is_muted() ? kSilence : mixedSample;

        // ---------------------------------------------------------------------
        // Queue the PCM sample with circular-buffer overflow protection.
        //
        // The read and write indices must not become equal after writing,
        // since equal positions represent an empty buffer.
        //
        // When full, advance the read index to discard the oldest
        // unread sample and make room for the new one.
        // ---------------------------------------------------------------------

        const size_t nextWrite = (sample_buffer_end + 1) % kBufferSize;

        if (nextWrite == sample_buffer_start) {
            sample_buffer_start = (sample_buffer_start + 1) % kBufferSize;
        }

        sample_buffer_[sample_buffer_end] = outputSample;
        sample_buffer_end = nextWrite;
    }

    // -------------------------------------------------------------------------
    // Advance the individual native APU channels.
    //
    // Preserve the original clock order.
    // -------------------------------------------------------------------------

    dmc.tick();
    noise.tick();
    triangle.tick();
    square_0.tick();
    square_1.tick();

    // Advance the global APU cycle counter.
    ++apu_cycles_;
}


// -----------------------------------------------------------------------------
// cycle_count
//
// Returns the current number of APU cycles that have elapsed.
//
// Parameters:
//   None.
//
// Returns:
//   Current APU cycle count.
// -----------------------------------------------------------------------------
uint64_t
cycle_count()
{
	return apu_cycles_;
}


// -----------------------------------------------------------------------------
// nes::apu::read_samples
//
// Reads mixed APU samples into the supplied host audio buffer.
//
// If fewer APU samples are available than requested, the last real output
// sample is repeated rather than abruptly inserting 0x80 silence.  This avoids
// introducing a discontinuity when the generated sample count is slightly
// shorter than the fixed host frame request.
//
// Parameters:
//   buffer - Destination audio buffer.
//   size   - Number of samples requested.
//
// Returns:
//   Number of samples written.
// -----------------------------------------------------------------------------
size_t
read_samples (uint8_t *buffer, size_t size)
{
	if (!buffer || size == 0) {
		return 0;
	}

	/*
	 * Debugger mute is intentional host silence rather than an APU
	 * underrun. Discard anything accumulated while muted and satisfy
	 * the requested buffer with silence.
	 */
	if (debug_audio_is_muted()) {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = kSilence;
		}

		sample_buffer_start = sample_buffer_end;
		return size;
	}

	/*
	 * Return only samples that the APU has actually generated.
	 *
	 * Do not pad a shortage here. The window-side audio staging code
	 * will accumulate real samples across video frames and submit a
	 * complete host block only when enough audio is available.
	 */
	size_t i = 0;

	while (i < size && sample_buffer_start != sample_buffer_end) {
		const uint8_t sample = sample_buffer_[sample_buffer_start];

		buffer[i] = sample;

		sample_buffer_start = (sample_buffer_start + 1) % kBufferSize;
		++i;
	}

	return i;
}


// -----------------------------------------------------------------------------
// nes::apu::start_frame
//
// Marks the beginning of an emulated video frame.
//
// Audio sample generation is continuous across video-frame boundaries, so no
// APU state or queued samples are reset here.  The function remains as part of
// the frame-lifecycle interface used by the emulator.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
start_frame()
{
	/*
	 we don't want to do this:
	
		sample_buffer_start = sample_buffer_end;
	
	that discards queued samples at every video-frame boundary and is one 
	of the causes of audio discontinuities.
	*/
}


// -----------------------------------------------------------------------------
// mute_channel
//
// Mutes one APU sound channel without changing its enabled state or internal
// timing.
//
// Parameters:
//   channel - Sound-channel identifier from sound_channel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
mute_channel (int const channel)
{	
	switch (channel) {
		case sound_channel::SQUARE1:
		square_0.mute();
		break;
		
		case sound_channel::SQUARE2:
		square_1.mute();
		break;
		
		case sound_channel::TRIANGLE:
		triangle.mute();
		break;
		
		case sound_channel::NOISE:
		noise.mute();
		break;
		
		case sound_channel::DPCM:
		dmc.mute();
		break;
	}			
}


// -----------------------------------------------------------------------------
// unmute_channel
//
// Restores audible output for one APU sound channel that was previously muted,
// without changing its enabled state or internal timing.
//
// Parameters:
//   channel - Sound-channel identifier from sound_channel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
unmute_channel (int const channel)
{
	switch (channel) {
		case sound_channel::SQUARE1:
		square_0.unmute();
		break;
		
		case sound_channel::SQUARE2:
		square_1.unmute();
		break;
		
		case sound_channel::TRIANGLE:
		triangle.unmute();
		break;
		
		case sound_channel::NOISE:
		noise.unmute();
		break;
		
		case sound_channel::DPCM:
		dmc.unmute();
		break;
	}			
}


// -----------------------------------------------------------------------------
// nes::apu::debug_state
//
// Returns a side-effect-free snapshot of the current native APU state for
// the debugger views.
//
// Captures global frame-sequencer timing, interrupt status, channel enable
// and mute states, timer settings, counters, and instantaneous output levels.
//
// The Global APU output is calculated using mix_native_channels_raw(),
// which applies the existing linear native APU mixing approximation.
//
// This output excludes cartridge expansion audio and does not advance the
// resampler, DC-blocking filter, or host PCM buffer.
//
// This function intentionally avoids read4015(), because reading $4015
// has emulation-visible side effects, including clearing the frame IRQ.
//
// Parameters:
//   None.
//
// Returns:
//   Side-effect-free snapshot of the current native APU state.
// -----------------------------------------------------------------------------
debug_state_t
debug_state()
{
    debug_state_t state;

    // -------------------------------------------------------------------------
    // Global APU state
    // -------------------------------------------------------------------------

    state.cycle = apu_cycles_;

    state.five_step_mode = frame_counter_.mode;
    state.frame_irq_inhibit = frame_counter_.inhibit_frame_irq;
    state.frame_step = clock_step_;
    state.next_frame_cycle = next_clock_;

    state.frame_irq = status.frame_irq;
    state.dmc_irq = status.dmc_irq;

    state.audio_muted = debug_audio_is_muted();

    // Capture the instantaneous native APU output for the Global APU meter.
    // Do not include mapper expansion audio or advance the audio filters.
    const double nativeOutput = mix_native_channels_raw();

    state.output = static_cast<uint8_t>(std::clamp(nativeOutput, 0.0, 255.0));


    // -------------------------------------------------------------------------
    // Square 1
    // -------------------------------------------------------------------------

    state.square1.enabled = square_0.enabled();
    state.square1.muted = square_0.debug_muted();

    state.square1.timer_period = square_0.debug_timer_period();
    state.square1.timer_frequency = square_0.debug_timer_frequency();

    state.square1.duty = square_0.debug_duty();
    state.square1.sequence_index = square_0.debug_sequence_index();

    state.square1.length_counter = square_0.length_counter.debug_value();
    state.square1.envelope_volume = square_0.envelope.debug_volume();

    state.square1.sweep_enabled = square_0.sweep.debug_enabled();
    state.square1.sweep_period = square_0.sweep.debug_period();
    state.square1.sweep_negate = square_0.sweep.debug_negate();
    state.square1.sweep_shift = square_0.sweep.debug_shift();
    state.square1.sweep_silenced = square_0.sweep.debug_silenced();

    state.square1.output = square_0.debug_output();


    // -------------------------------------------------------------------------
    // Square 2
    // -------------------------------------------------------------------------

    state.square2.enabled = square_1.enabled();
    state.square2.muted = square_1.debug_muted();

    state.square2.timer_period = square_1.debug_timer_period();
    state.square2.timer_frequency = square_1.debug_timer_frequency();

    state.square2.duty = square_1.debug_duty();
    state.square2.sequence_index = square_1.debug_sequence_index();

    state.square2.length_counter = square_1.length_counter.debug_value();
    state.square2.envelope_volume = square_1.envelope.debug_volume();

    state.square2.sweep_enabled = square_1.sweep.debug_enabled();
    state.square2.sweep_period = square_1.sweep.debug_period();
    state.square2.sweep_negate = square_1.sweep.debug_negate();
    state.square2.sweep_shift = square_1.sweep.debug_shift();
    state.square2.sweep_silenced = square_1.sweep.debug_silenced();

    state.square2.output = square_1.debug_output();


    // -------------------------------------------------------------------------
    // Triangle
    // -------------------------------------------------------------------------

    state.triangle.enabled = triangle.enabled();
    state.triangle.muted = triangle.debug_muted();

    state.triangle.timer_period = triangle.debug_timer_period();
    state.triangle.timer_frequency = triangle.debug_timer_frequency();

    state.triangle.length_counter = triangle.length_counter.debug_value();

    state.triangle.linear_counter = triangle.linear_counter.value();
    state.triangle.sequence_index = triangle.debug_sequence_index();

    state.triangle.output = triangle.output();


    // -------------------------------------------------------------------------
    // Noise
    // -------------------------------------------------------------------------

    state.noise.enabled = noise.enabled();
    state.noise.muted = noise.debug_muted();

    state.noise.timer_period = noise.debug_timer_period();

    state.noise.length_counter = noise.length_counter.debug_value();

    state.noise.envelope_volume = noise.envelope.debug_volume();

    state.noise.output = noise.debug_output();


    // -------------------------------------------------------------------------
    // DMC / DPCM
    // -------------------------------------------------------------------------

    state.dmc.enabled = dmc.debug_enabled();
    state.dmc.active = dmc.debug_active();
    state.dmc.muted = dmc.debug_muted();

    state.dmc.timer_period = dmc.debug_timer_period();

    state.dmc.irq_enabled = dmc.debug_irq_enabled();
    state.dmc.loop = dmc.debug_loop();

    // display the hardware DAC level, regardless of debugger muting.
	state.dmc.output = dmc.raw_output();

    state.dmc.sample_address = dmc.debug_sample_address();
    state.dmc.current_address = dmc.debug_current_address();
    state.dmc.sample_length = dmc.debug_sample_length();

    state.dmc.bytes_remaining = dmc.bytes_remaining();
    state.dmc.bits_remaining = dmc.debug_bits_remaining();

    state.dmc.sample_buffer_empty = dmc.debug_sample_buffer_empty();


    // -------------------------------------------------------------------------
    // Reconstruct $4015 status without reading the hardware register.
    //
    // The native channel bits indicate remaining length-counter or DMC
    // sample activity, rather than simply reflecting channel enable flags.
    // -------------------------------------------------------------------------

    uint8_t statusValue = 0x00;

    if (state.square1.length_counter != 0) {
        statusValue |= 0x01;
    }

    if (state.square2.length_counter != 0) {
        statusValue |= 0x02;
    }

    if (state.triangle.length_counter != 0) {
        statusValue |= 0x04;
    }

    if (state.noise.length_counter != 0) {
        statusValue |= 0x08;
    }

    if (state.dmc.bytes_remaining != 0) {
        statusValue |= 0x10;
    }

    if (state.frame_irq) {
        statusValue |= 0x40;
    }

    if (state.dmc_irq) {
        statusValue |= 0x80;
    }

    state.status = statusValue;

    return state;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_frame_sequencer_state
//
// Returns a side-effect-free snapshot of the APU frame sequencer.
//
// clock_step_ represents the next internal frame-sequencer event that will be
// executed when apu_cycles_ reaches next_clock_.  The returned event flags
// describe the quarter-frame, half-frame, and IRQ behavior associated with
// that upcoming event.
//
// The 4-step sequence uses six internal scheduling points because frame IRQ
// assertion occurs across the closely spaced final sequence clocks.
//
// The 5-step sequence uses five internal scheduling points and never generates
// a frame IRQ.
//
// Parameters:
//   None.
//
// Returns:
//   Current APU frame-sequencer debugger state.
// -----------------------------------------------------------------------------
frame_sequencer_debug_state_t
debug_frame_sequencer_state()
{
	frame_sequencer_debug_state_t state;

	state.apu_cycle = apu_cycles_;
	state.next_event_cycle = next_clock_;

	if (next_clock_ >= apu_cycles_) {
		state.cycles_until_next_event = next_clock_ - apu_cycles_;
	} else {
		state.cycles_until_next_event = 0;
	}

	state.next_step = clock_step_;
	state.five_step_mode = frame_counter_.mode;
	state.step_count = state.five_step_mode ? 5 : 6;
	state.irq_inhibit = frame_counter_.inhibit_frame_irq;
	state.frame_irq = status.frame_irq;

	state.next_event_quarter_frame = false;
	state.next_event_half_frame = false;
	state.next_event_irq_point = false;

	state.frame_counter_register = last_frame_counter_;

	if (state.five_step_mode) {
		switch (clock_step_) {
			case 0:
				state.next_event_quarter_frame = true;
				state.next_event_half_frame = true;
				break;

			case 1:
				state.next_event_quarter_frame = true;
				break;

			case 2:
				state.next_event_quarter_frame = true;
				state.next_event_half_frame = true;
				break;

			case 3:
				state.next_event_quarter_frame = true;
				break;

			case 4:
				break;
		}
	} else {
		switch (clock_step_) {
			case 0:
				state.next_event_quarter_frame = true;
				break;

			case 1:
				state.next_event_quarter_frame = true;
				state.next_event_half_frame = true;
				break;

			case 2:
				state.next_event_quarter_frame = true;
				break;

			case 3:
				state.next_event_irq_point = true;
				break;

			case 4:
				state.next_event_quarter_frame = true;
				state.next_event_half_frame = true;
				state.next_event_irq_point = true;
				break;

			case 5:
				state.next_event_irq_point = true;
				break;
		}
	}

	return state;
}


// -----------------------------------------------------------------------------
// log_frame_event
//
// Appends one APU frame-sequencer event to the debugger history.
//
// Parameters:
//   type - Event type being recorded.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
log_frame_event(apu_frame_event_type type)
{
	frame_event_t &event = frame_events_[frame_event_next_];
	event.cycle = apu_cycles_;
	event.type = type;
	event.step = clock_step_;
	event.five_step_mode = frame_counter_.mode;
	event.irq_inhibit = frame_counter_.inhibit_frame_irq;

	frame_event_next_ = (frame_event_next_ + 1) % APU_FRAME_EVENT_CAPACITY;

	if (frame_event_count_ < APU_FRAME_EVENT_CAPACITY) {
		frame_event_count_++;
	}
}


// -----------------------------------------------------------------------------
// nes::apu::explorer_state
//
// Returns a side-effect-free snapshot of the most recently programmed raw APU
// register values for the APU Explorer debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot of the current raw APU register programming state.
// -----------------------------------------------------------------------------
explorer_state_t
explorer_state()
{
	explorer_state_t state;

	for (int i = 0; i < 4; i++) {
		state.square1[i] = explorer_square1_[i];
		state.square2[i] = explorer_square2_[i];
		state.dmc[i] = explorer_dmc_[i];
	}

	state.triangle0 = explorer_triangle0_;
	state.triangle2 = explorer_triangle2_;
	state.triangle3 = explorer_triangle3_;

	state.noise0 = explorer_noise0_;
	state.noise2 = explorer_noise2_;
	state.noise3 = explorer_noise3_;

	state.status = explorer_status_;
	state.frame_counter = last_frame_counter_;

	return state;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_set_audio_muted
//
// Enables or disables debugger audio mute.  The queued sample buffer is flushed
// whenever the mute state changes so 1 audio cannot remain frozen in the
// host output path.
//
// Parameters:
//   muted - true to mute debugger audio output.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_set_audio_muted (bool muted)
{
	debug_audio_muted = muted;

	for (size_t i = 0; i < kBufferSize; i++) {
		sample_buffer_[i] = kSilence;
	}

	sample_buffer_start = 0;
	sample_buffer_end = 0;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_audio_is_muted
//
// Returns whether debugger audio mute is currently active.
//
// Parameters:
//   None.
//
// Returns:
//   true if debugger audio output is muted.
// -----------------------------------------------------------------------------
bool
debug_audio_is_muted()
{
	return debug_audio_muted;
}


// -----------------------------------------------------------------------------
// nes::apu::apu_log_count
//
// Returns the number of valid entries currently stored in the rolling APU
// register-write log.
//
// Parameters:
//   None.
//
// Returns:
//   Current APU write-log entry count.
// -----------------------------------------------------------------------------
uint32_t
write_log_count()
{
	return write_log_count_;
}


// -----------------------------------------------------------------------------
// nes::apu::write_log_snapshot
//
// Copies the current logical APU write log into caller-provided storage.
//
// The ring-buffer count and starting position are captured once before copying,
// so every entry in the returned snapshot uses the same logical ordering.
//
// Entries are returned oldest to newest.
//
// Parameters:
//   entries  - Destination array.
//   capacity - Maximum number of entries that may be written.
//
// Returns:
//   Number of entries copied.
// -----------------------------------------------------------------------------
uint32_t
write_log_snapshot(write_log_entry_t *entries, uint32_t capacity)
{
	if (!entries || capacity == 0) {
		return 0;
	}

	const uint32_t available = write_log_count_;
	const uint32_t count = (available < capacity) ? available : capacity;

	if (count == 0) {
		return 0;
	}

	uint32_t start = 0;

	if (available == APU_WRITE_LOG_CAPACITY) {
		start = write_log_next_;
	}

	if (count < available) {
		start = (start + (available - count)) % APU_WRITE_LOG_CAPACITY;
	}

	for (uint32_t index = 0; index < count; index++) {
		const uint32_t physicalIndex = (start + index) % APU_WRITE_LOG_CAPACITY;

		entries[index] = write_log_[physicalIndex];
	}

	return count;
}


// -----------------------------------------------------------------------------
// nes::apu::clear_write_log
//
// Clears the rolling APU register-write log and resets its sequence state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clear_write_log()
{
	write_log_next_ = 0;
	write_log_count_ = 0;
	write_log_write_index_ = 0;
}


} // end namespace nes::apu

