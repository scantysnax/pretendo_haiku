
#include "VRC2.h"

// TODO(eteran): implement sub-mapper madness...

// -----------------------------------------------------------------------------
// VRC2::VRC2
//
// Initializes the VRC2 mapper core.
//
// The constructor establishes the default PRG and CHR mappings and initializes
// the eight raw CHR bank registers to a contiguous sequence.  chr_shift selects
// the hardware-specific CHR bank scaling used by the particular VRC2 variant.
//
// Parameters:
//   chr_shift - Number of bits applied when converting raw CHR register values
//               into effective 1 KB CHR bank numbers.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VRC2::VRC2(int chr_shift)
	: chr_shift_(chr_shift) {

	set_prg_89ab(0);
	set_prg_cdef(-1);

	set_chr_0000_1fff(0);

	chr_[0] = 0;
	chr_[1] = 1;
	chr_[2] = 2;
	chr_[3] = 3;
	chr_[4] = 4;
	chr_[5] = 5;
	chr_[6] = 6;
	chr_[7] = 7;
}


// -----------------------------------------------------------------------------
// VRC2::name
//
// Returns the generic mapper hardware name for the VRC2 core.
//
// Derived mapper implementations override this when they represent a specific
// VRC2 hardware variant such as VRC2a, VRC2b, or VRC2c.
//
// Parameters:
//   None.
//
// Returns:
//   Generic mapper hardware name.
// -----------------------------------------------------------------------------
std::string
VRC2::name() const {
	return "VRC2";
}


// -----------------------------------------------------------------------------
// VRC2::read_6
//
// Reads the VRC2 one-bit latch through the CPU $6000-$6FFF range.
//
// The current latch bit is combined with the upper portion of the CPU address,
// reproducing the simple latch/open-bus behavior used by VRC2 hardware.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value formed from the address high byte and the current latch bit.
// -----------------------------------------------------------------------------
uint8_t
VRC2::read_6 (uint_least16_t address)
{
	return (address >> 8) | latch_;
}


// -----------------------------------------------------------------------------
// VRC2::write_6
//
// Updates the VRC2 one-bit latch mapped through the CPU $6000-$6FFF range.
//
// The latch stores only bit 0 of the written value.  Debugger activity counters
// track both the total number of latch writes and the number of writes that
// actually change the stored latch bit.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the latch.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_6 (uint_least16_t address, uint8_t value)
{
	(void)address;

	++latch_write_count_;

	const uint8_t oldLatch = latch_;
	const uint8_t newLatch = (latch_ & 0xfe) | (value & 0x01);

	if (newLatch != oldLatch) {
		++latch_change_count_;
	}

	latch_ = newLatch;
}


// -----------------------------------------------------------------------------
// VRC2::write_8
//
// Handles writes to the VRC2 $8000 register range.
//
// The written value selects the switchable 8 KB PRG bank mapped at
// $8000-$9FFF.  Only the low five bits are used as the bank number.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0x8000:
	case 0x8001:
	case 0x8002:
	case 0x8003:
		set_prg_89(value & 0x1f);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_9
