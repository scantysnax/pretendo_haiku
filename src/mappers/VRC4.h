#ifndef _VRC4_H_
#define _VRC4_H_

#include "BitField.h"
#include "Mapper.h"


// -----------------------------------------------------------------------------
// vrc4_debug_state_t
//
// Debugger-facing snapshot of VRC4 mapper-control state.
//
// Generic resolved PRG/CHR mappings and nametable mirroring are already exposed
// through Mapper::debug_state(), so this structure contains VRC4-specific
// internal register and IRQ state that cannot be derived reliably from those
// resolved mappings alone.
// -----------------------------------------------------------------------------
struct vrc4_debug_state_t {
	uint8_t prg[2] = {};
	uint8_t chr[8] = {};

	bool wram_enabled = false;

	uint8_t prg_mode = 0;
	bool prg_swap_mode = false;

	uint8_t irq_latch = 0;
	uint8_t irq_counter = 0;
	int irq_prescaler = 341;

	uint8_t irq_control = 0;
	bool irq_enable_after_ack = false;
	bool irq_enabled = false;
	bool irq_cycle_mode = false;

	uint64_t irq_clock_count = 0;
	uint64_t irq_assert_count = 0;
};


class VRC4 : public Mapper
{
	private:
	union IRQControl {
		uint8_t raw;
		BitField<uint8_t, 0> a;
		BitField<uint8_t, 1> enabled;
		BitField<uint8_t, 2> mode;
	};

	public:
	VRC4();

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
	
	public:
	vrc4_debug_state_t debug_mapper_state() const;

	private:
	void clock_irq();

	private:
	uint8_t chr_[8] = {};
	uint8_t prg_[2] = {};

	uint8_t wram_[0x2000] = {};
	bool wram_enabled_ = false;

	uint8_t irq_latch_      = 0;
	IRQControl irq_control_ = {0};
	uint8_t irq_counter_    = 0;
	uint8_t prg_mode_       = 0;
	int irq_prescaler_      = 341;
	
	uint64_t irq_clock_count_ = 0;
	uint64_t irq_assert_count_ = 0;
};

#endif	// _VRC4_H_

