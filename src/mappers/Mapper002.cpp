
#include "Mapper002.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(2)


//------------------------------------------------------------------------------
// Name: Mapper2
//
// Initializes mapper 2 (UxROM).
//
// UxROM provides:
//
//   - One switchable 16 KB PRG-ROM bank at $8000-$BFFF.
//   - One fixed 16 KB PRG-ROM bank at $C000-$FFFF.
//   - 8 KB of CHR RAM at $0000-$1FFF.
//   - No PRG RAM in this implementation.
//
// Bank writes anywhere in $8000-$FFFF select the 16 KB PRG-ROM bank mapped
// into $8000-$BFFF.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper2::Mapper2()
{
	// UxROM does not provide PRG RAM in this implementation.
	unmap_67();

	// Start with the first 16 KB PRG bank at $8000-$BFFF and keep the
	// final 16 KB PRG bank fixed at $C000-$FFFF.
	set_prg_89ab(0);
	set_prg_cdef(-1);

	// Standard UxROM boards use 8 KB of CHR RAM.
	set_chr_0000_1fff_ram(chr_ram_, 0);
}


//------------------------------------------------------------------------------
// Name: name
//
// Returns the human-readable mapper name.
//
// Parameters:
//   None.
//
// Returns:
//   "UxROM".
//------------------------------------------------------------------------------
std::string
Mapper2::name() const
{
	return "UxROM";
}


// -----------------------------------------------------------------------------
// Mapper2::debug_state_uxrom
//
// Returns a snapshot of UxROM-specific internal state for debugger inspection.
//
// The snapshot includes the raw bank-selection value, selected 16 KB PRG-ROM
// bank, and persistent mapper-write diagnostics.
//
// Parameters:
//   None.
//
// Returns:
//   Current UxROM-specific debugger state.
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


//------------------------------------------------------------------------------
// Name: write_8
//
// Handles mapper writes in the $8000-$8FFF range.
//
// The written value selects the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_8 (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Handles mapper writes in the $9000-$9FFF range.
//
// UxROM treats writes throughout $8000-$FFFF as PRG-bank-selection writes.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_9 (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles mapper writes in the $A000-$AFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_a (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Handles mapper writes in the $B000-$BFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_b (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Handles mapper writes in the $C000-$CFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_c (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Handles mapper writes in the $D000-$DFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_d (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Handles mapper writes in the $E000-$EFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_e (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Handles mapper writes in the $F000-$FFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper2::write_f (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_prg_89ab(value);
}


