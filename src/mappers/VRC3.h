#ifndef _VRC3_H_
#define _VRC3_H_

#include "BitField.h"
#include "Mapper.h"


class VRC3 : public Mapper
{
	private:
	union IRQControl {
		uint8_t raw;
		BitField<uint8_t, 0> a;
		BitField<uint8_t, 1> enabled;
		BitField<uint8_t, 2> mode;
	};

	public:
	VRC3();

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
	void write_f (uint_least16_t address, uint8_t value) override;

	public:
	void cpu_sync() override;

	private:
	void clock_irq();

	private:
	uint8_t wram_[0x2000] = {};

	uint16_t irq_latch_ = 0;
	uint16_t irq_counter_ = 0;
	IRQControl irq_control_ = {0};
};


#endif	// _VRC3_H_
