
#include "Mapper034.h"
#include "Cart.h"
#include "Nes.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(34)


// -----------------------------------------------------------------------------
// Mapper34::Mapper34
//
// Initializes Mapper 34 for either NINA-001 or BNROM hardware.
//
// NINA-001:
//   - 32 KB switchable PRG ROM.
//   - Two independently switchable 4 KB CHR-ROM banks.
//   - 8 KB PRG RAM mapped at CPU $6000-$7FFF.
//   - Bank-selection registers at $7FFD-$7FFF.
//
// BNROM:
//   - 32 KB switchable PRG ROM.
//   - 8 KB CHR RAM mapped at PPU $0000-$1FFF.
//   - Optional 8 KB PRG RAM when declared by the cartridge.
//   - AND-type bus conflicts on PRG-ROM bank-selection writes.
//
// For compatibility with existing ROMs, the hardware variant is currently
// determined by the presence of CHR ROM. Explicit NES 2.0 submapper selection
// would be preferable for cartridges that provide that information.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper34::Mapper34()
{
	// Select the hardware variant using the existing CHR-ROM heuristic.
	// NINA-001 uses CHR ROM, while BNROM uses CHR RAM.
	mode_ = nes::cart.has_chr_rom() ? NINA_001 : BNROM;

	// Obtain the volatile PRG-RAM size declared by the cartridge.
	// NINA-001 always enables its 8 KB PRG RAM. BNROM enables PRG RAM
	// only when the header declares a compatible 8 KB allocation.
	const uint32_t declared_ram_size = nes::cart.prg_ram_size();

	prg_ram_enabled_ = (mode_ == NINA_001)
		|| (declared_ram_size == sizeof(prg_ram_));

	// Map the optional PRG RAM into CPU $6000-$7FFF and update the
	// mapper debugger's memory-access information.
	if (prg_ram_enabled_) {
		swap_67(prg_ram_);
		debug_set_prg_ram(true, true, 0);
	} else {
		debug_set_prg_ram(false, false, 0);
	}

	if (mode_ == NINA_001) {
		// Initialize the two 16 KB PRG-ROM windows.
		// The lower window starts at bank 0, while the upper window
		// selects the final available 16 KB bank.
		set_prg_89ab(0);
		set_prg_cdef(-1);

		// Initialize the 8 KB CHR-ROM mapping.
		// Subsequent writes to $7FFE/$7FFF select independent 4 KB banks.
		set_chr_0000_1fff(0);
	} else {
		// Initialize BNROM PRG-ROM mapping.
		// Bank switching subsequently controls the entire 32 KB window.
		set_prg_89ab(0);
		set_prg_cdef(-1);

		// BNROM has no CHR ROM. Provide writable 8 KB CHR RAM.
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}
}


// -----------------------------------------------------------------------------
// Mapper34::name
//
// Returns the name of the active Mapper 34 hardware variant.
//
// The name is used by the emulator and mapper debugger to identify
// whether the loaded cartridge uses NINA-001 or BNROM banking.
//
// Parameters:
//   None.
//
// Returns:
//   Hardware variant name as a string.
// -----------------------------------------------------------------------------
std::string
Mapper34::name() const
{
	switch (mode_) {
	case NINA_001:
		return "NINA-001";

	case BNROM:
		return "BNROM";
	}

	// Fallback for an unrecognized hardware mode.
	return "NINA-001/BNROM";
}


// -----------------------------------------------------------------------------
// Mapper34::write_6
//
// Handles CPU writes to the $6000-$6FFF address range.
//
// Both NINA-001 and supported BNROM variants may provide cartridge PRG RAM
// in this region. Writes are stored only when PRG RAM is enabled.
//
// The address is masked to obtain an offset within the 8 KB RAM buffer.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_6 (uint_least16_t address, uint8_t value)
{
	if (prg_ram_enabled_) {
		prg_ram_[address & 0x1fff] = value;
	}
}


// -----------------------------------------------------------------------------
// Mapper34::write_7
//
// Handles CPU writes to the $7000-$7FFF address range.
//
// When cartridge PRG RAM is enabled, all writes update the corresponding
// byte in the 8 KB PRG-RAM buffer.
//
// NINA-001 additionally decodes three bank-selection registers:
//
//   $7FFD - Select the 32 KB PRG-ROM bank at $8000-$FFFF.
//   $7FFE - Select the lower 4 KB CHR-ROM bank at $0000-$0FFF.
//   $7FFF - Select the upper 4 KB CHR-ROM bank at $1000-$1FFF.
//
// Writes to these registers also update the underlying PRG RAM.
//
// On BNROM, the entire region is ordinary PRG RAM when present; no
// additional bank-selection operations are performed.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_7 (uint_least16_t address, uint8_t value)
{
	// Store the value in cartridge PRG RAM when available.
	if (prg_ram_enabled_) {
		prg_ram_[address & 0x1fff] = value;
	}

	// NINA-001 provides three bank-selection registers in this range.
	// BNROM does not decode these addresses for bank switching.
	if (mode_ == NINA_001) {
		switch (address) {
		case 0x7ffd:
			// Select the complete 32 KB PRG-ROM window.
			set_prg_89abcdef(value);
			break;

		case 0x7ffe:
			// Select the lower 4 KB CHR-ROM window.
			set_chr_0000_0fff(value);
			break;

		case 0x7fff:
			// Select the upper 4 KB CHR-ROM window.
			set_chr_1000_1fff(value);
			break;

		default:
			// All other addresses are ordinary PRG RAM.
			break;
		}
	}
}


// -----------------------------------------------------------------------------
// Mapper34::write_8
//
// Handles CPU writes to the $8000-$8FFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_8 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_9
//
// Handles CPU writes to the $9000-$9FFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_9 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_a
//
// Handles CPU writes to the $A000-$AFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_a (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_b
//
// Handles CPU writes to the $B000-$BFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_b (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_c
//
// Handles CPU writes to the $C000-$CFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_c (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_d
//
// Handles CPU writes to the $D000-$DFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_d (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_e
//
// Handles CPU writes to the $E000-$EFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_e (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_f
//
// Handles CPU writes to the $F000-$FFFF address range.
//
// Forwards the write to the shared BNROM bank-selection handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_f (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper34::write_handler
//
// Handles PRG-ROM bank-selection writes for BNROM.
//
// BNROM selects a 32 KB PRG-ROM bank through CPU writes to any address
// in the $8000-$FFFF range.
//
// Original BNROM hardware exhibits AND-type bus conflicts. During a
// bank-selection write, both the CPU and the currently mapped PRG ROM
// drive the data bus. The value seen by the bank register is therefore:
//
//   effective_value = CPU_write_value & currently_mapped_ROM_byte
//
// The ROM byte must be read before changing the bank mapping, because
// the bus conflict is determined by the previously selected PRG-ROM bank.
//
// NINA-001 does not use this register range for PRG bank selection.
// Writes are ignored when the mapper is operating in NINA-001 mode.
//
// Parameters:
//   address - CPU address being written.
//   value   - Original value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper34::write_handler (uint_least16_t address, uint8_t value)
{
	if (mode_ == BNROM) {
		// Read the ROM byte currently driving the CPU data bus.
		const uint8_t rom_value = read_memory(address);

		// Apply the AND-type bus conflict before switching banks.
		const uint8_t effective_value = value & rom_value;

		// Select the resulting 32 KB PRG-ROM bank.
		set_prg_89abcdef(effective_value);
	}
}


