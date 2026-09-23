#ifndef MAPPER0009_20080314_H_
#define MAPPER0009_20080314_H_

#include "Mapper.h"


struct mmc2_debug_state_t {
	uint8_t prg_bank = 0;

	uint8_t latch0_lo = 0;
	uint8_t latch0_hi = 0;
	uint8_t latch1_lo = 0;
	uint8_t latch1_hi = 0;

	bool latch0 = true;
	bool latch1 = true;

	uint8_t active_chr0_bank = 0;
	uint8_t active_chr1_bank = 0;

	uint64_t latch0_low_count = 0;
	uint64_t latch0_high_count = 0;
	uint64_t latch1_low_count = 0;
	uint64_t latch1_high_count = 0;

	bool have_last_trigger = false;
	uint16_t last_trigger_address = 0;
};


class Mapper9 final : public Mapper
{
	public:
	Mapper9();

	public:
	std::string name() const override;

	mmc2_debug_state_t debug_state() const;

	public:
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_b(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_d(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	void write_f(uint_least16_t address, uint8_t value) override;

	public:
	uint8_t read_vram(uint_least16_t address) override;

	private:
	uint8_t chr_ram_[0x2000] = {};

	bool latch0_ = true;
	bool latch1_ = true;

	uint8_t latch0_lo_ = 0;
	uint8_t latch0_hi_ = 0;
	uint8_t latch1_lo_ = 0;
	uint8_t latch1_hi_ = 0;

	uint8_t prg_bank_ = 0;

	private:
	uint64_t debug_latch0_low_count_ = 0;
	uint64_t debug_latch0_high_count_ = 0;
	uint64_t debug_latch1_low_count_ = 0;
	uint64_t debug_latch1_high_count_ = 0;

	bool debug_have_last_trigger_ = false;
	uint16_t debug_last_trigger_address_ = 0;
};


#endif


