

#include "Mapper007.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(7)


//------------------------------------------------------------------------------
// Name: Mapper7
//
// Initializes mapper 7 (AxROM).
//
// AxROM provides:
//
//   - One switchable 32 KB PRG-ROM bank at $8000-$FFFF.
//   - 8 KB of CHR RAM at $0000-$1FFF.
//   - Mapper-controlled single-screen nametable mirroring.
//   - No PRG RAM in this implementation.
//
// The mapper's mirroring selection is not considered known until the first
// mapper write occurs, so the debugger-visible state begins as Unknown.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper7::Mapper7()
{
	// AxROM does not provide PRG RAM in this implementation.
	unmap_67();

	// Start with the final 32 KB PRG-ROM bank mapped at $8000-$FFFF.
	set_prg_89abcdef(-1);

	// Standard AxROM uses 8 KB of CHR RAM.
	set_chr_0000_1fff_ram(chr_ram_, 0);

	// AxROM controls mirroring through its mapper register. Until the first
	// mapper write occurs, the effective single-screen selection is unknown.
	//
	// This changes only the debugger-visible state; the PPU mapping established
	// during base Mapper construction remains untouched.
	debug_set_mirroring_unknown();
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
//   "AxROM".
//------------------------------------------------------------------------------
std::string
Mapper7::name() const
{
	return "AxROM";
}


// -----------------------------------------------------------------------------
// Mapper7::debug_state_axrom
//
// Returns a snapshot of AxROM-specific internal state for debugger inspection.
//
// The snapshot includes the raw mapper-control value, selected 32 KB PRG-ROM
// bank, single-screen nametable selection, and persistent mapper-write
// diagnostics.
//
// Parameters:
//   None.
//
// Returns:
//   Current AxROM-specific debugger state.
// -----------------------------------------------------------------------------
axrom_debug_state_t
Mapper7::debug_state() const
{
	axrom_debug_state_t state;

	state.control = control_;
	state.prg_bank = control_ & 0x07;
	state.single_screen_high = (control_ & 0x10) != 0;
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
// AxROM treats writes throughout $8000-$FFFF as writes to the same mapper
// control register.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_8 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Handles mapper writes in the $9000-$9FFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_9 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles mapper writes in the $A000-$AFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_a (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Handles mapper writes in the $B000-$BFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_b (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Handles mapper writes in the $C000-$CFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_c (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Handles mapper writes in the $D000-$DFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_d (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Handles mapper writes in the $E000-$EFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_e (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Handles mapper writes in the $F000-$FFFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_f (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_handler
//
// Applies one AxROM mapper-control write.
//
// The register controls:
//
//   bits 0-2 -> Select the 32 KB PRG-ROM bank mapped at $8000-$FFFF.
//   bit 4    -> Select which CIRAM page is used for single-screen mirroring.
//
// Mirroring selection:
//
//   bit 4 clear -> single-screen low
//   bit 4 set   -> single-screen high
//
// The CPU address itself does not affect the selected function; writes
// anywhere in $8000-$FFFF reach the same mapper register.
//
// Debugger-only state preserves the raw register value and most recent mapper
// write for inspection.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mapper control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper7::write_handler (uint_least16_t address, uint8_t value)
{
	// Preserve the raw AxROM mapper-control register.
	control_ = value;

	// Record mapper-write activity for debugger inspection.
	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = value;

	// Select the complete 32 KB PRG-ROM bank mapped at $8000-$FFFF.
	set_prg_89abcdef(control_ & 0x07);

	// Bit 4 selects which 1 KB CIRAM page backs all four logical nametables.
	if (control_ & 0x10) {
		set_mirroring(mirror_single_high);
	} else {
		set_mirroring(mirror_single_low);
	}
}



