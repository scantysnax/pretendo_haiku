
#include "Mapper009.h"
#include "Cart.h"
#include "Nes.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(9)



//------------------------------------------------------------------------------
// Name: Mapper9
//
// Initializes mapper 9 (Nintendo MMC2 / PxROM).
//
// MMC2 provides:
//
//   - One switchable 8 KB PRG-ROM bank at $8000-$9FFF.
//   - Three fixed 8 KB PRG-ROM banks at $A000-$FFFF.
//   - Two independently latched 4 KB CHR-ROM regions.
//   - Mapper-controlled horizontal/vertical mirroring.
//   - No PRG RAM in this implementation.
//
// CHR bank selection is unusual on MMC2: each 4 KB pattern-table half has two
// possible banks, and PPU reads from specific trigger addresses select which
// bank is currently active.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper9::Mapper9()
{
	// MMC2/PxROM does not provide PRG RAM.
	unmap_67();

	// Initial PRG mapping:
	//
	//   $8000-$9FFF -> first switchable 8 KB PRG bank
	//   $A000-$BFFF -> third-to-last 8 KB PRG bank
	//   $C000-$DFFF -> second-to-last 8 KB PRG bank
	//   $E000-$FFFF -> final 8 KB PRG bank
	set_prg_89(0);
	set_prg_ab(-3);
	set_prg_cd(-2);
	set_prg_ef(-1);

	if (nes::cart.has_chr_rom()) {
		// Start with the first 8 KB of CHR ROM mapped at $0000-$1FFF.
		set_chr_0000_1fff(0);
	} else {
		// Fallback for cartridges without CHR ROM.
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}

	// MMC2 controls mirroring through its mapper register. Until the first
	// mirroring write occurs, the effective mapper-controlled state is unknown.
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
//   "PxROM (Nintendo MMC2)".
//------------------------------------------------------------------------------
std::string
Mapper9::name() const
{
	return "PxROM (Nintendo MMC2)";
}

// -----------------------------------------------------------------------------
// Mapper9::debug_state_mmc2
//
// Returns a snapshot of MMC2-specific internal state for debugger inspection.
//
// The snapshot includes the programmed PRG and CHR bank registers, current
// latch states, currently selected CHR banks, and persistent latch-trigger
// activity counters.
//
// Parameters:
//   None.
//
// Returns:
//   Current MMC2-specific debugger state.
// -----------------------------------------------------------------------------
mmc2_debug_state_t
Mapper9::debug_state() const
{
	mmc2_debug_state_t state;

	state.prg_bank = prg_bank_;

	state.latch0_lo = latch0_lo_ & 0x1f;
	state.latch0_hi = latch0_hi_ & 0x1f;
	state.latch1_lo = latch1_lo_ & 0x1f;
	state.latch1_hi = latch1_hi_ & 0x1f;

	state.latch0 = latch0_;
	state.latch1 = latch1_;

	state.active_chr0_bank = latch0_ ? (latch0_hi_ & 0x1f) : (latch0_lo_ & 0x1f);
	state.active_chr1_bank = latch1_ ? (latch1_hi_ & 0x1f) : (latch1_lo_ & 0x1f);
	state.latch0_low_count = debug_latch0_low_count_;
	state.latch0_high_count = debug_latch0_high_count_;
	state.latch1_low_count = debug_latch1_low_count_;
	state.latch1_high_count = debug_latch1_high_count_;
	state.have_last_trigger = debug_have_last_trigger_;
	state.last_trigger_address = debug_last_trigger_address_;

	return state;
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles writes to the MMC2 PRG-bank register.
//
// The low four bits select the 8 KB PRG-ROM bank mapped at $8000-$9FFF.
// The three upper PRG slots remain fixed.
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - PRG-ROM bank-selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_a (uint_least16_t address, uint8_t value)
{
	(void)address;

	prg_bank_ = value & 0x0f;

	set_prg_89(prg_bank_);
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Programs the low-latch CHR bank for PPU addresses $0000-$0FFF.
//
// The selected bank becomes visible immediately only when latch 0 is currently
// in its low state. Otherwise the value is stored until the PPU later triggers
// the low latch through address $0FD8.
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - 4 KB CHR-bank selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_b (uint_least16_t address, uint8_t value)
{
	(void)address;

	latch0_lo_ = value & 0x1f;

	if (!latch0_) {
		set_chr_0000_0fff(latch0_lo_);
	}
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Programs the high-latch CHR bank for PPU addresses $0000-$0FFF.
//
// The selected bank becomes visible immediately only when latch 0 is currently
// in its high state. Otherwise the value is stored until the PPU later
// triggers the high latch through address $0FE8.
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - 4 KB CHR-bank selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_c (uint_least16_t address, uint8_t value)
{
	(void)address;

	latch0_hi_ = value & 0x1f;

	if (latch0_) {
		set_chr_0000_0fff(latch0_hi_);
	}
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Programs the low-latch CHR bank for PPU addresses $1000-$1FFF.
//
// The selected bank becomes visible immediately only when latch 1 is currently
// in its low state. Otherwise the value is stored until the corresponding
// PPU latch-trigger address is read.
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - 4 KB CHR-bank selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_d (uint_least16_t address, uint8_t value)
{
	(void)address;

	latch1_lo_ = value & 0x1f;

	if (!latch1_) {
		set_chr_1000_1fff(latch1_lo_);
	}
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Programs the high-latch CHR bank for PPU addresses $1000-$1FFF.
//
// The selected bank becomes visible immediately only when latch 1 is currently
// in its high state. Otherwise the value is stored until the corresponding
// PPU latch-trigger address is read.
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - 4 KB CHR-bank selection value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_e (uint_least16_t address, uint8_t value)
{
	(void)address;

	latch1_hi_ = value & 0x1f;

	if (latch1_) {
		set_chr_1000_1fff(latch1_hi_);
	}
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Controls MMC2 nametable mirroring.
//
// Mirroring selection:
//
//   bit 0 clear -> vertical mirroring
//   bit 0 set   -> horizontal mirroring
//
// Parameters:
//   address - CPU address being written. The address itself is not used.
//   value   - Mirroring control value.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
Mapper9::write_f (uint_least16_t address, uint8_t value)
{
	(void)address;

	if (value & 0x01) {
		set_mirroring(mirror_horizontal);
	} else {
		set_mirroring(mirror_vertical);
	}
}


//------------------------------------------------------------------------------
// Name: read_vram
//
// Reads one byte from PPU VRAM and updates the MMC2 CHR latches when one of
// the hardware trigger addresses is accessed.
//
// MMC2 divides CHR space into two independently controlled 4 KB regions:
//
//   $0000-$0FFF -> latch 0
//   $1000-$1FFF -> latch 1
//
// Certain pattern-table reads toggle each latch and immediately remap the
// corresponding 4 KB CHR region:
//
//   $0FD8       -> latch 0 low
//   $0FE8       -> latch 0 high
//
//   $1FD8-$1FDF -> latch 1 low
//   $1FE8-$1FEF -> latch 1 high
//
// The VRAM byte is read before the latch-induced bank switch is applied.
//
// Debugger-only counters record each latch trigger and preserve the most recent
// trigger address.
//
// Parameters:
//   address - PPU VRAM address being read.
//
// Returns:
//   The byte read from the mapping that was active when the access began.
//------------------------------------------------------------------------------
uint8_t
Mapper9::read_vram (uint_least16_t address)
{
	// Complete the current VRAM read before allowing the access to change
	// the MMC2 CHR latch state.
	const uint8_t ret = Mapper::read_vram(address);

	switch (address) {
	case 0x0fd8:
		// Select the low CHR bank for $0000-$0FFF.
		set_chr_0000_0fff(latch0_lo_);
		latch0_ = false;

		++debug_latch0_low_count_;
		debug_have_last_trigger_ = true;
		debug_last_trigger_address_ = address;
		break;

	case 0x0fe8:
		// Select the high CHR bank for $0000-$0FFF.
		set_chr_0000_0fff(latch0_hi_);
		latch0_ = true;

		++debug_latch0_high_count_;
		debug_have_last_trigger_ = true;
		debug_last_trigger_address_ = address;
		break;

	case 0x1fd8:
	case 0x1fd9:
	case 0x1fda:
	case 0x1fdb:
	case 0x1fdc:
	case 0x1fdd:
	case 0x1fde:
	case 0x1fdf:
		// Select the low CHR bank for $1000-$1FFF.
		set_chr_1000_1fff(latch1_lo_);
		latch1_ = false;

		++debug_latch1_low_count_;
		debug_have_last_trigger_ = true;
		debug_last_trigger_address_ = address;
		break;

	case 0x1fe8:
	case 0x1fe9:
	case 0x1fea:
	case 0x1feb:
	case 0x1fec:
	case 0x1fed:
	case 0x1fee:
	case 0x1fef:
		// Select the high CHR bank for $1000-$1FFF.
		set_chr_1000_1fff(latch1_hi_);
		latch1_ = true;

		++debug_latch1_high_count_;
		debug_have_last_trigger_ = true;
		debug_last_trigger_address_ = address;
		break;
	}

	return ret;
}