//
// Handles writes to the VRC2 $9000 register range.
//
// Bit 0 selects the nametable mirroring mode.  VRC2 supports only vertical and
// horizontal mirroring; bit 1 is ignored.  This differs from VRC4, which also
// supports the two single-screen mirroring modes.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value selecting the mirroring mode.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_9(uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0x9000:
	case 0x9001:
	case 0x9002:
	case 0x9003:
		if (value & 0x01) {
			set_mirroring(mirror_horizontal);
		} else {
			set_mirroring(mirror_vertical);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_a
//
// Handles writes to the VRC2 $A000 register range.
//
// The written value selects the switchable 8 KB PRG bank mapped at
// $A000-$BFFF.  Only the low five bits are used as the bank number.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xa000:
	case 0xa001:
	case 0xa002:
	case 0xa003:
		set_prg_ab(value & 0x1f);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_b
//
// Handles writes to the first two VRC2 CHR bank registers.
//
// Each 1 KB CHR bank number is assembled from separate low- and high-nibble
// writes.  The raw register value is then shifted according to chr_shift_
// before being mapped into PPU ranges $0000-$03FF and $0400-$07FF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Value containing the CHR bank nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
VRC2::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xb000:
		chr_[0] = (chr_[0] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_0000_03ff(chr_[0] >> chr_shift_);
		break;
	case 0xb001:
		chr_[0] = (chr_[0] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0000_03ff(chr_[0] >> chr_shift_);
		break;
	case 0xb002:
		chr_[1] = (chr_[1] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_0400_07ff(chr_[1] >> chr_shift_);
		break;
	case 0xb003:
		chr_[1] = (chr_[1] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0400_07ff(chr_[1] >> chr_shift_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_c
//
// Handles writes to VRC2 CHR bank registers 2 and 3.
//
// Each 1 KB CHR bank number is assembled from separate low- and high-nibble
// writes.  The raw register value is then shifted according to chr_shift_
// before being mapped into PPU ranges $0800-$0BFF and $0C00-$0FFF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Value containing the CHR bank nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xc000:
		chr_[2] = (chr_[2] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_0800_0bff(chr_[2] >> chr_shift_);
		break;
	case 0xc001:
		chr_[2] = (chr_[2] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0800_0bff(chr_[2] >> chr_shift_);
		break;
	case 0xc002:
		chr_[3] = (chr_[3] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_0c00_0fff(chr_[3] >> chr_shift_);
		break;
	case 0xc003:
		chr_[3] = (chr_[3] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0c00_0fff(chr_[3] >> chr_shift_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_d
//
// Handles writes to VRC2 CHR bank registers 4 and 5.
//
// Each 1 KB CHR bank number is assembled from separate low- and high-nibble
// writes.  The raw register value is then shifted according to chr_shift_
// before being mapped into PPU ranges $1000-$13FF and $1400-$17FF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Value containing the CHR bank nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xd000:
		chr_[4] = (chr_[4] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_1000_13ff(chr_[4] >> chr_shift_);
		break;
	case 0xd001:
		chr_[4] = (chr_[4] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1000_13ff(chr_[4] >> chr_shift_);
		break;
	case 0xd002:
		chr_[5] = (chr_[5] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_1400_17ff(chr_[5] >> chr_shift_);
		break;
	case 0xd003:
		chr_[5] = (chr_[5] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1400_17ff(chr_[5] >> chr_shift_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::write_e
//
// Handles writes to VRC2 CHR bank registers 6 and 7.
//
// Each 1 KB CHR bank number is assembled from separate low- and high-nibble
// writes.  The raw register value is then shifted according to chr_shift_
// before being mapped into PPU ranges $1800-$1BFF and $1C00-$1FFF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Value containing the CHR bank nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC2::write_e(uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xe000:
		chr_[6] = (chr_[6] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_1800_1bff(chr_[6] >> chr_shift_);
		break;
	case 0xe001:
		chr_[6] = (chr_[6] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1800_1bff(chr_[6] >> chr_shift_);
		break;
	case 0xe002:
		chr_[7] = (chr_[7] & 0xf0) | ((value & 0x0f) << 0);
		set_chr_1c00_1fff(chr_[7] >> chr_shift_);
		break;
	case 0xe003:
		chr_[7] = (chr_[7] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1c00_1fff(chr_[7] >> chr_shift_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC2::debug_state
//
// Returns a debugger-facing snapshot of VRC2-specific mapper state.
//
// The CHR values are the raw VRC2 register contents before chr_shift_ is
// applied.  This makes it possible for the debugger to show both the mapper's
// programmed register values and, through the generic Mapper state, the
// resulting CHR mappings.
//
// Latch activity counters are included so short-lived $6000 latch transitions
// remain visible to the debugger even when periodic UI sampling misses the
// actual value changes.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC2 mapper-specific debugger state.
// -----------------------------------------------------------------------------
vrc2_debug_state_t
VRC2::debug_mapper_state() const
{
	vrc2_debug_state_t state;

	for (int i = 0; i < 8; ++i) {
		state.chr[i] = chr_[i];
	}

	state.latch = latch_;
	state.chr_shift = chr_shift_;

	state.latch_write_count = latch_write_count_;
	state.latch_change_count = latch_change_count_;

	return state;
}


