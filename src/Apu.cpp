
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
	BitField<uint8_t, 6> inihibit_frame_irq;
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


void clock_linear() {
	triangle.linear_counter.clock();

	square_0.envelope.clock();
	square_1.envelope.clock();
	noise.envelope.clock();
}


void clock_length() {
	square_0.length_counter.clock();
	square_1.length_counter.clock();
	triangle.length_counter.clock();
	noise.length_counter.clock();

	square_0.sweep.clock();
	square_1.sweep.clock();
}


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
		if (!(frame_counter_.inihibit_frame_irq)) {
			status.frame_irq = true;
		}

		++next_clock_;
		break;

	case 4:
		clock_linear();
		clock_length();
		if (!(frame_counter_.inihibit_frame_irq)) {
			status.frame_irq = true;
		}

		++next_clock_;
		break;

	case 5:
		if (!(frame_counter_.inihibit_frame_irq)) {
			status.frame_irq = true;
		}

		next_clock_ += 7457;
		break;
	}

	clock_step_ = (clock_step_ + 1) % 6;
}


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
	constexpr double R = 0.995;
	const double output = input - sDCBlockPreviousInput + R * sDCBlockPreviousOutput;
	sDCBlockPreviousInput = input;
	sDCBlockPreviousOutput = output;
	const int sample = static_cast<int>(output + 128.0);

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


void write4000(uint8_t value) {
	square_0.write_reg0(value);
}



void write4001(uint8_t value) {
	square_0.write_reg1(value);
}


void write4002(uint8_t value) {
	square_0.write_reg2(value);
}


void write4003(uint8_t value) {
	square_0.write_reg3(value);
}


void write4004(uint8_t value) {
	square_1.write_reg0(value);
}


void write4005(uint8_t value) {
	square_1.write_reg1(value);
}


void write4006(uint8_t value) {
	square_1.write_reg2(value);
}


void write4007(uint8_t value) {
	square_1.write_reg3(value);
}


void write4008(uint8_t value) {
	triangle.write_reg0(value);
}


void write400A(uint8_t value) {
	triangle.write_reg2(value);
}


void write400B(uint8_t value) {
	triangle.write_reg3(value);
}


void write400C(uint8_t value) {
	noise.write_reg0(value);
}


void write400E(uint8_t value) {
	noise.write_reg2(value);
}


void write400F(uint8_t value) {
	noise.write_reg3(value);
}


void write4010(uint8_t value) {
	dmc.write_reg0(value);
}


void write4011(uint8_t value) {
	dmc.write_reg1(value);
}


void write4012(uint8_t value) {
	dmc.write_reg2(value);
}


void write4013(uint8_t value) {
	dmc.write_reg3(value);
}


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


void write4017(uint8_t value) {

	frame_counter_.raw  = value;
	last_frame_counter_ = value;

	if (frame_counter_.inihibit_frame_irq) {
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
	if (!(frame_counter_.inihibit_frame_irq) && (status.frame_irq)) {
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
	
	
	// That discarded queued samples at every video-frame boundary and was one 
	// of the causes of audio discontinuities.
}


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

