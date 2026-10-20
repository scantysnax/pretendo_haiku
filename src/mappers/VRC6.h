#ifndef _VRC6_H_
#define _VRC6_H_

#include "BitField.h"
#include "Mapper.h"


// -----------------------------------------------------------------------------
// vrc6_debug_state_t
//
// Debugger-facing snapshot of VRC6 mapper-control state.
//
// Generic resolved PRG/CHR mappings and mirroring are already exposed through
// Mapper::debug_state(), so this structure contains only VRC6-specific internal
// control state that cannot be derived reliably from those mappings.
// -----------------------------------------------------------------------------
struct vrc6_debug_state_t {
	bool wram_enabled = false;

	uint8_t irq_latch = 0;
	uint8_t irq_counter = 0;
	int irq_prescaler = 341;

	uint8_t irq_control = 0;
	bool irq_enable_after_ack = false;
	bool irq_enabled = false;
	bool irq_cycle_mode = false;

	uint8_t frequency_control = 0;
	bool audio_halted = false;
	uint16_t audio_clock_scale = 1;
	
	uint64_t irq_clock_count = 0;
	uint64_t irq_assert_count = 0;
};


// Konami VRC-6
class VRC6 : public Mapper
{
	private:
	union IRQControl {
		uint8_t raw;
		BitField<uint8_t, 0> a;
		BitField<uint8_t, 1> enabled;
		BitField<uint8_t, 2> mode;
	};

	private:
	struct PulseChannel {
		uint8_t volume = 0;
		uint8_t duty = 0;
		bool mode = false;
		bool enabled = false;

		uint16_t period = 0;
		uint16_t timer = 0;

		uint8_t step = 15;

		void write_control (uint8_t value);
		void write_period_low (uint8_t value);
		void write_period_high (uint8_t value);

		void clock (uint8_t frequency_control);
		uint8_t output() const;
	};


	// -------------------------------------------------------------------------
	// SawChannel
	//
	// Represents the VRC6 sawtooth audio channel.
	//
	// The channel uses a 12-bit timer and a 14-step sequence. On every second
	// sequencer step, the programmed 6-bit rate is added to an 8-bit accumulator.
	// After six additions, the next pair of sequencer steps begins by clearing the
	// accumulator.
	// -------------------------------------------------------------------------
	struct SawChannel {
		uint8_t rate = 0;
		bool enabled = false;

		uint16_t period = 0;
		uint16_t timer = 0;

		uint8_t accumulator = 0;
		uint8_t step = 0;

		void write_rate (uint8_t value);
		void write_period_low (uint8_t value);
		void write_period_high (uint8_t value);

		void clock (uint8_t frequency_control);

		uint8_t output() const;
	};

	public:
	VRC6();

	public:
	std::string name() const override;

	public:
	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
	void write_f (uint_least16_t address, uint8_t value) override;

	public:
	uint8_t read_6 (uint_least16_t address) override;
	uint8_t read_7 (uint_least16_t address) override;

	void write_6 (uint_least16_t address, uint8_t value) override;
	void write_7 (uint_least16_t address, uint8_t value) override;

	public:
	void cpu_sync() override;
	void reset() override;

	uint8_t audio_output() const override;
	size_t audio_channel_count() const override;
	const char *audio_channel_name (size_t channel) const override;
	bool audio_channel_muted (size_t channel) const override;
	void set_audio_channel_muted (size_t channel, bool muted) override;

	mapper_audio_debug_state_t debug_audio_state() const override;
	mapper_audio_scope_sample_t debug_audio_scope_sample() const override;

	vrc6_debug_state_t debug_mapper_state() const;

	private:
	void clock_irq();

	private:
	uint8_t wram_[0x2000] = {};
	bool wram_enabled_ = false;

	private:
	PulseChannel pulse1_;
	PulseChannel pulse2_;
	SawChannel saw_;

	private:
	uint8_t frequency_control_ = 0;
	bool audio_channel_muted_[3] = {};

	private:
	uint8_t irq_latch_      = 0;
	IRQControl irq_control_ = {0};
	uint8_t irq_counter_    = 0;
	int irq_prescaler_      = 341;
	
	private:
	uint64_t irq_clock_count_ = 0;
	uint64_t irq_assert_count_ = 0;
};

#endif
