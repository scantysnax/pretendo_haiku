
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
Mapper3::Mapper3 (bool bus_conflicts)
	: bus_conflicts_(bus_conflicts)
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


// -----------------------------------------------------------------------------
// Mapper3::select_chr_bank
//
// Handles CNROM CHR-ROM bank-selection writes throughout $8000-$FFFF.
//
// CNROM provides a switchable 8 KB CHR-ROM bank selected by writes to the
// PRG-ROM address space.
//
// On boards with AND-type bus conflicts (NES 2.0 submapper 2), the CPU and
// currently mapped PRG-ROM simultaneously drive the data bus during writes.
// The effective bank-selection value is the bitwise AND of the CPU write
// value and the ROM byte at the destination address.
//
// The ROM byte is sampled before updating the CHR-ROM bank mapping.
//
// Boards without bus conflicts (submapper 1) use the original CPU write
// value directly. Submapper 0 retains Pretendo's legacy behavior.
//
// Updates the active CHR-ROM bank and persistent mapper-write diagnostics.
// The debugger retains the original CPU write value separately from the
// effective bank-selection value.
//
// Parameters:
//   address - CPU address being written ($8000-$FFFF).
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::select_chr_bank (uint_least16_t address, uint8_t value)
{
	const uint8_t original_value = value;

	if (bus_conflicts_) {
		const uint8_t rom_value = read_memory(address);
		value &= rom_value;
	}

	bank_select_ = value;

	++debug_write_count_;

	debug_have_last_write_ = true;
	debug_last_write_address_ = address;
	debug_last_write_value_ = original_value;

	set_chr_0000_1fff(value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_8
//
// Handles CNROM CHR-ROM bank-selection writes at $8000-$8FFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_8 (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_9
//
// Handles CNROM CHR-ROM bank-selection writes at $9000-$9FFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_9 (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_a
//
// Handles CNROM CHR-ROM bank-selection writes at $A000-$AFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_a (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_b
//
// Handles CNROM CHR-ROM bank-selection writes at $B000-$BFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_b (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_c
//
// Handles CNROM CHR-ROM bank-selection writes at $C000-$CFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_c (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_d
//
// Handles CNROM CHR-ROM bank-selection writes at $D000-$DFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_d (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_e
//
// Handles CNROM CHR-ROM bank-selection writes at $E000-$EFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_e (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}


// -----------------------------------------------------------------------------
// Mapper3::write_f
//
// Handles CNROM CHR-ROM bank-selection writes at $F000-$FFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original CPU write value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper3::write_f (uint_least16_t address, uint8_t value)
{
	select_chr_bank(address, value);
}

