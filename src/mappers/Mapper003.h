#ifndef MAPPER0003_20080314_H_
#define MAPPER0003_20080314_H_

#include "Mapper.h"


struct cnrom_debug_state_t {
	uint8_t bank_select = 0;
	uint32_t resolved_chr_bank = 0;

	uint64_t write_count = 0;

	bool have_last_write = false;
	uint16_t last_write_address = 0;
	uint8_t last_write_value = 0;
};


class Mapper3 final : public Mapper
{
	public:
	Mapper3();

	public:
	std::string name() const override;

	cnrom_debug_state_t debug_state() const;

	public:
	void write_8(uint_least16_t address, uint8_t value) override;
	void write_9(uint_least16_t address, uint8_t value) override;
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_b(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_d(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	void write_f(uint_least16_t address, uint8_t value) override;

	private:
	uint8_t bank_select_ = 0;

	private:
	uint64_t debug_write_count_ = 0;

	bool debug_have_last_write_ = false;
	uint16_t debug_last_write_address_ = 0;
	uint8_t debug_last_write_value_ = 0;
};


#endif
