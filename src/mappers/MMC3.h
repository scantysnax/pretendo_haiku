
#ifndef _MMC3_H_
#define _MMC3_H_

#include "Mapper.h"


struct mmc3_debug_state_t {
	uint8_t command = 0;
	uint8_t selected_register = 0;

	bool prg_mode = false;
	bool chr_mode = false;

	uint8_t prg_bank[2] = {};
	uint8_t chr_bank[8] = {};

	bool prg_ram_enabled = false;
	bool prg_ram_writable = false;

	uint8_t irq_latch = 0;
	uint8_t irq_counter = 0;
	bool irq_reload = false;
	bool irq_enabled = false;

	uint8_t hardware_mode = 0;

	uint64_t a12_rising_edge_count = 0;
	uint64_t a12_qualified_edge_count = 0;
	uint64_t a12_rejected_edge_count = 0;

	uint64_t irq_clock_count = 0;
	uint64_t irq_assert_count = 0;

	uint64_t last_a12_spacing = 0;
};


class MMC3 : public Mapper
{
	public:
	MMC3();

	public:
	std::string name() const override;

	public:
	uint8_t read_6(uint_least16_t address) override;
	uint8_t read_7(uint_least16_t address) override;

	public:
	void write_6(uint_least16_t address, uint8_t value) override;
	void write_7(uint_least16_t address, uint8_t value) override;
	void write_8(uint_least16_t address, uint8_t value) override;
	void write_9(uint_least16_t address, uint8_t value) override;
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_b(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_d(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	void write_f(uint_least16_t address, uint8_t value) override;

	public:
	void vram_change_hook(uint_least16_t vram_address) override;

	private:
	int prg_bank(int bank) const;
	int chr_bank(int bank) const;

	private:
	void clock_irqA();
	void clock_irqB();
	void clock_irq();

	protected:
	uint8_t chr_ram_[0x40000] = {}; // we should get this from iNES 2.0,
									// but this seems to do for now
	uint8_t chr_bank_[8] = {};
	uint8_t prg_bank_[2] = {};

	uint64_t prev_ppu_cycle_    = 0;
	uint_least16_t prev_vram_address_ = 0xffff;
	uint8_t command_            = 0;
	uint8_t irq_latch_          = 0;
	uint8_t irq_counter_        = 0;
	bool irq_enabled_           = false;
	bool irq_reload_            = false;
	bool save_ram_enabled_      = false;
	bool save_ram_writable_     = false;
	
	public:
	mmc3_debug_state_t debug_state() const;
	
	private:
	uint64_t debug_a12_rising_edge_count_ = 0;
	uint64_t debug_a12_qualified_edge_count_ = 0;
	uint64_t debug_a12_rejected_edge_count_ = 0;

	uint64_t debug_irq_clock_count_ = 0;
	uint64_t debug_irq_assert_count_ = 0;

	uint64_t debug_last_a12_spacing_ = 0;

	private:
	MemoryMappedFile prg_ptr_;

	protected:
	enum Mode {
		ModeA,
		ModeB,
		ModeMMC6,
	} mode_ = ModeA;
};


#endif	//  _MMC3_H_
