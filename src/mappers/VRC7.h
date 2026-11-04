#ifndef _VRC7_H_
#define _VRC7_H_

#include "BitField.h"
#include "Mapper.h"


struct vrc7_fm_channel_debug_t {
	uint16_t f_number = 0;
	uint8_t block = 0;
	uint8_t instrument = 0;
	uint8_t volume = 0;

	bool key_on = false;
	bool sustain = false;

	uint8_t modulator_multiplier = 0;
	uint8_t carrier_multiplier = 0;
	uint8_t feedback = 0;
	uint8_t modulator_output_level = 0;

	uint8_t modulator_attack = 0;
	uint8_t modulator_decay = 0;
	uint8_t carrier_attack = 0;
	uint8_t carrier_decay = 0;
	
	uint32_t modulator_phase_increment = 0;
	uint32_t carrier_phase_increment = 0;
	
	uint32_t modulator_phase = 0;
	uint32_t carrier_phase = 0;
	
	int16_t modulator_output = 0;
	int16_t carrier_output = 0;
	
	uint8_t modulator_envelope_state = 0;
	uint8_t carrier_envelope_state = 0;

	uint8_t modulator_envelope_level = 127;	
	uint8_t carrier_envelope_level = 127;
};


// -----------------------------------------------------------------------------
// vrc7_debug_state_t
//
// Describes the current VRC7 mapper-specific debugger state.
//
// The snapshot exposes the raw PRG and CHR bank registers, mapper-control
// state, selected FM register, IRQ latch/counter/control state, decoded IRQ
// control bits, current prescaler value, and persistent IRQ activity counters.
//
// The activity counters are cumulative and allow the debugger to show whether
// the VRC7 IRQ system is actively clocking and asserting interrupts over time.
// -----------------------------------------------------------------------------
struct vrc7_debug_state_t {
	uint8_t prg[3] = {};
	uint8_t chr[8] = {};

	bool wram_enabled = false;
	bool audio_silenced = false;

	uint8_t audio_register = 0;

	uint8_t irq_latch = 0;
	uint8_t irq_counter = 0;
	uint8_t irq_control = 0;

	bool irq_enable_after_ack = false;
	bool irq_enabled = false;
	bool irq_cycle_mode = false;

	int irq_prescaler = 0;

	uint64_t irq_clock_count = 0;
	uint64_t irq_assert_count = 0;
	
	vrc7_fm_channel_debug_t fm_channels[6] = {};
	
	int32_t fm_output = 0;
};


class VRC7 : public Mapper
{
	private:
	union IRQControl {
		uint8_t raw;
		BitField<uint8_t, 0> a;
		BitField<uint8_t, 1> enabled;
		BitField<uint8_t, 2> mode;
	};

	public:
	VRC7();

	public:
	std::string name() const override;

	public:
	uint8_t read_6 (uint_least16_t address) override;
	uint8_t read_7 (uint_least16_t address) override;

	void write_6 (uint_least16_t address, uint8_t value) override;
	void write_7 (uint_least16_t address, uint8_t value) override;

	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
	void write_f (uint_least16_t address, uint8_t value) override;

	public:
	void cpu_sync() override;
	vrc7_debug_state_t debug_mapper_state() const;
	
	private:
	void clock_irq();
	void write_audio_register (uint8_t reg, uint8_t value);

	private:
	uint8_t wram_[0x2000] = {};
	uint8_t chr_ram_[0x2000] = {};
	bool wram_enabled_ = false;

	private:
	uint8_t prg_[3] = {};
	uint8_t chr_[8] = {};
	
	private:
	uint8_t irq_latch_ = 0;
	IRQControl irq_control_ = {0};
	uint8_t irq_counter_ = 0;
	int irq_prescaler_ = 341;
	
	private:
	uint64_t irq_clock_count_ = 0;
	uint64_t irq_assert_count_ = 0;

