
#include "Mapper075.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(75)


// -----------------------------------------------------------------------------
// Mapper75::Mapper75
//
// Initializes mapper 75 as Konami VRC1.
//
// The initial PRG mapping exposes the first three 8 KB PRG banks in the
// switchable CPU regions while keeping the final PRG bank fixed at
// $E000-$FFFF.
//
// The two 4 KB CHR registers are initialized to banks 0 and 1 so their shadow
// state matches the initial contiguous 8 KB CHR mapping.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper75::Mapper75()
{
	set_prg_89abcdef(0);
	set_prg_ef(-1);

	chr_reg_[0] = 0;
	chr_reg_[1] = 1;

	set_chr_0000_0fff(chr_reg_[0]);
	set_chr_1000_1fff(chr_reg_[1]);
}


//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
std::string
Mapper75::name() const
{
	return "Konami VRC1";
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void
Mapper75::write_8 (uint_least16_t address, uint8_t value)
{
	(void)address;
	set_prg_89(value & 0x0f);
}


// -----------------------------------------------------------------------------
// Mapper75::write_9
//
// Handles writes to the VRC1 $9000 register range.
//
// Bit 0 controls nametable mirroring.  Bits 1 and 2 provide the high bank bits
// for the two 4 KB CHR ROM banks.
//
// Parameters:
//   address - CPU address being written.
//   value   - Mirroring and CHR high-bit control value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper75::write_9(uint_least16_t address, uint8_t value)
{
	(void)address;

	control_ = value;

	if (value & 0x01) {
		set_mirroring(mirror_horizontal);
	} else {
		set_mirroring(mirror_vertical);
	}

	chr_reg_[0] = (chr_reg_[0] & 0x0f) | ((value << 3) & 0x10);
	chr_reg_[1] = (chr_reg_[1] & 0x0f) | ((value << 2) & 0x10);

	set_chr_0000_0fff(chr_reg_[0]);
	set_chr_1000_1fff(chr_reg_[1]);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper75::write_a(uint_least16_t address, uint8_t value)
{
	(void)address;
	set_prg_ab(value & 0x0f);
}


//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper75::write_c(uint_least16_t address, uint8_t value)
{
	(void)address;
	set_prg_cd(value & 0x0f);
}


// -----------------------------------------------------------------------------
// Mapper75::write_e
//
// Handles writes to the VRC1 $E000 register range.
//
// The low four bits update the low nibble of the 4 KB CHR bank mapped at
// $0000-$0FFF.  The high bank bit is preserved from the $9000 control register.
//
// Parameters:
//   address - CPU address being written.
//   value   - Low four bits of the CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper75::write_e(uint_least16_t address, uint8_t value)
{
	(void)address;
	chr_reg_[0] = (chr_reg_[0] & 0xf0) | (value & 0x0f);

	set_chr_0000_0fff(chr_reg_[0]);
}


// -----------------------------------------------------------------------------
// Mapper75::write_f
//
// Handles writes to the VRC1 $F000 register range.
//
// The low four bits update the low nibble of the 4 KB CHR bank mapped at
// $1000-$1FFF.  The high bank bit is preserved from the $9000 control register.
//
// Parameters:
//   address - CPU address being written.
//   value   - Low four bits of the CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper75::write_f(uint_least16_t address, uint8_t value)
{
	(void)address;
	chr_reg_[1] = (chr_reg_[1] & 0xf0) | (value & 0x0f);

	set_chr_1000_1fff(chr_reg_[1]);
}


// -----------------------------------------------------------------------------
// Mapper75::debug_mapper_state
//
// Returns a debugger-facing snapshot of VRC1-specific mapper state.
//
// The snapshot exposes the two raw 5-bit CHR bank registers and the current
// $9000 control register.  Resolved PRG/CHR mappings are handled separately by
// the generic mapper debugger state.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC1 mapper-specific debugger state.
// -----------------------------------------------------------------------------
vrc1_debug_state_t
Mapper75::debug_mapper_state() const
{
	vrc1_debug_state_t state;

	state.chr[0] = chr_reg_[0];
	state.chr[1] = chr_reg_[1];

	state.control = control_;
	state.horizontal_mirroring = (control_ & 0x01) != 0;

	return state;
}
