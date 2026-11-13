
#include "Mapper002.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(2)


// -----------------------------------------------------------------------------
// Mapper2::Mapper2
//
// Initializes mapper 2 (UxROM).
//
// UxROM provides a switchable 16 KB PRG-ROM bank at $8000-$BFFF, a fixed
// 16 KB PRG-ROM bank at $C000-$FFFF, and 8 KB of CHR RAM.
//
// NES 2.0 submapper 2 uses AND-type bus conflicts. Submapper 1 disables
// bus conflicts. The legacy/default behavior is selected by the factory.
//
// Parameters:
//   bus_conflicts - Whether AND-type PRG-ROM bus conflicts are enabled.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper2::Mapper2 (bool bus_conflicts)
	: bus_conflicts_(bus_conflicts)
{
	unmap_67();

	set_prg_89ab(0);
	set_prg_cdef(-1);

	set_chr_0000_1fff_ram(chr_ram_, 0);
}


// -----------------------------------------------------------------------------
// Mapper2::name
//
// Returns the human-readable mapper name.
//
// Parameters:
//   None.
//
// Returns:
//   "UxROM".
// -----------------------------------------------------------------------------
std::string
Mapper2::name() const
{
	return "UxROM";
}


// -----------------------------------------------------------------------------
// Mapper2::debug_state
//
// Returns a snapshot of UxROM-specific internal state for debugger inspection.
//
// The bank-selection value is the effective value received by the mapper,
// after any applicable bus conflict.
//
// The last-write value remains the original CPU write value.
//
// Parameters:
//   None.
//
// Returns:
//   Current UxROM debugger state.
// -----------------------------------------------------------------------------
uxrom_debug_state_t
Mapper2::debug_state() const
{
	uxrom_debug_state_t state;

	state.bank_select = bank_select_;
	state.prg_bank = bank_select_;
	state.write_count = debug_write_count_;
	state.have_last_write = debug_have_last_write_;
	state.last_write_address = debug_last_write_address_;
	state.last_write_value = debug_last_write_value_;

	return state;
}



// -----------------------------------------------------------------------------
// Mapper2::select_prg_bank
//
// Handles UxROM PRG-ROM bank-selection writes throughout $8000-$FFFF.
//
// On boards with AND-type bus conflicts, the CPU and PRG-ROM simultaneously
// drive the data bus during a mapper register write.
//
// The effective bank-selection value is the bitwise AND of the CPU write
// value and the ROM byte currently mapped at the write address.
//
// The ROM byte must be sampled before changing the PRG-ROM bank.
//
// The debugger records the original CPU write value separately from the
// effective bank-selection value.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper2::select_prg_bank(uint_least16_t address, uint8_t value)
{
	const uint8_t original_value = value;

	/*
	 * Apply AND-type bus conflicts when enabled.
	 *
	 * read_memory() uses the mapper's current memory mapping,
	 * allowing us to sample the PRG-ROM byte before changing banks.
	 */
	if (bus_conflicts_) {
		const uint8_t rom_value = read_memory(address);

		value &= rom_value;
	}

	/*
	 * Store the effective bank-selection value.
	 */
	bank_select_ = value;

	/*
	 * Update the mapper debugger diagnostics.
	 *
	 * Preserve the original CPU value so we can distinguish
	 * what the CPU wrote from what the mapper received.
	 */
	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = original_value;

	/*
	 * Select the switchable 16 KB PRG-ROM bank.
	 */
	set_prg_89ab(value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_8
//
// Handles UxROM bank-selection writes at $8000-$8FFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_8(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_9
//
// Handles UxROM bank-selection writes at $9000-$9FFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_9(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_a
//
// Handles UxROM bank-selection writes at $A000-$AFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_a(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_b
//
// Handles UxROM bank-selection writes at $B000-$BFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_b(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_c
//
// Handles UxROM bank-selection writes at $C000-$CFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_c(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_d
//
// Handles UxROM bank-selection writes at $D000-$DFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_d(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_e
//
// Handles UxROM bank-selection writes at $E000-$EFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_e(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper2::write_f
//
// Handles UxROM bank-selection writes at $F000-$FFFF.
// -----------------------------------------------------------------------------
void
Mapper2::write_f(uint_least16_t address, uint8_t value)
{
	select_prg_bank(address, value);
}



