
#ifndef MAPPER068_20080314_H_
#define MAPPER068_20080314_H_

#include "Mapper.h"

class Mapper68 final : public Mapper {
public:
	Mapper68();

public:
	std::string name() const override;

	void write_8(uint32_t address, uint8_t value) override;
	void write_9(uint32_t address, uint8_t value) override;
	void write_a(uint32_t address, uint8_t value) override;
	void write_b(uint32_t address, uint8_t value) override;
	void write_c(uint32_t address, uint8_t value) override;
	void write_d(uint32_t address, uint8_t value) override;
	void write_e(uint32_t address, uint8_t value) override;
	void write_f(uint32_t address, uint8_t value) override;

	uint8_t read_vram(uint32_t address) override;

private:
	uint8_t chr_rom_reg_[2] = {0x80, 0x80};
	uint8_t reg_e000_       = 0;
};

#endif
