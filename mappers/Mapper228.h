
#ifndef MAPPER228_20080314_H_
#define MAPPER228_20080314_H_

#include "Mapper.h"

class Mapper228 final : public Mapper {
public:
	Mapper228();

public:
	std::string name() const override;

public:
	void write_4(uint32_t address, uint8_t value) override;
	void write_5(uint32_t address, uint8_t value) override;
	void write_8(uint32_t address, uint8_t value) override;
	void write_9(uint32_t address, uint8_t value) override;
	void write_a(uint32_t address, uint8_t value) override;
	void write_b(uint32_t address, uint8_t value) override;
	void write_c(uint32_t address, uint8_t value) override;
	void write_d(uint32_t address, uint8_t value) override;
	void write_e(uint32_t address, uint8_t value) override;
	void write_f(uint32_t address, uint8_t value) override;

private:
	uint8_t read_4(uint32_t address) override;
	uint8_t read_5(uint32_t address) override;
	uint8_t read_8(uint32_t address) override;
	uint8_t read_9(uint32_t address) override;
	uint8_t read_a(uint32_t address) override;
	uint8_t read_b(uint32_t address) override;
	uint8_t read_c(uint32_t address) override;
	uint8_t read_d(uint32_t address) override;
	uint8_t read_e(uint32_t address) override;
	uint8_t read_f(uint32_t address) override;

private:
	void write_hander(uint32_t address, uint8_t value);

private:
	const uint8_t *prg_chips_[4] = {};
	uint8_t ram_[0x04]           = {};
	const uint8_t *rom_[0x08]    = {};
};

#endif
