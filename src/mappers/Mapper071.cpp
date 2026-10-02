#include "Mapper071.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(71)


//------------------------------------------------------------------------------
// Name: Mapper71
//
// Initializes Mapper 71 (Camerica/Codemasters).
//
// Mapper 71 provides a switchable 16 KB PRG-ROM bank at $8000-$BFFF and keeps
// the final 16 KB PRG-ROM bank fixed at $C000-$FFFF.  Pattern memory is provided
// by 8 KB of CHR RAM.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper71::Mapper71()
{
	set_prg_89ab(0);
	set_prg_cdef(-1);
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
//   Mapper name.
//------------------------------------------------------------------------------
std::string
Mapper71::name() const
{
	return "Camerica/Codemasters";
}


//------------------------------------------------------------------------------
// Name: write_8
//
// Handles writes in the $8000-$8FFF range.
//
// Standard Camerica Mapper 71 boards do not use this range for PRG banking.
// Some Fire Hawk boards use writes in this area for single-screen mirroring;
// that board-specific behavior is currently not enabled.
//
// Parameters:
//   address - CPU address written.
//   value   - Value written.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_8(uint_least16_t address, uint8_t value)
{
	(void)address;
	(void)value;

#if 0
	// Fire Hawk only.
	if (value & 0x10) {
		set_mirroring(mirror_single_high);
	} else {
		set_mirroring(mirror_single_low);
	}
#endif
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Handles writes in the $9000-$9FFF range.
//
// This range follows the same behavior as $8000-$8FFF.
//
// Parameters:
//   address - CPU address written.
//   value   - Value written.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_9(uint_least16_t address, uint8_t value)
{
	write_8(address, value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Selects the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address written.
//   value   - Mapper register value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_c(uint_least16_t address, uint8_t value)
{
	(void)address;

	write_prg_bank(value);
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Selects the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address written.
//   value   - Mapper register value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_d(uint_least16_t address, uint8_t value)
{
	(void)address;

	write_prg_bank(value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Selects the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address written.
//   value   - Mapper register value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_e(uint_least16_t address, uint8_t value)
{
	(void)address;

	write_prg_bank(value);
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Selects the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address written.
//   value   - Mapper register value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_f(uint_least16_t address, uint8_t value)
{
	(void)address;

	write_prg_bank(value);
}


//------------------------------------------------------------------------------
// Name: write_prg_bank
//
// Updates the Mapper 71 PRG-bank register and maps the selected 16 KB PRG-ROM
// bank at $8000-$BFFF.
//
// Only the low four bits of the written value participate in bank selection.
//
// Parameters:
//   value - Mapper register value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper71::write_prg_bank(uint8_t value)
{
	prg_bank_ = value & 0x0f;

	set_prg_89ab(prg_bank_);
}


//------------------------------------------------------------------------------
// Name: camerica_debug_state
//
// Captures Mapper-71-specific state for the Mapper Explorer.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot of the current Mapper 71 debugger-visible state.
//------------------------------------------------------------------------------
camerica_debug_state_t
Mapper71::debug_state() const
{
	camerica_debug_state_t state;

	state.prg_bank = prg_bank_;

	return state;
}