	private:
	uint8_t audio_register_ = 0;
	bool audio_silenced_ = false;
	
	private:
	enum class FMEnvelopeState {
		Idle,
		Damp,
		Attack,
		Decay,
		Sustain,
		Release
	};
	
	private:
	struct fm_operator_t {
		uint32_t phase = 0;
		uint32_t phase_increment = 0;

		FMEnvelopeState envelope_state = FMEnvelopeState::Idle;

		uint8_t envelope_level = 127;

		uint32_t envelope_counter = 0;

		int16_t output = 0;

		int16_t feedback_output_0 = 0;
		int16_t feedback_output_1 = 0;

		bool key_on = false;
	};
	
	
	private:
	struct fm_instrument_t {
		uint8_t modulator_multiplier = 0;
		uint8_t carrier_multiplier = 0;

		bool modulator_sustain = false;
		bool carrier_sustain = false;

		bool modulator_tremolo = false;
		bool carrier_tremolo = false;

		bool modulator_key_scale_rate = false;
		bool carrier_key_scale_rate = false;

		bool modulator_vibrato = false;
		bool carrier_vibrato = false;

		uint8_t modulator_key_scale_level = 0;
		uint8_t carrier_key_scale_level = 0;

		uint8_t modulator_output_level = 0;

		uint8_t feedback = 0;

		bool modulator_waveform = false;
		bool carrier_waveform = false;

		uint8_t modulator_attack = 0;
		uint8_t carrier_attack = 0;

		uint8_t modulator_decay = 0;
		uint8_t carrier_decay = 0;

		uint8_t modulator_sustain_level = 0;
		uint8_t carrier_sustain_level = 0;

		uint8_t modulator_release = 0;
		uint8_t carrier_release = 0;
	};
	
	private:
	struct fm_channel_t {
		uint16_t f_number = 0;
		uint8_t block = 0;
		uint8_t instrument = 0;
		uint8_t volume = 0;

		bool key_on = false;
		bool sustain = false;

		fm_operator_t modulator;
		fm_operator_t carrier;

		fm_instrument_t patch;
		
		bool muted = false;
	};
	
	private:
	void decode_instrument (fm_channel_t &channel);
	void update_phase_increments (fm_channel_t &channel);
	void clock_fm_phase();
	int16_t waveform_sample (uint32_t phase, bool half_sine) const;
	void update_key_state (fm_channel_t &channel, bool keyOn);
	
	private:
	void clock_envelope (fm_channel_t &channel, fm_operator_t &op, bool carrier);
	uint8_t envelope_key_scale (const fm_channel_t &channel, bool carrier) const;
	uint8_t envelope_parameter_rate (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const;
	uint8_t envelope_rate_high (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const;
	uint8_t envelope_rate_low (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const;
	int16_t apply_envelope (int16_t sample, uint8_t envelopeLevel) const;
	
	private:
	uint8_t combined_attenuation (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const;
	int32_t feedback_phase_offset (const fm_channel_t &channel) const;
	int32_t carrier_phase_offset (const fm_channel_t &channel) const;
	void update_fm_output();
	
	private:
	uint8_t audio_output() const override;
	size_t audio_channel_count() const override;
	const char *audio_channel_name (size_t index) const override;
	bool audio_channel_muted(size_t index) const override;
	void set_audio_channel_muted (size_t index, bool muted) override;
	mapper_audio_debug_state_t debug_audio_state() const override;
	mapper_audio_scope_sample_t debug_audio_scope_sample() const override;
	
	private:
	int32_t audio_output_bias() const override;
	
	private:
	uint8_t fm_registers_[0x40] = {};
	uint8_t custom_instrument_[8] = {};

	fm_channel_t fm_channels_[6] = {};
	
	int fm_clock_divider_ = 36;
	uint32_t envelope_counter_ = 0;
	int32_t fm_output_ = 0;
	
};


#endif	// _VRC7_H_

