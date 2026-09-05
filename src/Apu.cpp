
#include "Apu.h"
#include "Cpu.h"
#include "Dmc.h"
#include "Noise.h"
#include "Square.h"
#include "Triangle.h"

#include <algorithm>
#include <iostream>

// note: DMC and DPCM may be used interchangeably

namespace nes::apu {

bool debug_audio_muted = false;	

namespace {

typedef enum {
	ENABLE_SQUARE1 = 	0x1,
	ENABLE_SQUARE2 = 	0x2,
	ENABLE_TRIANGLE = 	0x4,
	ENABLE_NOISE = 		0x8,
	ENABLE_DMC = 		0x10,
	FRAME_IRQ = 		0x40,
	DMC_IRQ = 			0x80
} apu_status;
	

union APUFrameCounter {
	uint8_t raw;
	BitField<uint8_t, 6> inhibit_frame_irq;
	BitField<uint8_t, 7> mode;
};

constexpr double CPUFrequency = 1789772.7272; // 1.7897727272MHz
// CPUFrequency / 44100Hz  = 40.5844155828 clocks per sample
// CPUFrequency / 48000Hz  = 37.2869318167  clocks per sample
// CPUFrequency / 192000Hz = 9.32173295417 clocks per sample
constexpr auto ClocksPerSample = static_cast<int32_t>(CPUFrequency / frequency);

auto apu_cycles_               = static_cast<uint64_t>(-1);
auto next_clock_               = static_cast<uint64_t>(-1);
uint8_t clock_step_            = 0;
APUFrameCounter frame_counter_ = {0};
uint8_t last_frame_counter_    = 0;

// dc filter
double sDCBlockPreviousInput = 0.0;
double sDCBlockPreviousOutput = 0.0;

}

Square<0> square_0;
Square<1> square_1;
Triangle triangle;
Noise noise;
DMC dmc;
APUStatus status = {0};

uint8_t sample_buffer_[buffer_size];
size_t sample_buffer_start = 0;
size_t sample_buffer_end   = 0;
static uint8_t sLastOutputSample = silence;


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
void clock_linear() {
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
void clock_length() {
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
// sweeps.  Frame IRQ state is asserted at the appropriate sequence points when
// frame IRQ generation is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void clock_frame_mode_0() {

	// 4 step sequence
	switch (clock_step_) {
	case 0:
		clock_linear();
		next_clock_ += 7456;
		break;

	case 1:
		clock_linear();
		clock_length();
		next_clock_ += 7458;
		break;

	case 2:
		clock_linear();
		next_clock_ += 7457;
		break;

	case 3:
		if (!(frame_counter_.inhibit_frame_irq)) {
			status.frame_irq = true;
		}

		++next_clock_;
		break;

	case 4:
		clock_linear();
		clock_length();
		if (!(frame_counter_.inhibit_frame_irq)) {
			status.frame_irq = true;
		}

		++next_clock_;
		break;

	case 5:
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
void clock_frame_mode_1() {

	// 5 step sequence
	switch (clock_step_) {
	case 0:
		clock_linear();
		clock_length();
		next_clock_ += 7458;
		break;

	case 1:
		clock_linear();
		next_clock_ += 7456;
		break;

	case 2:
		clock_linear();
		clock_length();
		next_clock_ += 7458;
		break;

	case 3:
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
// mix_channels
//
// Mixes the five NES audio channels and removes the DC component from the
// resulting unipolar signal.
//
// The NES mixer naturally produces a positive-only output level, while unsigned
// 8-bit host PCM uses 0x80 as silence.  A simple DC-blocking high-pass filter
// converts the mixer output into a bipolar waveform centered around 0x80.
//
// Parameters:
//   None.
//
// Returns:
//   Unsigned 8-bit PCM sample centered around 0x80.
// -----------------------------------------------------------------------------
uint8_t
mix_channels()
{
	const int32_t square1Out = square_0.output();
	const int32_t square2Out = square_1.output();
	const int32_t triangleOut = triangle.output();
	const int32_t noiseOut = noise.output();
	const int32_t dmcOut = dmc.output();

	const double squareOut = 0.00752 * (square1Out + square2Out);
	const double tndOut = 0.00851 * triangleOut + 0.00494 * noiseOut + 0.00335 * dmcOut;

	const double input = (squareOut + tndOut) * 255.0;

	/*
	 * One-pole DC-blocking high-pass filter:
	 *
	 *     y[n] = x[n] - x[n-1] + R * y[n-1]
	 *
	 * This removes the DC component of the positive-only NES mixer while
	 * preserving the audible waveform.
	 */
	
	constexpr double R = 0.995;	// R = dc decay/feedback coefficient
	const double output = input - sDCBlockPreviousInput + R * sDCBlockPreviousOutput;
	sDCBlockPreviousInput = input;
	sDCBlockPreviousOutput = output;
	const int32_t sample = static_cast<int32_t>(output + 128.0);

	return static_cast<uint8_t>(std::clamp(sample, 0, 255));
}


// -----------------------------------------------------------------------------
// nes::apu::reset
//
// Resets APU timing, channel state, sample-output state, and DC-blocking filter.
//
// Parameters:
//   reset_type - Hard or soft reset mode.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
reset(Reset reset_type)
{
	status.raw     = 0;
	frame_counter_ = {0};
	apu_cycles_    = 0;
	next_clock_    = 0;
	clock_step_    = 0;

	sLastOutputSample = silence;

	sDCBlockPreviousInput = 0.0;
	sDCBlockPreviousOutput = 0.0;

	if (reset_type == Reset::Hard) {
		last_frame_counter_ = 0;
	}

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
	// cycles by the time the reset is complete. I beleive that the first 7
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
	if (reset_type == Reset::Hard) {
		exec<2>();
	}

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
void write4000(uint8_t value) {
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
void write4001(uint8_t value) {
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
void write4002(uint8_t value) {
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
void write4003(uint8_t value) {
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
void write4004(uint8_t value) {
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
void write4005(uint8_t value) {
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
void write4006(uint8_t value) {
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
void write4007(uint8_t value) {
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
void write4008(uint8_t value) {
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
void write400A(uint8_t value) {
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
void write400B(uint8_t value) {
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
void write400C(uint8_t value) {
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
void write400E(uint8_t value) {
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
void write400F(uint8_t value) {
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
void write4010(uint8_t value) {
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
void write4011(uint8_t value) {
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
void write4012(uint8_t value) {
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
void write4013(uint8_t value) {
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
void write4015(uint8_t value) {

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
uint8_t read4015() {
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
write4017(uint8_t value)
{
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
// tick
//
// Advances the APU by one APU cycle.  Frame counter timing, IRQ generation,
// channel clocks, and sample generation continue normally.  When debugger audio
// mute is active, the mixed output sample is replaced with silence before it is
// written to the host sample buffer.
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
	if (!(frame_counter_.inhibit_frame_irq) && (status.frame_irq)) {
		cpu::irq(cpu::APU_IRQ);
	}

	if (apu_cycles_ == next_clock_) {
		if (frame_counter_.mode) {
			clock_frame_mode_1();
		} else {
			clock_frame_mode_0();
		}
	}

	if ((apu_cycles_ % ClocksPerSample) == 0) {
		if (debug_audio_is_muted()) {
			sample_buffer_[sample_buffer_end] = silence;
		} else {
			sample_buffer_[sample_buffer_end] = mix_channels();
		}
		
		sample_buffer_end = (sample_buffer_end + 1) % sizeof(sample_buffer_);
	}

	dmc.tick();
	noise.tick();
	triangle.tick();
	square_0.tick();
	square_1.tick();

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
uint64_t cycle_count() {
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
read_samples(uint8_t *buffer, size_t size)
{
	if (!buffer || size == 0) {
		return 0;
	}

	if (debug_audio_is_muted()) {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = silence;
		}

		sample_buffer_start = sample_buffer_end;
		sLastOutputSample = silence;

		return size;
	}

	size_t i = 0;

	while (i < size && sample_buffer_start != sample_buffer_end) {
		const uint8_t sample = sample_buffer_[sample_buffer_start];

		buffer[i] = sample;
		sLastOutputSample = sample;

		sample_buffer_start = (sample_buffer_start + 1) % buffer_size;

		++i;
	}

	while (i < size) {
		buffer[i] = sLastOutputSample;
		++i;
	}

	return size;
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
	// we don't want to do this:
	
	// sample_buffer_start = sample_buffer_end;
	
	
	// that discards queued samples at every video-frame boundary and is one 
	// of the causes of audio discontinuities.
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
// Returns a side-effect-free snapshot of the current APU state for debugger
// views.
//
// This function intentionally avoids read4015(), because reading $4015 has
// emulation-visible side effects, including clearing the frame IRQ flag.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot of the current APU and channel state.
// -----------------------------------------------------------------------------
apu_debug_state_t
debug_state()
{
	apu_debug_state_t state;

	state.cycle = apu_cycles_;
	state.five_step_mode = frame_counter_.mode;
	state.frame_irq_inhibit = frame_counter_.inhibit_frame_irq;
	state.frame_step = clock_step_;
	state.next_frame_cycle = next_clock_;
	state.frame_irq = status.frame_irq;
	state.dmc_irq = status.dmc_irq;

	state.audio_muted = debug_audio_is_muted();

	// square 1
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

	// square 2
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

	// triangle
	state.triangle.enabled = triangle.enabled();
	state.triangle.muted = triangle.debug_muted();
	state.triangle.timer_period = triangle.debug_timer_period();
	state.triangle.timer_frequency = triangle.debug_timer_frequency();
	state.triangle.length_counter = triangle.length_counter.debug_value();
	state.triangle.linear_counter = triangle.linear_counter.value();
	state.triangle.sequence_index = triangle.debug_sequence_index();
	state.triangle.output = triangle.output();

	// noise
	state.noise.enabled = noise.enabled();
	state.noise.muted = noise.debug_muted();
	state.noise.timer_period = noise.debug_timer_period();
	state.noise.length_counter = noise.length_counter.debug_value();
	state.noise.envelope_volume = noise.envelope.debug_volume();
	state.noise.output = noise.debug_output();

	// dmc
	state.dmc.enabled = dmc.debug_enabled();
	state.dmc.active = dmc.debug_active();
	state.dmc.muted = dmc.debug_muted();
	state.dmc.timer_period = dmc.debug_timer_period();
	state.dmc.irq_enabled = dmc.debug_irq_enabled();
	state.dmc.loop = dmc.debug_loop();
	state.dmc.output = dmc.output();
	state.dmc.sample_address = dmc.debug_sample_address();
	state.dmc.current_address = dmc.debug_current_address();
	state.dmc.sample_length = dmc.debug_sample_length();
	state.dmc.bytes_remaining = dmc.bytes_remaining();
	state.dmc.bits_remaining = dmc.debug_bits_remaining();
	state.dmc.sample_buffer_empty = dmc.debug_sample_buffer_empty();
	
	// apu status (rebuilt from $4015 without reading it)
	uint8_t status = 0x0;

	if (state.square1.length_counter != 0) {
		status |= 0x1;
	}

	if (state.square2.length_counter != 0) {
		status |= 0x2;
	}

	if (state.triangle.length_counter != 0) {
		status |= 0x4;
	}

	if (state.noise.length_counter != 0) {
		status |= 0x8;
	}

	if (state.dmc.bytes_remaining != 0) {
		status |= 0x10;
	}

	if (state.frame_irq) {
		status |= 0x40;
	}

	if (state.dmc_irq) {
		status |= 0x80;
	}

	state.status = status;

	return state;
}


// -----------------------------------------------------------------------------
// nes::apu::debug_set_audio_muted
//
// Enables or disables debugger audio mute.  The queued sample buffer is flushed
// whenever the mute state changes so stale audio cannot remain frozen in the
// host output path.
//
// Parameters:
//   muted - true to mute debugger audio output.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_set_audio_muted(bool muted)
{
	debug_audio_muted = muted;

	for (size_t i = 0; i < buffer_size; i++) {
		sample_buffer_[i] = silence;
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

}

