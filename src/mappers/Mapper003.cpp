
#include "Mapper003.h"
#include "Cart.h"
#include "Nes.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(3)


//------------------------------------------------------------------------------
// Name: Mapper3
//
// Initializes mapper 3 (CNROM).
//
// CNROM provides:
//
//   - Fixed PRG ROM across $8000-$FFFF.
//   - One switchable 8 KB CHR-ROM bank at $0000-$1FFF.
//   - No PRG RAM in this implementation.
//
// Writes anywhere in $8000-$FFFF select the active 8 KB CHR-ROM bank.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper3::Mapper3()
{
	// CNROM does not provide PRG RAM in this implementation.
	unmap_67();

	// PRG ROM is fixed for the lifetime of the cartridge.
	//
	// The first 16 KB PRG bank is mapped at $8000-$BFFF, while the final
	// 16 KB PRG bank is mapped at $C000-$FFFF.
	set_prg_89ab(0);
	set_prg_cdef(-1);

	// Start with the first 8 KB CHR-ROM bank mapped at $0000-$1FFF.
	set_chr_0000_1fff(0);
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
//   "CNROM".
//------------------------------------------------------------------------------
std::string
Mapper3::name() const
{
	return "CNROM";
}

// -----------------------------------------------------------------------------
// Mapper3::debug_state
//
// Returns a snapshot of CNROM-specific internal state for debugger inspection.
//
// The snapshot includes the raw CHR-bank selection value, resolved 8 KB
// CHR-ROM bank, and persistent mapper-write diagnostics.
//
// Parameters:
//   None.
//
// Returns:
//   Current CNROM-specific debugger state.
// -----------------------------------------------------------------------------
cnrom_debug_state_t
Mapper3::debug_state() const
{
	cnrom_debug_state_t state;

	state.bank_select = bank_select_;

	const uint32_t mask = nes::cart.chr_mask();
	const uint32_t base = static_cast<uint32_t>(bank_select_) * 0x2000;
	const uint32_t offset = base & mask;

	state.resolved_chr_bank = offset / 0x2000;
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
// The written value selects the complete 8 KB CHR-ROM bank mapped at
// $0000-$1FFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_8 (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Handles mapper writes in the $9000-$9FFF range.
//
// CNROM treats writes throughout $8000-$FFFF as CHR-bank-selection writes.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_9 (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles mapper writes in the $A000-$AFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_a (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Handles mapper writes in the $B000-$BFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_b (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Handles mapper writes in the $C000-$CFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_c (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Handles mapper writes in the $D000-$DFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_d (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Handles mapper writes in the $E000-$EFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_e (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Handles mapper writes in the $F000-$FFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - CHR-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper3::write_f (uint_least16_t address, uint8_t value)
{
	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	set_chr_0000_1fff(value);
}


