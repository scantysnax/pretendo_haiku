
#ifndef _MAPPER_071_H_
#define _MAPPER_071_H_

#include "Mapper.h"


struct camerica_debug_state_t {
	uint8_t prg_bank = 0;
};


class Mapper71 final : public Mapper
{
	public:
	Mapper71();

	public:
	std::string name() const override;

	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
	void write_f (uint_least16_t address, uint8_t value) override;

	camerica_debug_state_t debug_state() const;

	private:
	void write_prg_bank (uint8_t value);

	private:
	uint8_t chr_ram_[0x2000] = {};
	uint8_t prg_bank_ = 0;
};


#endif	// _MAPPER_071_H_
