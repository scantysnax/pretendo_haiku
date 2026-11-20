
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
#include <cstdio>

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
// Twenty milliseconds provides a short, click-reducing transition.
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

// Pending $4017 frame-counter restart.
bool frame_restart_pending_ = false;
uint64_t frame_restart_cycle_ = 0;
bool frame_pending_mode_ = false;

uint64_t last_half_frame_clock_cycle_ =
    static_cast<uint64_t>(-1);

uint64_t frame_last_write_cycle_ =
    static_cast<uint64_t>(-1);

// EXPERIMENT 23CT:
// Last successfully delivered quarter/half-frame event.
uint64_t last_delivered_frame_clock_cycle_ =
    static_cast<uint64_t>(-1);

// CPU/APU cycle on which the most recent $4017 write occurred.
// Used to distinguish a same-cycle frame event from later events
// while a delayed restart is pending.

// EXPERIMENT 23CV:
// Shared frame-clock arbitration counter.
uint8_t frame_clock_block_ticks_ = 0;



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


bool trace_length_test_ = false;
// NEW: Count the final $4015 reads in test 11.
int trace_length_reads_ = 0;



// Add alongside the frame-counter state.
uint64_t frame_sequence_start_cycle_ = 0;


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
    // ---------------------------------------------------------------------
    // TEMPORARY DIAGNOSTIC:
    //
    // Inspect length-counter internals during a pending
    // four-step to five-step frame-counter transition.
    //
    // Includes the old-sequence half-frame clock and the
    // immediate clock generated by the delayed restart.
    // ---------------------------------------------------------------------

    const bool trace_collision =
    	trace_length_test_
    		&& frame_pending_mode_
    		&& (
        		frame_restart_pending_
        		|| apu_cycles_ == frame_restart_cycle_
    		);

    if (trace_collision) {

        printf(
            "[LENGTH COLLISION BEFORE] "
            "apu=%llu write=%llu restart=%llu "
            "previous_half=%llu\n",

            (unsigned long long)apu_cycles_,
            (unsigned long long)frame_last_write_cycle_,
            (unsigned long long)frame_restart_cycle_,
            (unsigned long long)last_half_frame_clock_cycle_
        );

        square_0.length_counter.debug_dump_state();
    }

    // ---------------------------------------------------------------------
    // Record this half-frame clock.
    // ---------------------------------------------------------------------

    last_half_frame_clock_cycle_ = apu_cycles_;

    const uint8_t old_length =
        square_0.length_counter.debug_value();

    // ---------------------------------------------------------------------
    // Clock pulse 1 length counter.
    // ---------------------------------------------------------------------

    square_0.length_counter.clock();

    if (trace_length_test_) {

        printf(
            "[APU TEST] HALF cycle=%llu LC=%u -> %u\n",

            (unsigned long long)apu_cycles_,
            (unsigned)old_length,
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    if (trace_collision) {

        printf(
            "[LENGTH COLLISION AFTER] apu=%llu\n",
            (unsigned long long)apu_cycles_
        );

        square_0.length_counter.debug_dump_state();
    }

    // ---------------------------------------------------------------------
    // Clock remaining length counters and sweep units.
    // ---------------------------------------------------------------------

    square_1.length_counter.clock();
    triangle.length_counter.clock();
    noise.length_counter.clock();

    square_0.sweep.clock();
    square_1.sweep.clock();
}





// EXPERIMENT 22N:
 // Suppress channel-clock delivery, not sequencer advancement.
static bool suppress_frame_channel_clocks_ = false;


// EXPERIMENT 23CI:
// Track whether the delayed $4017 quarter/half-frame clock
// has already been delivered before the divider restart.

static bool frame_write_clock_delivered_ = false;




enum class FrameClockSource : uint8_t {
    Sequencer,
    Write4017
};


// -----------------------------------------------------------------------------
// deliver_frame_clock
//
// Delivers a quarter-frame or quarter+half-frame clock to the
// native APU channels.
//
// EXPERIMENT 22E:
// Identify whether the clock originates from the ordinary frame
// sequencer or a delayed $4017 five-step write.
//
// This implementation does not suppress or delay any clocks.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// deliver_frame_clock
//
// Delivers quarter-frame or quarter+half-frame clocks to the
// native APU channels.
//
// EXPERIMENT 23CJ:
//
// Trace both clock sources before applying channel-clock suppression.
//
// This is diagnostic-only. No changes to frame-clock delivery,
// length-counter behavior, IRQ timing, or the frame sequencer.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// deliver_frame_clock
//
// Delivers quarter-frame or quarter+half-frame clocks to the native APU
// channels.
//
// EXPERIMENT 23CV:
// Use a shared two-tick countdown to arbitrate closely spaced frame-clock
// requests.
//
// A successfully delivered clock starts the blocking countdown.
// A blocked or suppressed request does not restart the countdown.
//
// The countdown itself is decremented once at the end of each APU tick.
// -----------------------------------------------------------------------------

static void
deliver_frame_clock(FrameClockSource source, bool half_frame)
{
    // ---------------------------------------------------------------------
    // EXPERIMENT 23CJ:
    // Trace both clock sources before applying suppression.
    // ---------------------------------------------------------------------

    if (trace_length_test_ &&
        frame_restart_pending_ &&
        frame_pending_mode_) {

        printf(
            "[23CJ CLOCK SIGNAL] "
            "apu=%llu source=%u half=%u "
            "step=%u suppressed=%u "
            "write=%llu restart=%llu "
            "last_half=%llu LC=%u\n",

            (unsigned long long)apu_cycles_,
            (unsigned)source,
            (unsigned)half_frame,
            (unsigned)clock_step_,
            (unsigned)suppress_frame_channel_clocks_,

            (unsigned long long)frame_last_write_cycle_,
            (unsigned long long)frame_restart_cycle_,
            (unsigned long long)last_half_frame_clock_cycle_,

            (unsigned)square_0.length_counter.debug_value()
        );
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 22N / 23BN:
    //
    // Preserve the existing suppression mechanism for diagnostics.
    // The sequencer advances even when its channel clocks are blocked.
    //
    // Under 23CV Stage 1, tick() sets suppress_old_event=false.
    // ---------------------------------------------------------------------

    if (suppress_frame_channel_clocks_) {

        if (trace_length_test_) {
            printf(
                "[22N CLOCK BLOCKED] "
                "apu=%llu source=%u half=%u "
                "step=%u LC=%u\n",

                (unsigned long long)apu_cycles_,
                (unsigned)source,
                (unsigned)half_frame,
                (unsigned)clock_step_,
                (unsigned)square_0.length_counter.debug_value()
            );
        }

        return;
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CV:
    //
    // Replace 23CT's timestamp comparison with a shared countdown.
    //
    // When a clock is delivered, the countdown becomes 2.
    // Another clock request while the countdown is nonzero is blocked.
    //
    // The countdown is decremented once at the end of tick(), AFTER
    // processing all frame-clock requests for that cycle.
    // ---------------------------------------------------------------------

    if (frame_clock_block_ticks_ != 0) {

        if (trace_length_test_) {
            printf(
                "[23CV CLOCK BLOCKED] "
                "apu=%llu source=%u half=%u "
                "remaining=%u LC=%u\n",

                (unsigned long long)apu_cycles_,
                (unsigned)source,
                (unsigned)half_frame,
                (unsigned)frame_clock_block_ticks_,
                (unsigned)square_0.length_counter.debug_value()
            );
        }

        return;
    }

    // Record only successfully delivered clocks.
    frame_clock_block_ticks_ = 2;

    // ---------------------------------------------------------------------
    // Record successfully delivered frame events for the debugger.
    // ---------------------------------------------------------------------

    log_frame_event(
        half_frame
            ? APU_FRAME_EVENT_QUARTER_HALF
            : APU_FRAME_EVENT_QUARTER
    );

    const char* source_name =
        source == FrameClockSource::Write4017
            ? "4017"
            : "SEQUENCER";

    const unsigned lc_before =
        square_0.length_counter.debug_value();

    if (trace_length_test_) {
        printf(
            "[FRAME CLOCK DELIVERY] "
            "apu=%llu source=%s type=%s "
            "step=%u pending=%u restart=%llu "
            "LC_before=%u\n",

            (unsigned long long)apu_cycles_,
            source_name,
            half_frame ? "QUARTER_HALF" : "QUARTER",
            (unsigned)clock_step_,
            (unsigned)frame_restart_pending_,
            (unsigned long long)frame_restart_cycle_,
            lc_before
        );
    }

    // ---------------------------------------------------------------------
    // Quarter-frame clock.
    // ---------------------------------------------------------------------

    clock_linear();

    // ---------------------------------------------------------------------
    // Half-frame clock.
    // ---------------------------------------------------------------------

    if (half_frame) {
        clock_length();
    }

    // ---------------------------------------------------------------------
    // Delivery diagnostic.
    // ---------------------------------------------------------------------

    if (trace_length_test_) {
        printf(
            "[FRAME CLOCK DELIVERED] "
            "apu=%llu source=%s type=%s "
            "LC=%u->%u\n",

            (unsigned long long)apu_cycles_,
            source_name,
            half_frame ? "QUARTER_HALF" : "QUARTER",
            lc_before,
            (unsigned)square_0.length_counter.debug_value()
        );
    }
}


static void
clock_frame_mode_0()
{
    if (trace_length_test_) {
        printf(
            "[FRAME 4] cycle=%llu step=%u "
            "next=%llu pending=%u pending_mode=%u LC=%u\n",
            (unsigned long long)apu_cycles_,
            (unsigned)clock_step_,
            (unsigned long long)next_clock_,
            (unsigned)frame_restart_pending_,
            (unsigned)frame_pending_mode_,
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    // EXPERIMENT 23CV Stage 2:
    // NTSC four-step frame-counter positions relative
    // to the sequence origin:
    //
    // Step 0:  7457  quarter
    // Step 1: 14913  quarter + half
    // Step 2: 22371  quarter
    // Step 3: 29828  IRQ
    // Step 4: 29829  quarter + half, IRQ
    // Step 5: 29830  IRQ
    //
    // Next sequence begins with step 0 at +37287.

    switch (clock_step_) {

        case 0:
            // Quarter at +7457.
            deliver_frame_clock(
                FrameClockSource::Sequencer, false);

            next_clock_ += 7456;
            break;

        case 1:
            // Quarter + half at +14913.
            deliver_frame_clock(
                FrameClockSource::Sequencer, true);

            next_clock_ += 7458;
            break;

        case 2:
            // Quarter at +22371.
            deliver_frame_clock(
                FrameClockSource::Sequencer, false);

            next_clock_ += 7457;
            break;

        case 3:
            // Frame IRQ at +29828.
            if (!frame_counter_.inhibit_frame_irq) {
                status.frame_irq = true;
            }

            ++next_clock_;
            break;

        case 4:
            // Quarter + half and IRQ at +29829.
            deliver_frame_clock(
                FrameClockSource::Sequencer, true);

            if (!frame_counter_.inhibit_frame_irq) {
                status.frame_irq = true;
            }

            ++next_clock_;
            break;

        case 5:
            // Final IRQ at +29830.
            if (!frame_counter_.inhibit_frame_irq) {
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
// Advances the APU frame counter through its five-step sequence.
//
// Quarter-frame events clock the envelopes and Triangle linear counter.
// Half-frame events additionally clock length counters and pulse sweeps.
//
// Five-step mode does not generate frame IRQs.
//
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// clock_frame_mode_1
//
// EXPERIMENT 23CY:
//
// NTSC five-step frame-counter sequence, matching the six scheduled
// event positions used by Mesen2.
//
// The immediate quarter+half clock generated by writing $4017=$80
// is delivered separately by apply_frame_restart().
//
// Five-step mode never generates frame IRQs.
// -----------------------------------------------------------------------------

void
clock_frame_mode_1()
{
    if (trace_length_test_) {
        printf(
            "[FRAME 5] cycle=%llu step=%u next=%llu "
            "pending=%u pending_mode=%u LC=%u\n",

            (unsigned long long)apu_cycles_,
            (unsigned)clock_step_,
            (unsigned long long)next_clock_,
            (unsigned)frame_restart_pending_,
            (unsigned)frame_pending_mode_,
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    switch (clock_step_) {

        case 0:
            // +7457: quarter-frame.

            deliver_frame_clock(
                FrameClockSource::Sequencer,
                false
            );

            next_clock_ += 7456;
            break;

        case 1:
            // +14913: quarter + half.

            deliver_frame_clock(
                FrameClockSource::Sequencer,
                true
            );

            next_clock_ += 7458;
            break;

        case 2:
            // +22371: quarter-frame.

            deliver_frame_clock(
                FrameClockSource::Sequencer,
                false
            );

            next_clock_ += 7458;
            break;

        case 3:
            // +29829: no channel clocks.

            next_clock_ += 7452;
            break;

        case 4:
            // +37281: quarter + half.

            deliver_frame_clock(
                FrameClockSource::Sequencer,
                true
            );

            next_clock_ += 1;
            break;

        case 5:
            // +37282: no channel clocks.
            // Next event is step 0 of the following sequence.

            next_clock_ += 7457;
            break;
    }

    // Both frame-counter modes now have six scheduled positions.
    clock_step_ = (clock_step_ + 1) % 6;
}


// -----------------------------------------------------------------------------
// apply_frame_restart
//
// Applies a pending APU frame-counter restart after the three- or four-cycle
// delay following a write to $4017.
//
// The requested frame-counter mode becomes active while the IRQ inhibition
// flag is preserved.
//
// In five-step mode, the restart immediately clocks both quarter-frame and
// half-frame units, then schedules the next periodic quarter-frame event.
//
// In four-step mode, no channel units are clocked at restart. The first
// quarter-frame event is scheduled for the next sequence interval.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------






static void
apply_frame_restart()
{
    // ---------------------------------------------------------------------
    // EXPERIMENT 23CB:
    // Record restart phase before modifying state.
    // ---------------------------------------------------------------------

    if (trace_length_test_) {
        printf(
            "[23CB RESTART PHASE] "
            "cpu=%llu apu=%llu write=%llu "
            "delta=%llu parity=%u "
            "old_mode=%u new_mode=%u "
            "step=%u LC=%u\n",

            (unsigned long long)cpu::cycle_count(),
            (unsigned long long)apu_cycles_,
            (unsigned long long)frame_last_write_cycle_,
            (unsigned long long)(
                apu_cycles_ - frame_last_write_cycle_
            ),
            (unsigned)(apu_cycles_ & 1),
            (unsigned)frame_counter_.mode,
            (unsigned)frame_pending_mode_,
            (unsigned)clock_step_,
            (unsigned)square_0.length_counter.debug_value()
        );
    }


    frame_sequence_start_cycle_ = apu_cycles_;

    frame_restart_pending_ = false;

    // Preserve the immediately applied IRQ-inhibit bit.
    frame_counter_.raw =
        (frame_counter_.raw & 0x40)
        | (frame_pending_mode_ ? 0x80 : 0x00);

    clock_step_ = 0;

    if (frame_counter_.mode) {

        // -------------------------------------------------------------
		// EXPERIMENT 23CY:
		// Five-step frame-counter restart.
		//
		// Attempt the write-generated quarter + half-frame clock
		// at the delayed restart cycle.
		//
		// The shared 23CV blocker may suppress this clock if an
		// ordinary frame clock was delivered recently.
		//
		// Then initialize the six-position five-step sequence.
		// -------------------------------------------------------------

        if (!frame_write_clock_delivered_) {

            const unsigned lc_before =
                square_0.length_counter.debug_value();

            deliver_frame_clock(
                FrameClockSource::Write4017,
                true
            );

            frame_write_clock_delivered_ = true;

            if (trace_length_test_) {
                printf(
                    "[23CI CLOCK FALLBACK] "
                    "apu=%llu LC=%u->%u\n",

                    (unsigned long long)apu_cycles_,
                    lc_before,
                    (unsigned)
                        square_0.length_counter.debug_value()
                );
            }
        }

        // EXPERIMENT 23CY:
		// Begin at the first scheduled five-step event,
		// which occurs 7457 cycles after the restart.

		clock_step_ = 0;
		next_clock_ = apu_cycles_ + 7457;
    } else {

        // -------------------------------------------------------------
        // Four-step mode.
        //
        // No write-generated quarter/half-frame clock.
        // -------------------------------------------------------------

        clock_step_ = 0;
        next_clock_ = apu_cycles_ + 7457;
    }

    // ---------------------------------------------------------------------
    // Existing frame-restart diagnostic.
    // ---------------------------------------------------------------------

    if (trace_length_test_) {
        printf(
            "[FRAME RESTART] cycle=%llu mode=%u "
            "step=%u next=%llu LC=%u\n",

            (unsigned long long)apu_cycles_,
            (unsigned)frame_counter_.mode,
            (unsigned)clock_step_,
            (unsigned long long)next_clock_,
            (unsigned)square_0.length_counter.debug_value()
        );
    }
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


// -------------------------------------------------------------------------
// nes::apu::finalize_audio_reset
//
// Initializes the FIR resampler and DC-blocking filter from the final
// combined native APU and cartridge expansion-audio output.
//
// Called after both the APU and cartridge mapper have completed reset.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------------------
void
finalize_audio_reset()
{
	const double initialAudioLevel = mix_channels_raw();
	/*
	 * Initialize the FIR history from the current mixed level,
	 * avoiding an artificial step when sample generation begins.
	 */
	audio_resampler.reset(initialAudioLevel);

	/*
	 * Initialize the DC blocker at the same level.
	 *
	 * A constant input should initially produce silence
	 * after DC blocking.
	 */
	sDCBlockPreviousInput = initialAudioLevel;
	sDCBlockPreviousOutput = 0.0;
}


// -------------------------------------------------------------------------
// nes::apu::reset
//
// Resets APU timing, channel state, sample-output state, queued host
// samples, and debugger state.
//
// A hard reset also discards outstanding DMC DMA bookkeeping and resets
// the Triangle linear counter.
//
// The FIR resampler and DC-blocking filter are initialized separately by
// finalize_audio_reset(), after the cartridge mapper has been reset.
// This ensures their initial state reflects the final combined native
// APU and expansion-audio output.
//
// Parameters:
//   type - Hard or soft reset mode.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------------------
void
reset (reset_type type)
{
	status.raw     = 0;
	frame_counter_ = {0};
	apu_cycles_    = 0;
	
	
	last_half_frame_clock_cycle_ =
    static_cast<uint64_t>(-1);
    
    frame_last_write_cycle_ =
    static_cast<uint64_t>(-1);
	
	last_delivered_frame_clock_cycle_ =
    static_cast<uint64_t>(-1);
	
	
	// EXPERIMENT 23Y: Trace the length-counter collision.
	trace_length_test_ = true;
	trace_length_reads_ = 0;
	
	
	
	frame_clock_block_ticks_ = 0;

	
	
	
	// No ordinary frame event is scheduled until the delayed restart.
	next_clock_    = static_cast<uint64_t>(-1);

	clock_step_    = 0;

	frame_restart_pending_ = false;
	frame_restart_cycle_ = 0;
	frame_pending_mode_ = false;
	
	// Clear any outstanding channel-clock suppression.
	suppress_frame_channel_clocks_ = false;
	

	/*
	 * Discard any PCM generated before the reset. Otherwise the next
	 * host audio block could contain stale pre-reset samples followed
	 * by newly generated post-reset samples.
	 */
	sample_buffer_start = 0;
	sample_buffer_end   = 0;

	sDCBlockPreviousInput  = 0.0;
	sDCBlockPreviousOutput = 0.0;

	debug_clear_scope();
	debug_clear_frame_events();

	if (type == reset_type::hard) {
    	last_frame_counter_ = 0;

    	// Initialize the DMC after CPU DMA cancellation.
    	dmc.reset_for_power_on();

    	// Initialize the Noise LFSR for a fresh emulation session.
    	noise.reset_lfsr();
	}

	/*
	 * Reset the Triangle timer and waveform state.
	 */
	 triangle.reset();

	/*
	 * Initialize Triangle debugger mute state from the channel's
	 * current hardware DAC level.
	 */
	sTriangleWasMuted = triangle.debug_muted();
	sTriangleMuteHeldDAC = static_cast<double>(triangle.raw_output());
	sTriangleMuteGain = sTriangleWasMuted ? 0.0 : 1.0;

	/*
	 * Initialize DMC debugger mute state from the channel's
	 * current hardware DAC level.
	 */
	sDmcWasMuted = dmc.debug_muted();
	sDmcMuteHeldDAC = static_cast<double>(dmc.raw_output());
	sDmcMuteGain = sDmcWasMuted ? 0.0 : 1.0;

	write4017(last_frame_counter_);

	/*
	 * Establish the initial frame-sequencer phase during hard reset.
	 *
	 * The CPU's seven-cycle reset sequence executes after APU
	 * initialization. On power-up, the frame sequencer is already
	 * running before the CPU begins executing the first ROM instruction.
	 *
	 * Apply the initial restart immediately rather than waiting for
	 * the normal $4017 write delay. This preserves the expected
	 * power-on frame-sequencer timing.
	 *
	 * Soft resets retain the normal delayed-restart behavior.
	 */
	if (type == reset_type::hard) {
		apply_frame_restart();
	}

	write4015(0x00);

	/*
	 * Pulse channel 1.
	 */
	write4000(10);
	write4001(00);
	write4002(00);
	write4003(00);

	/*
	 * Pulse channel 2.
	 */
	write4004(10);
	write4005(00);
	write4006(00);
	write4007(00);

	/*
	 * Triangle channel.
	 *
	 * Preserve the existing handling of $4008.
	 */

	// write4008(10); // Triangle is unaffected.
	write400A(00);
	write400B(00);

	/*
	 * Noise channel.
	 */
	write400C(10);
	write400E(00);
	write400F(00);

	/*
	 * DMC channel.
	 */
	write4010(10);

	/*
	 * Preserve the existing startup timing adjustment.
	 *
	 * The original implementation accounts for approximately
	 * nine APU cycles during reset, seven of which correspond
	 * to the CPU reset sequence.
	 *
	 * Execute two additional APU cycles for a hard reset.
	 */
	if (type == reset_type::hard) {
		exec<2>();
	}

	/*
	 * Do not initialize the audio resampler or DC blocker here.
	 *
	 * nes::reset() calls apu::finalize_audio_reset() after the
	 * cartridge mapper has finished resetting.
	 */
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
	
	if (trace_length_test_) {
    	printf("[APU TEST] $4003=%02X cycle=%llu LC=%u\n",
        	value,
        	(unsigned long long)apu_cycles_,
        	(unsigned)square_0.length_counter.debug_value());
	}
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
write4015(uint8_t value)
{
    log_write(0x4015, value);

    explorer_status_ = value;


    // -------------------------------------------------------------------------
    // Original $4015 behavior — unchanged.
    // -------------------------------------------------------------------------

    // Writing to this register clears the DMC interrupt flag.
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
// The returned value reports active channel length/sample state together
// with the DMC and frame interrupt flags.
//
// Reading this register clears the frame IRQ flag and clears the CPU APU
// IRQ source when no APU interrupt remains active.
//
//
// Parameters:
//   None.
//
// Returns:
//   Current APU status-register value.
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// read4015
//
// Reads the APU status register.
//
// The returned value reports active channel length/sample state together
// with the DMC and frame interrupt flags.
//
// Reading this register clears the frame IRQ flag and clears the CPU APU
// IRQ source when no APU interrupt remains active.
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
    uint8_t ret =
        status.raw & (apu_status::DMC_IRQ | apu_status::FRAME_IRQ);

    // Reading $4015 clears the frame interrupt flag.
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

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CH:
    //
    // Trace the early $4015 reads that determine the BNE branch timing
    // in Test 11, plus the final $4015 status reads.
    //
    // Diagnostic only. No extra length-counter value() calls and no
    // changes to the returned register value.
    // ---------------------------------------------------------------------

    if (trace_length_test_ &&
        ((apu_cycles_ >= 87800  && apu_cycles_ <= 87830)  ||
         (apu_cycles_ >= 117720 && apu_cycles_ <= 117770) ||
         (apu_cycles_ >= 147605 && apu_cycles_ <= 147635) ||
         (apu_cycles_ >= 177530 && apu_cycles_ <= 177580))) {

        printf(
            "[23CH 4015 READ] "
            "cpu=%llu apu=%llu ret=%02X "
            "mode=%u step=%u next=%llu "
            "pending=%u restart=%llu "
            "irq=%u LC=%u\n",

            (unsigned long long)cpu::cycle_count(),
            (unsigned long long)apu_cycles_,
            (unsigned)ret,

            (unsigned)frame_counter_.mode,
            (unsigned)clock_step_,
            (unsigned long long)next_clock_,

            (unsigned)frame_restart_pending_,
            (unsigned long long)frame_restart_cycle_,

            (unsigned)((ret & apu_status::FRAME_IRQ) != 0),
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    return ret;
}


// -----------------------------------------------------------------------------
// write4017
//
// Writes the APU frame-counter control register.
//
// Frame IRQ inhibition takes effect immediately. The requested sequence
// mode and frame-counter restart take effect after a delayed CPU-cycle
// interval.
//
// Until that restart, the currently active frame sequence continues.
// A subsequent $4017 write replaces any pending restart request.
//
// Parameters:
//   value - Value written to APU register $4017.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------




void
write4017(uint8_t value)
{
    log_write(0x4017, value);

    // ---------------------------------------------------------------------
    // Complete a frame restart that is due on this CPU cycle
    // before a new $4017 write can replace it.
    //
    // The CPU executes before the APU tick, so back-to-back
    // writes can otherwise overwrite a restart scheduled
    // for the current cycle.
    // ---------------------------------------------------------------------

    if (frame_restart_pending_
        && frame_restart_cycle_ == apu_cycles_) {

        apply_frame_restart();
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23BZ:
    //
    // Record the frame-sequencer phase at the moment of the write,
    // BEFORE modifying frame-counter state.
    //
    // Diagnostic only. No emulation state is changed.
    // ---------------------------------------------------------------------

    if (trace_length_test_) {

        const uint64_t elapsed =
            apu_cycles_ >= frame_sequence_start_cycle_
                ? apu_cycles_ - frame_sequence_start_cycle_
                : 0;

        printf(
            "[23BZ WRITE PHASE] "
            "cpu=%llu apu=%llu "
            "value=%02X "
            "active=%u requested=%u "
            "step=%u elapsed=%llu next=%llu "
            "pending=%u restart_at=%llu "
            "irq=%u inhibit=%u LC=%u\n",

            (unsigned long long)cpu::cycle_count(),
            (unsigned long long)apu_cycles_,

            (unsigned)value,

            (unsigned)frame_counter_.mode,
            (unsigned)((value & 0x80) != 0),

            (unsigned)clock_step_,
            (unsigned long long)elapsed,
            (unsigned long long)next_clock_,

            (unsigned)frame_restart_pending_,
            (unsigned long long)frame_restart_cycle_,

            (unsigned)status.frame_irq,
            (unsigned)frame_counter_.inhibit_frame_irq,

            (unsigned)
                square_0.length_counter.debug_value()
        );
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 20:
    //
    // Record the exact APU cycle of the $4017 write.
    //
    // This allows tick() to distinguish an old frame event
    // coincident with the write from one occurring afterward.
    //
    // No additional clocking or delay is introduced here.
    // ---------------------------------------------------------------------

    frame_last_write_cycle_ = apu_cycles_;

    last_frame_counter_ = value;

    // ---------------------------------------------------------------------
    // IRQ inhibition takes effect immediately.
    //
    // The active frame-counter mode remains unchanged until
    // the delayed restart.
    // ---------------------------------------------------------------------

    frame_counter_.raw =
        (frame_counter_.raw & 0x80) | (value & 0x40);

    if (frame_counter_.inhibit_frame_irq) {

        status.frame_irq = false;

        if (!status.irq_firing) {
            cpu::clear_irq(cpu::APU_IRQ);
        }
    }

    // ---------------------------------------------------------------------
    // Save the requested frame-counter mode.
    // ---------------------------------------------------------------------

    frame_pending_mode_ = (value & 0x80) != 0;
	frame_restart_pending_ = true;

	// EXPERIMENT 23CI:
	// Every new write replaces the pending clock request.
	frame_write_clock_delivered_ = false;
    
    // ---------------------------------------------------------------------
    // Preserve the previously tested restart timing:
    //
    // Even APU cycle: 3-cycle delay.
    // Odd APU cycle:  4-cycle delay.
    //
    // This alignment previously passed cpu_interrupts_v2,
    // including 4-irq_and_dma.nes.
    // ---------------------------------------------------------------------

    // EXPERIMENT 23CW:
	// Determine the delayed $4017 restart from CPU cycle parity.

	const uint64_t delay =
    	(cpu::cycle_count() & 1) ? 4 : 3;
    	
    // EXPERIMENT 23DC:
	// Test whether the current CPU/APU synchronization places
	// the delayed restart one APU tick too late.

	frame_restart_cycle_ = apu_cycles_ + delay - 1;
    	
    	
    if (trace_length_test_) {
    	printf(
        	"[23CX WRITE ALIGN] "
        	"cpu=%llu apu=%llu "
        	"value=%02X cpu_parity=%u apu_parity=%u "
        	"delay=%llu restart=%llu "
        	"origin=%llu elapsed=%llu\n",

        	(unsigned long long)cpu::cycle_count(),
        	(unsigned long long)apu_cycles_,
        	(unsigned)value,
        	(unsigned)(cpu::cycle_count() & 1),
        	(unsigned)(apu_cycles_ & 1),
        	(unsigned long long)delay,
        	(unsigned long long)frame_restart_cycle_,
        	(unsigned long long)frame_sequence_start_cycle_,
        	(unsigned long long)(
            	apu_cycles_ - frame_sequence_start_cycle_
        	)
    	);
	}	
   
    	

    

    // ---------------------------------------------------------------------
    // Existing CPU/APU write diagnostic.
    // ---------------------------------------------------------------------

    if (trace_length_test_) {

        const uint64_t cpu_cycle = cpu::cycle_count();
        const uint64_t apu_cycle = apu_cycles_;

        const int64_t cycle_difference =
            static_cast<int64_t>(cpu_cycle)
            - static_cast<int64_t>(apu_cycle);

        printf(
            "[CPU/APU WRITE] "
            "cpu=%llu apu=%llu difference=%lld "
            "value=%02X parity=%llu "
            "next=%llu step=%u "
            "active_mode=%u pending_mode=%u "
            "restart=%llu delay=%llu LC=%u\n",

            static_cast<unsigned long long>(cpu_cycle),
            static_cast<unsigned long long>(apu_cycle),
            static_cast<long long>(cycle_difference),

            static_cast<unsigned>(value),
            static_cast<unsigned long long>(apu_cycles_ & 1),

            static_cast<unsigned long long>(next_clock_),
            static_cast<unsigned>(clock_step_),

            static_cast<unsigned>(frame_counter_.mode),
            static_cast<unsigned>(frame_pending_mode_),

            static_cast<unsigned long long>(frame_restart_cycle_),
            static_cast<unsigned long long>(delay),

            static_cast<unsigned>(
                square_0.length_counter.debug_value())
        );
    }

    // ---------------------------------------------------------------------
    // Existing $4017 initialization/restart diagnostic.
    // ---------------------------------------------------------------------

    static unsigned frame_write_trace_count = 0;

    if (frame_write_trace_count < 40) {

        printf(
            "[4017 INIT TRACE] "
            "cpu=%llu apu=%llu value=%02X "
            "active_mode=%u pending_mode=%u "
            "step=%u next=%llu "
            "restart=%llu delay=%llu "
            "irq=%u inhibit=%u\n",

            static_cast<unsigned long long>(
                cpu::cycle_count()),

            static_cast<unsigned long long>(
                apu_cycles_),

            static_cast<unsigned>(value),

            static_cast<unsigned>(
                frame_counter_.mode),

            static_cast<unsigned>(
                frame_pending_mode_),

            static_cast<unsigned>(clock_step_),

            static_cast<unsigned long long>(
                next_clock_),

            static_cast<unsigned long long>(
                frame_restart_cycle_),

            static_cast<unsigned long long>(
                frame_restart_cycle_ - apu_cycles_),

            static_cast<unsigned>(
                status.frame_irq),

            static_cast<unsigned>(
                frame_counter_.inhibit_frame_irq)
        );

        ++frame_write_trace_count;
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
// Triangle and DMC debugger muting use approximately 20-millisecond fades.
// When muting begins, the current effective DAC contribution is captured
// and held as a constant reference while the live waveform fades out.
//
// Both gains are advanced once per emulated CPU cycle, independently of
// debugger refreshes and host audio sample generation.
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






// -----------------------------------------------------------------------------
// nes::apu::tick
//
// Advances the APU by one emulated CPU cycle.
//
// EXPERIMENT 23CI:
// Separate the five-step $4017-generated quarter/half-frame clock
// from the delayed frame-divider restart.
//
// The write-generated clock occurs one CPU cycle before the restart.
// The restart itself installs the new sequence and schedules its
// next ordinary frame event.
//
// Preserve the 23BN old-sequencer suppression behavior.
//
// All other APU channel, audio, and debugger behavior is unchanged.
// -----------------------------------------------------------------------------

void
tick()
{
    // ---------------------------------------------------------------------
    // Handle pending frame-counter interrupts.
    // ---------------------------------------------------------------------

    if (!frame_counter_.inhibit_frame_irq && status.frame_irq) {
        cpu::irq(cpu::APU_IRQ);
    }

    // ---------------------------------------------------------------------
    // Existing restart-collision diagnostic.
    // ---------------------------------------------------------------------

    if (trace_length_test_ &&
        frame_restart_pending_ &&
        (apu_cycles_ == next_clock_ ||
         apu_cycles_ == frame_restart_cycle_)) {

        const bool event_due =
            apu_cycles_ == next_clock_;

        const bool restart_due =
            apu_cycles_ == frame_restart_cycle_;

        printf(
            "[APU RESTART COLLISION] "
            "cpu=%llu apu=%llu "
            "event=%u restart=%u "
            "active=%u requested=%u "
            "step=%u next=%llu restart_at=%llu "
            "inhibit=%u irq=%u LC=%u\n",

            (unsigned long long)cpu::cycle_count(),
            (unsigned long long)apu_cycles_,

            (unsigned)event_due,
            (unsigned)restart_due,

            (unsigned)frame_counter_.mode,
            (unsigned)frame_pending_mode_,

            (unsigned)clock_step_,
            (unsigned long long)next_clock_,
            (unsigned long long)frame_restart_cycle_,

            (unsigned)frame_counter_.inhibit_frame_irq,
            (unsigned)status.frame_irq,
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    // ---------------------------------------------------------------------
    // Existing frame-transition diagnostic.
    // ---------------------------------------------------------------------

    if (frame_restart_pending_ &&
        frame_pending_mode_ &&
        !frame_counter_.mode &&
        apu_cycles_ >= frame_last_write_cycle_ &&
        apu_cycles_ <= frame_restart_cycle_) {

        printf(
            "[FRAME TRANSITION DETAIL] "
            "cpu=%llu apu=%llu "
            "write=%llu elapsed=%llu "
            "parity=%u "
            "step=%u next=%llu "
            "restart=%llu "
            "irq=%u LC=%u\n",

            (unsigned long long)cpu::cycle_count(),
            (unsigned long long)apu_cycles_,

            (unsigned long long)frame_last_write_cycle_,

            (unsigned long long)(
                apu_cycles_ - frame_last_write_cycle_
            ),

            (unsigned)(apu_cycles_ & 1),

            (unsigned)clock_step_,
            (unsigned long long)next_clock_,
            (unsigned long long)frame_restart_cycle_,

            (unsigned)status.frame_irq,
            (unsigned)square_0.length_counter.debug_value()
        );
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CU / 23CV:
    // The original 23CI early write-generated clock is disabled.
    // apply_frame_restart() delivers it on the restart cycle instead.
    // ---------------------------------------------------------------------

    // EXPERIMENT 23CU: keep disabled until experiment cleanup.
    if (false && frame_restart_pending_ &&
        frame_pending_mode_ &&
        !frame_write_clock_delivered_ &&
        frame_restart_cycle_ > 0 &&
        apu_cycles_ == frame_restart_cycle_ - 1) {
        const unsigned lc_before =
            square_0.length_counter.debug_value();

        // Deliver the write-generated clock independently of
        // old-sequencer channel-clock suppression.

        deliver_frame_clock(
            FrameClockSource::Write4017,
            true
        );

        frame_write_clock_delivered_ = true;

        if (trace_length_test_) {
            printf(
                "[23CI PRE-RESTART CLOCK] "
                "cpu=%llu apu=%llu "
                "restart=%llu LC=%u->%u\n",

                (unsigned long long)cpu::cycle_count(),
                (unsigned long long)apu_cycles_,
                (unsigned long long)frame_restart_cycle_,

                lc_before,
                (unsigned)square_0.length_counter.debug_value()
            );
        }
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CV Stage 2:
    // Process an ordinary sequencer event BEFORE an equally timed restart.
    // These two conditions are deliberately independent.
    // ---------------------------------------------------------------------

    if (apu_cycles_ == next_clock_) {

        // -------------------------------------------------------------
        // EXPERIMENT 23BN:
        //
        // During a pending five-step restart, the old frame
        // sequencer continues advancing and generating IRQs,
        // but its channel clocks are suppressed.
        //
        // EXPERIMENT 23CI retains this behavior for now.
        // -------------------------------------------------------------
        // ---------------------------------------------------------------------
        // EXPERIMENT 23CV Stage 1:
        // Do not suppress old-sequencer channel clocks according to the
        // write timestamp. Arbitration occurs in deliver_frame_clock().
        // ---------------------------------------------------------------------

        const bool suppress_old_event = false;
        
        // Retained for existing diagnostics.
        const bool allow_transition_event = false;

        // -------------------------------------------------------------
        // Existing transition-event diagnostic.
        // -------------------------------------------------------------

        if (frame_restart_pending_ && frame_pending_mode_) {
            printf(
                "[EXP21 FRAME EVENT] "
                "apu=%llu write=%llu "
                "step=%u active=%u requested=%u "
                "allow=%u suppress=%u LC=%u\n",

                (unsigned long long)apu_cycles_,
                (unsigned long long)frame_last_write_cycle_,

                (unsigned)clock_step_,
                (unsigned)frame_counter_.mode,
                (unsigned)frame_pending_mode_,

                (unsigned)allow_transition_event,
                (unsigned)suppress_old_event,

                (unsigned)square_0.length_counter.debug_value()
            );
        }

        // -------------------------------------------------------------
        // Existing frame-phase diagnostic.
        // -------------------------------------------------------------

        if (trace_length_test_) {
            printf(
                "[FRAME PHASE] cycle=%llu "
                "elapsed=%llu elapsed_mod2=%u "
                "step=%u next=%llu\n",

                (unsigned long long)apu_cycles_,

                (unsigned long long)(
                    apu_cycles_ - frame_sequence_start_cycle_
                ),

                (unsigned)(
                    (apu_cycles_ - frame_sequence_start_cycle_) & 1
                ),

                (unsigned)clock_step_,
                (unsigned long long)next_clock_
            );
        }

        // -------------------------------------------------------------
        // Existing suppressed-event diagnostic.
        // -------------------------------------------------------------

        if (suppress_old_event && trace_length_test_) {
            printf(
                "[FRAME SUPPRESSED] cycle=%llu "
                "step=%u active_mode=%u pending_mode=%u "
                "restart=%llu LC=%u\n",

                (unsigned long long)apu_cycles_,
                (unsigned)clock_step_,

                (unsigned)frame_counter_.mode,
                (unsigned)frame_pending_mode_,

                (unsigned long long)frame_restart_cycle_,

                (unsigned)square_0.length_counter.debug_value()
            );
        }

        // -------------------------------------------------------------
        // EXPERIMENT 22N / 23BN:
        //
        // Advance the sequencer even when channel-clock delivery
        // is suppressed.
        //
        // This preserves IRQ generation and frame-event scheduling.
        // -------------------------------------------------------------

        const unsigned lc_before =
            square_0.length_counter.debug_value();

        const unsigned step_before =
            clock_step_;

        const bool irq_before =
            status.frame_irq;

        const bool mode_before =
            frame_counter_.mode;

        suppress_frame_channel_clocks_ =
            suppress_old_event;

        if (frame_counter_.mode) {
            clock_frame_mode_1();
        } else {
            clock_frame_mode_0();
        }

        suppress_frame_channel_clocks_ = false;

        if (trace_length_test_) {
            printf(
                "[22N FRAME EVENT] apu=%llu "
                "mode=%u step=%u suppressed=%u "
                "LC=%u->%u IRQ=%u->%u "
                "next_step=%u next=%llu\n",

                (unsigned long long)apu_cycles_,
                (unsigned)mode_before,
                step_before,
                (unsigned)suppress_old_event,

                lc_before,
                (unsigned)square_0.length_counter.debug_value(),

                (unsigned)irq_before,
                (unsigned)status.frame_irq,

                (unsigned)clock_step_,
                (unsigned long long)next_clock_
            );
        }
    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CV Stage 2:
    // Apply delayed $4017 restart AFTER the ordinary sequencer event.
    // This permits the ordinary event to set frame_clock_block_ticks_
    // before the write-generated clock is requested.
    // ---------------------------------------------------------------------

    if (frame_restart_pending_ &&
        apu_cycles_ == frame_restart_cycle_) {
        const unsigned lc_before =
            square_0.length_counter.debug_value();

        const unsigned step_before =
            clock_step_;

        const bool old_mode =
            frame_counter_.mode;

        // -------------------------------------------------------------
        // EXPERIMENT 23CI:
        //
        // With 23CU enabled, the early write-generated clock is disabled.
        // apply_frame_restart() delivers the five-step write clock.
        // -------------------------------------------------------------

        apply_frame_restart();

        if (trace_length_test_) {
            printf(
                "[FRAME RESTART APPLIED] "
                "cpu=%llu apu=%llu "
                "old_mode=%u new_mode=%u "
                "old_step=%u new_step=%u "
                "LC=%u->%u next=%llu\n",

                (unsigned long long)cpu::cycle_count(),
                (unsigned long long)apu_cycles_,

                (unsigned)old_mode,
                (unsigned)frame_counter_.mode,

                step_before,
                (unsigned)clock_step_,

                lc_before,
                (unsigned)square_0.length_counter.debug_value(),

                (unsigned long long)next_clock_
            );
        }

    }

    // ---------------------------------------------------------------------
    // EXPERIMENT 23CV Stage 2:
    // Decrement the shared frame-clock blocker exactly once per APU cycle,
    // after all frame-event/restart processing (including empty cycles).
    // ---------------------------------------------------------------------

    if (frame_clock_block_ticks_ > 0) {
        --frame_clock_block_ticks_;
    }

    // ---------------------------------------------------------------------
    // Advance Triangle debugger mute smoothing.
    // ---------------------------------------------------------------------

    const bool triangleMuted = triangle.debug_muted();

    if (triangleMuted && !sTriangleWasMuted) {
        const double rawDAC =
            static_cast<double>(triangle.raw_output());

        sTriangleMuteHeldDAC += sTriangleMuteGain *
            (rawDAC - sTriangleMuteHeldDAC);
    }

    sTriangleWasMuted = triangleMuted;

    if (triangleMuted) {
        sTriangleMuteGain = std::max(
            0.0,
            sTriangleMuteGain - kTriangleMuteGainStep
        );
    } else {
        sTriangleMuteGain = std::min(
            1.0,
            sTriangleMuteGain + kTriangleMuteGainStep
        );
    }

    // ---------------------------------------------------------------------
    // Advance DMC debugger mute smoothing.
    // ---------------------------------------------------------------------

    const bool dmcMuted = dmc.debug_muted();

    if (dmcMuted && !sDmcWasMuted) {
        const double rawDAC =
            static_cast<double>(dmc.raw_output());

        sDmcMuteHeldDAC += sDmcMuteGain *
            (rawDAC - sDmcMuteHeldDAC);
    }

    sDmcWasMuted = dmcMuted;

    if (dmcMuted) {
        sDmcMuteGain = std::max(
            0.0,
            sDmcMuteGain - kDmcMuteGainStep
        );
    } else {
        sDmcMuteGain = std::min(
            1.0,
            sDmcMuteGain + kDmcMuteGainStep
        );
    }

    // ---------------------------------------------------------------------
    // Feed instantaneous mixed output into the FIR resampler.
    // ---------------------------------------------------------------------

    double filteredSample = 0.0;

    if (audio_resampler.clock(
            mix_channels_raw(), filteredSample)) {



        // Apply DC blocking and convert to unsigned 8-bit PCM.
        const uint8_t mixedSample =
            mix_channels(filteredSample);

        // Capture the generated sample for the debugger.
        capture_scope_sample(mixedSample);

        // Apply debugger-controlled host audio muting.
        const uint8_t outputSample =
            debug_audio_is_muted() ? kSilence : mixedSample;

        // Queue PCM with circular-buffer overflow protection.
        const size_t nextWrite =
            (sample_buffer_end + 1) % kBufferSize;

        if (nextWrite == sample_buffer_start) {
            sample_buffer_start =
                (sample_buffer_start + 1) % kBufferSize;
        }

        sample_buffer_[sample_buffer_end] = outputSample;
        sample_buffer_end = nextWrite;
    }

    // ---------------------------------------------------------------------
    // Advance native APU channels in the established order.
    // ---------------------------------------------------------------------

    dmc.tick();
    noise.tick();
    triangle.tick();
    square_0.tick();
    square_1.tick();

    // ---------------------------------------------------------------------
    // Advance the global APU cycle counter.
    // ---------------------------------------------------------------------

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
// Copies available mixed APU samples into the supplied host audio buffer.
//
// Under normal operation, only samples actually generated by the APU are
// returned. If fewer samples are available than requested, the function
// returns the number copied without padding the remaining buffer.
//
// The host-side audio staging code is responsible for accumulating samples
// and submitting complete playback blocks.
//
// When debugger-controlled host audio mute is active, the requested buffer
// is filled with unsigned 8-bit PCM silence instead. Any queued samples
// accumulated while muted are discarded.
//
// Parameters:
//   buffer - Destination PCM buffer.
//   size   - Maximum number of samples to copy.
//
// Returns:
//   Number of samples written to the destination buffer.
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
// which applies the nonlinear native NES APU mixing equations.
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
log_frame_event (apu_frame_event_type type)
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
write_log_snapshot (write_log_entry_t *entries, uint32_t capacity)
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

