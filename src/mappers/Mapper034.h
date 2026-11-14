
#ifndef _MAPPER_034_H_
#define _MAPPER_034_H_

#include "Mapper.h"

class Mapper34 final : public Mapper
{
	public:
	Mapper34();

	public:
	std::string name() const override;

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

	private:
	void write_handler(uint_least16_t address, uint8_t value);

	private:
	uint8_t prg_ram_[0x2000] = {};
	uint8_t chr_ram_[0x2000] = {};
	bool prg_ram_enabled_ = false;

	private:
	enum Mode {
		NINA_001,
		BNROM,
	} mode_;
};

#endif	// _MAPPER_034_H_
