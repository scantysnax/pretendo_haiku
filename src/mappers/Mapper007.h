#ifndef MAPPER0007_20080314_H_
#define MAPPER0007_20080314_H_

#include "Mapper.h"


struct axrom_debug_state_t {
	uint8_t control = 0;
	uint8_t prg_bank = 0;

	bool single_screen_high = false;

	uint64_t write_count = 0;

	bool have_last_write = false;
	uint16_t last_write_address = 0;
	uint8_t last_write_value = 0;
};


class Mapper7 : public Mapper
{
	public:
	Mapper7();

	public:
	std::string name() const override;

	axrom_debug_state_t debug_state() const;

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
	void write_handler(uint_least16_t address, uint8_t value);

	private:
	uint8_t chr_ram_[0x2000] = {};

	uint8_t control_ = 0;

	private:
	uint64_t debug_write_count_ = 0;

	bool debug_have_last_write_ = false;
	uint16_t debug_last_write_address_ = 0;
	uint8_t debug_last_write_value_ = 0;
};


#endif
