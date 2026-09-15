#include "Mapper001.h"
#include "Cart.h"
#include "Nes.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(1)

namespace {

enum {
	Control  = 0x00,
	ChrBank0 = 0x01,
	ChrBank1 = 0x02,
	PrgBank  = 0x03,
};

}

// TODO(eteran): how the heck do we support the "256KB PRG ROM bank" of S[OUX]ROM?
// does that mean the chip # or something?

//------------------------------------------------------------------------------
// Name: Mapper1
//
// Initializes mapper 1 (Nintendo MMC1).
//
// MMC1 uses a serial 5-bit shift-register interface to control:
//
//   - Nametable mirroring.
//   - CHR banking.
//   - PRG banking.
//   - PRG-RAM enable/disable state.
//
// The mapper starts with PRG RAM enabled, the first 16 KB PRG bank mapped at
// $8000-$BFFF, and the final 16 KB PRG bank mapped at $C000-$FFFF.
//
// CHR memory is initialized as either cartridge CHR ROM or 8 KB of CHR RAM.
//
// Finally, the mapper is placed into its reset state by passing a reset write
// through the normal MMC1 write handler.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper1::Mapper1()
{
	// Open the 8 KB battery-backed/save-RAM region used at $6000-$7FFF.
	prg_ptr_ = open_sram(0x2000);

	// MMC1 PRG RAM occupies $6000-$7FFF and starts enabled.
	debug_set_prg_ram(true);

	// Initial PRG mapping:
	//
	//   $8000-$BFFF -> first 16 KB PRG bank
	//   $C000-$FFFF -> final 16 KB PRG bank
	set_prg_89ab(0);
	set_prg_cdef(-1);

	if (nes::cart.has_chr_rom()) {
		// Map the first 8 KB CHR-ROM bank at $0000-$1FFF.
		set_chr_0000_1fff(0);
	} else {
		// Cartridges without CHR ROM use 8 KB of CHR RAM.
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}

	// Reset the MMC1 serial register and force the control register into
	// its reset PRG-banking mode.
	write_handler(0x8000, 0x80);
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
//   "Nintendo MMC1".
//------------------------------------------------------------------------------
std::string
Mapper1::name() const
{
	return "Nintendo MMC1";
}


//------------------------------------------------------------------------------
// Name: read_6
//
// Reads from CPU addresses $6000-$6FFF.
//
// When bit 4 of the MMC1 PRG-bank register is clear, PRG RAM is enabled and
// the read is serviced from the mapper's 8 KB save-RAM region.
//
// Otherwise, the read falls back to the base mapper implementation.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   The byte read from PRG RAM or the base mapper.
//------------------------------------------------------------------------------
uint8_t
Mapper1::read_6 (uint_least16_t address)
{
	if (!(regs_[PrgBank] & 0b10000)) {
		return prg_ptr_[address & 0x1fff];
	}

	return Mapper::read_6(address);
}


//------------------------------------------------------------------------------
// Name: read_7
//
// Reads from CPU addresses $7000-$7FFF.
//
// The behavior is identical to read_6(); both ranges occupy the same 8 KB
// PRG-RAM window.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   The byte read from PRG RAM or the base mapper.
//------------------------------------------------------------------------------
uint8_t
Mapper1::read_7 (uint_least16_t address)
{
	if (!(regs_[PrgBank] & 0b10000)) {
		return prg_ptr_[address & 0x1fff];
	}

	return Mapper::read_7(address);
}


//------------------------------------------------------------------------------
// Name: write_6
//
// Writes to CPU addresses $6000-$6FFF.
//
// Writes are accepted only while MMC1 PRG RAM is enabled.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte to store.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper1::write_6 (uint_least16_t address, uint8_t value)
{
	if (!(regs_[PrgBank] & 0b10000)) {
		prg_ptr_[address & 0x1fff] = value;
	}
}


//------------------------------------------------------------------------------
// Name: write_7
//
// Writes to CPU addresses $7000-$7FFF.
//
// This is the upper half of the same 8 KB PRG-RAM region handled by write_6().
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte to store.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper1::write_7 (uint_least16_t address, uint8_t value)
{
	if (!(regs_[PrgBank] & 0b10000)) {
		prg_ptr_[address & 0x1fff] = value;
	}
}


//------------------------------------------------------------------------------
// Name: write_8
//
// Handles MMC1 register writes in the $8000-$8FFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper1::write_8 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Handles MMC1 register writes in the $9000-$9FFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_9 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles MMC1 register writes in the $A000-$AFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_a (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Handles MMC1 register writes in the $B000-$BFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_b (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Handles MMC1 register writes in the $C000-$CFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_c (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Handles MMC1 register writes in the $D000-$DFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_d (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Handles MMC1 register writes in the $E000-$EFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_e (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Handles MMC1 register writes in the $F000-$FFFF range.
//------------------------------------------------------------------------------
void
Mapper1::write_f (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


//------------------------------------------------------------------------------
// Name: write_handler
//
// Processes MMC1 serial register writes.
//
// MMC1 accepts one data bit per CPU write. Five accepted writes are shifted
// into a temporary latch and then committed to one of four internal registers:
//
//   $8000-$9FFF -> Control
//   $A000-$BFFF -> CHR bank 0
//   $C000-$DFFF -> CHR bank 1
//   $E000-$FFFF -> PRG bank
//
// A write with bit 7 set resets the serial latch and forces the Control
// register into the MMC1 reset PRG-banking configuration.
//
// The implementation also ignores writes that occur too close together,
// matching the MMC1 consecutive-cycle write behavior.
//
// Parameters:
//   address - CPU address written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper1::write_handler (uint_least16_t address, uint8_t value)
{
	// Ignore writes that occur only one or two CPU cycles after the previous
	// accepted mapper write.
	if (nes::cpu::cycle_count() - cpu_cycles_ > 1) {
		// Select the MMC1 register region from the CPU address.
		uint8_t bank = (address >> 12) & 0xf;

		if (value & 0x80) {
			// Bit 7 resets the serial interface immediately.
			latch_         = 0;
			write_counter_ = 5;

			// Force Control bits 2 and 3 high, selecting the MMC1 reset
			// PRG-banking mode.
			regs_[Control] |= 0x0c;

			// Treat the reset as a Control-register update.
			bank = 0;
		} else {
			// MMC1 serial writes use only bit 0 of each CPU write.
			value &= 1;

			// Shift the new bit into the current serial position.
			latch_ |= (value << write_counter_);

			++write_counter_;
		}

		// Five serial writes complete one MMC1 register value.
		if (write_counter_ == 5) {
			if (bank > 0) {
				// Convert the CPU register region into the corresponding
				// MMC1 register index and commit the 5-bit value.
				regs_[(bank & 0xf7) >> 1] = (latch_ & 0x1f);
			}

			// Reset the serial interface for the next 5-bit transfer.
			latch_         = 0;
			write_counter_ = 0;

			// Control bits 0-1 select nametable mirroring.
			switch (regs_[Control] & 0x03) {
			case 0:
				set_mirroring(mirror_single_low);
				break;

			case 1:
				set_mirroring(mirror_single_high);
				break;

			case 2:
				set_mirroring(mirror_vertical);
				break;

			case 3:
				set_mirroring(mirror_horizontal);
				break;
			}

			if (nes::cart.has_chr_rom()) {
				// Control bit 4 selects 4 KB or 8 KB CHR banking.
				if (regs_[Control] & 0x10) {
					// 4 KB mode: each pattern-table half is independently
					// selected by one CHR bank register.
					set_chr_0000_0fff(regs_[ChrBank0]);
					set_chr_1000_1fff(regs_[ChrBank1]);
				} else {
					// 8 KB mode: CHR bank 0 selects the complete pattern
					// table; the low bank bit is ignored.
					set_chr_0000_1fff(regs_[ChrBank0] >> 1);
				}
			} else {
				// NOTE(eteran): this is for SNROM, we may need iNES 2.0
				// to detect SOROM, SUROM and SXROM.

				if (regs_[Control] & 0x10) {
					// In 4 KB CHR-RAM mode, the low bank bit selects the
					// active 4 KB region for each pattern-table half.
					set_chr_0000_0fff_ram(
						chr_ram_,
						regs_[ChrBank0] & 1);

					set_chr_1000_1fff_ram(
						chr_ram_,
						regs_[ChrBank1] & 1);
				}

				// These bits are used by some MMC1 board variants to control
				// PRG-RAM selection/enable behavior.
				prg_ram_enable0_ = regs_[ChrBank0] & 0x10;
				prg_ram_enable1_ = regs_[ChrBank1] & 0x10;
			}

			// Control bits 2-3 select the PRG banking mode.
			switch ((regs_[Control] >> 2) & 0x03) {
			case 0x00:
			case 0x01:
				// 32 KB mode. Ignore the low PRG-bank bit.
				set_prg_89abcdef(
					(regs_[PrgBank] & 0x0f) >> 1);
				break;

			case 0x02:
				// Fix the first 16 KB PRG bank at $8000-$BFFF and
				// switch the 16 KB bank at $C000-$FFFF.
				set_prg_89ab(0x00);
				set_prg_cdef(regs_[PrgBank] & 0x0f);
				break;

			case 0x03:
				// Switch the 16 KB bank at $8000-$BFFF and fix the
				// final 16 KB PRG bank at $C000-$FFFF.
				set_prg_89ab(regs_[PrgBank] & 0x0f);
				set_prg_cdef(0x0f);
				break;
			}

			// Bit 4 of the PRG-bank register controls the effective PRG-RAM
			// visibility used by this implementation.
			debug_set_prg_ram(!(regs_[PrgBank] & 0x10));
		}
	}

	// Remember the cycle of this write for MMC1 consecutive-write filtering.
	cpu_cycles_ = nes::cpu::cycle_count();
}
