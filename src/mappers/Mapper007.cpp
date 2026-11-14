

#include "Mapper007.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(7)



// -----------------------------------------------------------------------------
// Mapper7::Mapper7
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
// NES 2.0 submapper 1 has no bus conflicts.
// NES 2.0 submapper 2 uses AND-type bus conflicts.
//
// The mapper's mirroring selection is not considered known until the first
// mapper write occurs, so the debugger-visible state begins as Unknown.
//
// Parameters:
//   bus_conflicts - Enables AND-type PRG-ROM bus conflicts.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper7::Mapper7(bool bus_conflicts)
	:
	bus_conflicts_(bus_conflicts)
{
	// AxROM does not provide PRG RAM in this implementation.
	unmap_67();

	// Start with the final 32 KB PRG-ROM bank mapped at $8000-$FFFF.
	set_prg_89abcdef(-1);

	// Standard AxROM uses 8 KB of CHR RAM.
	set_chr_0000_1fff_ram(chr_ram_, 0);

	// The initial single-screen mirroring selection is unknown
	// until the first mapper-control register write.
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
	state.prg_bank = control_ & 0x0f;
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



// -----------------------------------------------------------------------------
// Mapper7::write_handler
//
// Handles AxROM mapper-control writes throughout $8000-$FFFF.
//
// The mapper-control register provides:
//
//   bits 0-2 -> Select the 32 KB PRG-ROM bank on standard AxROM boards.
//   bit 3    -> Extended PRG-ROM bank selection for oversized 512 KB AxROM.
//   bit 4    -> Select the single-screen nametable mirroring mode.
//   bits 5-7 -> Unused.
//
// PRG-ROM banking:
//
//   Standard AxROM supports up to 256 KB of PRG ROM, organized as eight
//   switchable 32 KB banks.
//
//   Oversized AxROM variants may support up to 512 KB of PRG ROM, organized
//   as sixteen switchable 32 KB banks.
//
//   Bits 0-3 are passed to the PRG bank-mapping helper. The cartridge's
//   PRG-ROM mask handles bank wrapping for smaller ROM images.
//
// Nametable mirroring:
//
//   Bit 4 clear -> Single-screen low  (CIRAM page 0).
//   Bit 4 set   -> Single-screen high (CIRAM page 1).
//
// Bus conflicts:
//
//   On boards with AND-type bus conflicts (NES 2.0 submapper 2), the CPU
//   and currently mapped PRG ROM simultaneously drive the data bus during
//   writes to the mapper-control register.
//
//   The effective mapper-control value is the bitwise AND of the original
//   CPU write value and the PRG-ROM byte at the destination address.
//
//   The ROM byte must be sampled before updating the PRG-ROM bank mapping.
//
//   Boards without bus conflicts use the original CPU write value directly.
//
//   Bus conflicts affect both PRG-ROM bank selection and nametable
//   mirroring because both are controlled by the effective register value.
//
// Debugger diagnostics:
//
//   The effective control value is preserved in control_.
//
//   The original CPU write value is recorded separately, allowing the
//   debugger to distinguish CPU writes from bus-conflict-modified values.
//
//   The debugger reports the selected PRG-ROM bank using bits 0-3,
//   supporting bank numbers $00-$0F.
//
// Parameters:
//   address - CPU address being written ($8000-$FFFF).
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper7::write_handler(uint_least16_t address, uint8_t value)
{
	const uint8_t original_value = value;

	/*
	 * Apply AND-type bus conflicts when enabled.
	 *
	 * Read the currently mapped PRG-ROM byte before changing
	 * the active 32 KB bank.
	 */
	if (bus_conflicts_) {
		const uint8_t rom_value = read_memory(address);

		value &= rom_value;
	}

	/*
	 * Preserve the effective mapper-control register.
	 */
	control_ = value;

	/*
	 * Record mapper-write activity for debugger inspection.
	 *
	 * The last-write value represents what the CPU originally
	 * placed on the data bus, before any bus conflict.
	 */
	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = original_value;

	/*
 	 * Bits 0-3 select the complete 32 KB PRG-ROM bank.
 	 *
 	 * Standard AxROM uses only bits 0-2, supporting up to 256 KB.
 	 * Oversized AxROM uses bit 3 to access up to 512 KB.
	 *
 	 * The base mapper's PRG mask wraps unused bank bits to the
 	 * available cartridge ROM size.
	 *
 	 * Bit 4 remains reserved for single-screen mirroring.
 	 */
	set_prg_89abcdef(control_ & 0x0f);

	/*
	 * Bit 4 selects which CIRAM page backs all four
	 * logical nametables.
	 *
	 * Use the effective control value, since bus conflicts
	 * can also affect the mirroring-selection bit.
	 */
	if (control_ & 0x10) {
		set_mirroring(mirror_single_high);
	} else {
		set_mirroring(mirror_single_low);
	}
}




