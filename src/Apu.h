#ifndef _APU_H_
#define _APU_H_

#include "BitField.h"
#include "Reset.h"

#include <cstddef>
#include <cstdint>


namespace nes::apu {
	
constexpr int32_t kOutputFrequency = 48000;
constexpr int32_t kFrameRate = 60;
constexpr int32_t kBufferSize = (kOutputFrequency / kFrameRate) * 4;
constexpr uint8_t kSilence = 0x80;
	
	
template <int Channel>
class Square;
class Triangle;
class Noise;
class DMC;


union APUStatus {
	uint8_t raw;

	BitField<uint8_t, 0> square1_enabled;
	BitField<uint8_t, 1> square2_enabled;
	BitField<uint8_t, 2> triangle_enabled;
	BitField<uint8_t, 3> noise_enabled;
	BitField<uint8_t, 4> dmc_enabled;

	BitField<uint8_t, 6> frame_irq;
	BitField<uint8_t, 7> dmc_irq;

	// meta field
	BitField<uint8_t, 6, 2> irq_firing;
};


struct square_debug_state_t {
	bool enabled = false;
	bool muted = false;

	uint16_t timer_period = 0;
	uint16_t timer_frequency = 0;

	uint8_t duty = 0;
	uint8_t sequence_index = 0;

	uint8_t length_counter = 0;

	uint8_t envelope_volume = 0;

	bool sweep_enabled = false;
	uint8_t sweep_period = 0;
	bool sweep_negate = false;
	uint8_t sweep_shift = 0;
	bool sweep_silenced = false;

	uint8_t output = 0;
};


struct triangle_debug_state_t {
	bool enabled = false;
	bool muted = false;

	uint16_t timer_period = 0;
	uint16_t timer_frequency = 0;

	uint8_t length_counter = 0;
	uint8_t linear_counter = 0;

	uint8_t sequence_index = 0;
	uint8_t output = 0;
};


struct noise_debug_state_t {
	bool enabled = false;
	bool muted = false;

	uint16_t timer_period = 0;

	uint8_t length_counter = 0;
	uint8_t envelope_volume = 0;

	uint8_t output = 0;
};


struct dmc_debug_state_t {
	bool enabled = false;
	bool active = false;
	bool muted = false;

	uint16_t timer_period = 0;

	bool irq_enabled = false;
	bool loop = false;

	uint8_t output = 0;

	uint16_t sample_address = 0;
	uint16_t current_address = 0;
	uint16_t sample_length = 0;
	uint16_t bytes_remaining = 0;

	uint8_t bits_remaining = 0;

	bool sample_buffer_empty = true;
};


struct apu_debug_state_t {
	uint64_t cycle = 0;

	bool five_step_mode = false;
	bool frame_irq_inhibit = false;
	uint8_t frame_step = 0;

	uint64_t next_frame_cycle = 0;

	uint8_t status = 0;

	bool frame_irq = false;
	bool dmc_irq = false;

	bool audio_muted = false;

	square_debug_state_t square1;
	square_debug_state_t square2;

	triangle_debug_state_t triangle;
	noise_debug_state_t noise;
	dmc_debug_state_t dmc;
};


extern bool debug_audio_muted;

void debug_set_audio_muted(bool muted);
bool debug_audio_is_muted();

apu_debug_state_t debug_state();

constexpr uint32_t APU_WRITE_LOG_CAPACITY = 256;

struct apu_write_log_entry_t {
	uint64_t cycle;
	uint16_t address;
	uint8_t value;
	uint32_t write_index;

	uint16_t timer_period;
	bool has_timer_period;
	
	uint8_t previous_enable_mask;
	bool has_previous_enable_mask;
};

uint32_t apu_write_log_count();
uint32_t apu_write_log_snapshot(apu_write_log_entry_t *entries, uint32_t capacity);
void clear_apu_write_log();


struct apu_explorer_state_t {
	uint8_t square1[4] = {};	// $4000-$4003
	uint8_t square2[4] = {};	// $4004-$4007

	uint8_t triangle0 = 0;		// $4008
	uint8_t triangle2 = 0;		// $400A
	uint8_t triangle3 = 0;		// $400B

	uint8_t noise0 = 0;			// $400C
	uint8_t noise2 = 0;			// $400E
	uint8_t noise3 = 0;			// $400F

	uint8_t dmc[4] = {};		// $4010-$4013

	uint8_t status = 0;			// last value written to $4015
	uint8_t frame_counter = 0;	// last value writeen to $4017
};

apu_explorer_state_t explorer_state();



void reset(Reset reset_type);

void write4000(uint8_t value);
void write4001(uint8_t value);
void write4002(uint8_t value);
void write4003(uint8_t value);

void write4004(uint8_t value);
void write4005(uint8_t value);
void write4006(uint8_t value);
void write4007(uint8_t value);

void write4008(uint8_t value);
void write400A(uint8_t value);
void write400B(uint8_t value);

void write400C(uint8_t value);
void write400E(uint8_t value);
void write400F(uint8_t value);

void write4010(uint8_t value);
void write4011(uint8_t value);
void write4012(uint8_t value);
void write4013(uint8_t value);

void write4015(uint8_t value);
void write4017(uint8_t value);


uint8_t read4015();

uint64_t cycle_count();

void tick();
void start_frame();


void mute_channel(int const channel);
void unmute_channel(int const channel);


size_t read_samples(uint8_t *buffer, size_t size);


template <int Cycles>
void exec()
{
	for (int i = 0; i < Cycles; ++i) {
		tick();
	}
}


extern Square<0> square_0;
extern Square<1> square_1;

extern Triangle triangle;
extern Noise noise;
extern DMC dmc;

extern APUStatus status;


typedef enum {
	SQUARE1 = 0,
	SQUARE2 = 1,
	TRIANGLE = 2,
	NOISE = 3,
	DPCM = 4
} sound_channel;


}


#endif // _APU_H_
