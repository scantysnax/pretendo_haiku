
#ifndef _MAPPER_007_H_
#define _MAPPER_007_H_

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


// -----------------------------------------------------------------------------
// Mapper7
//
// Implements mapper 7 (AxROM).
//
// Hardware features:
//
//   - One switchable 32 KB PRG-ROM bank at $8000-$FFFF.
//   - 8 KB of CHR RAM at $0000-$1FFF.
//   - Mapper-controlled single-screen nametable mirroring.
//   - No PRG RAM in this implementation.
//   - Optional AND-type bus conflicts on mapper register writes.
//
// NES 2.0 submapper behavior:
//
//   - Submapper 0: Unspecified; preserve legacy behavior.
//   - Submapper 1: No bus conflicts.
//   - Submapper 2: AND-type bus conflicts.
//
// The mapper factory selects the appropriate behavior during construction.
// -----------------------------------------------------------------------------
class Mapper7 : public Mapper
{
public:
	explicit Mapper7(bool bus_conflicts = false);

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
	// Enables AND-type bus conflicts when required by the board.
	bool bus_conflicts_ = false;

	// AxROM provides 8 KB of CHR RAM.
	uint8_t chr_ram_[0x2000] = {};

	// Effective mapper-control value after any bus conflict.
	uint8_t control_ = 0;

private:
	// Persistent mapper-write diagnostics.
	uint64_t debug_write_count_ = 0;

	bool debug_have_last_write_ = false;
	uint16_t debug_last_write_address_ = 0;
	uint8_t debug_last_write_value_ = 0;
};


#endif  // _MAPPER_007_H_

