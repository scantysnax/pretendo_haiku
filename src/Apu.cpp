
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

//------------------------------------------------------------------------------
// Name: mix_channels
//------------------------------------------------------------------------------
uint8_t mix_channels() {

	int const square1_out =		square_0.output();
	int const square2_out = 	square_1.output();
	int const triangle_out = 	triangle.output();
	int const noise_out = 		noise.output();
	int const dmc_out = 		dmc.output();

#if 1
	double const square_out = 	0.00752 * (square1_out + square2_out);
	double const tnd_out = 		0.00851 * triangle_out + 
							 	0.00494 * noise_out + 
							 	0.00335 * dmc_out;
	int const output = 		(square_out + tnd_out) * 255.0;
#else
	int const output = (square1_out +
							square2_out +
							triangle_out +
							noise_out +
							dmc_out +
							0);
#endif

	return std::clamp(output, 0, 255);
}


void reset(Reset reset_type) {

	status.raw     = 0;
	frame_counter_ = {0};
	apu_cycles_    = 0;
	next_clock_    = 0;
	clock_step_    = 0;

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


void tick() {
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
		sample_buffer_[sample_buffer_end] = mix_channels();
		
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


size_t read_samples(uint8_t *buffer, size_t size) {

    size_t index = sample_buffer_start;
    size_t i = 0;
    for(; i < size && index != sample_buffer_end; ++i) {
        buffer[i] = sample_buffer_[index];
        index = (index + 1) % buffer_size;
    }

    return i;
}


void start_frame() {
	sample_buffer_start = sample_buffer_end;
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

}

