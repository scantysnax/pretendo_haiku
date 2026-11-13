
#ifndef _MAPPER_003_H_
#define _MAPPER_003_H_

#include "Mapper.h"


// -----------------------------------------------------------------------------
// cnrom_debug_state_t
//
// Contains CNROM-specific state for the Mapper Explorer.
//
// The raw bank-selection value reflects the effective value received by
// the mapper after any applicable bus conflict.
//
// The last-write value records the original CPU write value.
// -----------------------------------------------------------------------------
struct cnrom_debug_state_t {
	uint8_t bank_select = 0;
	uint32_t resolved_chr_bank = 0;

	uint64_t write_count = 0;

	bool have_last_write = false;
	uint16_t last_write_address = 0;
	uint8_t last_write_value = 0;
};


// -----------------------------------------------------------------------------
// Mapper3
//
// Implements mapper 3 (CNROM).
//
// Hardware features:
//
//   - Fixed PRG-ROM mapping at $8000-$FFFF.
//   - Switchable 8 KB CHR-ROM bank at $0000-$1FFF.
//   - No PRG RAM in this implementation.
//   - Optional AND-type bus conflicts on PRG-ROM register writes.
//
// NES 2.0 submapper behavior:
//
//   - Submapper 0: Unspecified; use legacy default behavior.
//   - Submapper 1: No bus conflicts.
//   - Submapper 2: AND-type bus conflicts.
//
// The mapper factory selects the appropriate behavior during construction.
// -----------------------------------------------------------------------------
class Mapper3 final : public Mapper
{
	public:
	explicit Mapper3(bool bus_conflicts = false);
	
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
	// Applies bus conflicts and selects the active CHR-ROM bank.
	void select_chr_bank(uint_least16_t address, uint8_t value);

	private:
	// Enables AND-type bus conflicts when required by the board.
	bool bus_conflicts_ = false;

	// Effective CHR-ROM bank-selection register.
	uint8_t bank_select_ = 0;

	private:
	// Persistent mapper-write diagnostics.
	uint64_t debug_write_count_ = 0;

	bool debug_have_last_write_ = false;
	uint16_t debug_last_write_address_ = 0;
	uint8_t debug_last_write_value_ = 0;
};


#endif

