
#include "VRC4.h"
#include "Cpu.h"


// -----------------------------------------------------------------------------
// VRC4::VRC4
//
// Initializes the VRC4 mapper core.
//
// The initial PRG mapping places the first 16 KB bank at $8000-$BFFF and the
// final 16 KB bank at $C000-$FFFF. CHR is initially mapped as a contiguous
// 8 KB region starting at bank 0, and the individual CHR bank registers are
// initialized to banks 0 through 7.
// -----------------------------------------------------------------------------
VRC4::VRC4()
{
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
// VRC4::name
//
// Returns the generic mapper hardware name for the VRC4 core.
//
// Derived mapper implementations override this when they represent a specific
// VRC4 hardware variant such as VRC4a, VRC4b, VRC4c, VRC4d, VRC4e, or VRC4f.
// -----------------------------------------------------------------------------
std::string
VRC4::name() const
{
	return "VRC4";
}

// -----------------------------------------------------------------------------
// read_6
//
// Reads VRC4 work RAM in the CPU $6000-$6FFF range when WRAM is enabled.
//
// Disabled WRAM currently returns $FF.
// -----------------------------------------------------------------------------
uint8_t
VRC4::read_6 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return 0xff;
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// read_7
//
// Reads VRC4 work RAM in the CPU $7000-$7FFF range when WRAM is enabled.
//
// Disabled WRAM currently returns $FF.
// -----------------------------------------------------------------------------
uint8_t
VRC4::read_7 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return 0xff;
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// write_6
//
// Writes VRC4 work RAM in the CPU $6000-$6FFF range when WRAM is enabled.
// -----------------------------------------------------------------------------
void
VRC4::write_6 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// write_7
//
// Writes VRC4 work RAM in the CPU $7000-$7FFF range when WRAM is enabled.
// -----------------------------------------------------------------------------
void
VRC4::write_7 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC4::write_8
//
// Handles writes to the VRC4 $8000 register range.
//
// The selected value controls the switchable 8 KB PRG bank. Depending on the
// current PRG mode, that bank is mapped at either $8000-$9FFF or $C000-$DFFF,
// while the opposite slot is fixed to the second-last PRG bank.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
// -----------------------------------------------------------------------------
void
VRC4::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0x8000:
	case 0x8002:
	case 0x8004:
	case 0x8006:
		prg_[0] = value;

		if (prg_mode_ & 0x02) {
			set_prg_cd(prg_[0] & 0x1f);
			set_prg_89(-2);
		} else {
			set_prg_89(prg_[0] & 0x1f);
			set_prg_cd(-2);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_9
//
// Handles VRC4 mirroring, WRAM enable, and PRG banking mode control.
//
// $9000 selects nametable mirroring. $9008 controls WRAM availability and the
// PRG banking mode, immediately reapplying the current PRG bank configuration.
// The remaining decoded register addresses are unused by this implementation.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the selected control register.
// -----------------------------------------------------------------------------
void
VRC4::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0x9000:
		switch (value & 0x03) {
		case 0x00:
			set_mirroring(mirror_vertical);
			break;

		case 0x01:
			set_mirroring(mirror_horizontal);
			break;

		case 0x02:
			set_mirroring(mirror_single_low);
			break;

		case 0x03:
			set_mirroring(mirror_single_high);
			break;
		}
		break;

	case 0x9008:
		wram_enabled_ = (value & 0x01) != 0;

		prg_mode_ = value;

		if (prg_mode_ & 0x02) {
			set_prg_cd(prg_[0] & 0x1f);
			set_prg_89(-2);
		} else {
			set_prg_89(prg_[0] & 0x1f);
			set_prg_cd(-2);
		}
		break;

	case 0x9004:
	case 0x900c:
		/*
		 * Not used by the normal VRC4 mirroring/PRG-swap logic.
		 */
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_a
//
// Handles writes to the VRC4 $A000 register range.
//
// The value selects the switchable 8 KB PRG bank mapped at $A000-$BFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
// -----------------------------------------------------------------------------
void
VRC4::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xa000:
	case 0xa002:
	case 0xa004:
	case 0xa006:
		prg_[1] = value;
		set_prg_ab(prg_[1] & 0x1f);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_b
//
// Handles writes to the first two VRC4 CHR bank registers.
//
// Each 1 KB CHR bank number is assembled from separate low- and high-nibble
// writes and immediately applied to PPU ranges $0000-$03FF and $0400-$07FF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Low four bits of the CHR bank value.
// -----------------------------------------------------------------------------
void
VRC4::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xb000:
		chr_[0] = (chr_[0] & 0xf0) | (value & 0x0f);
		set_chr_0000_03ff(chr_[0]);
		break;

	case 0xb004:
		chr_[0] = (chr_[0] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0000_03ff(chr_[0]);
		break;

	case 0xb008:
		chr_[1] = (chr_[1] & 0xf0) | (value & 0x0f);
		set_chr_0400_07ff(chr_[1]);
		break;

	case 0xb00c:
		chr_[1] = (chr_[1] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0400_07ff(chr_[1]);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_c
//
// Handles writes to VRC4 CHR bank registers 2 and 3.
//
// Separate low- and high-nibble writes form the complete bank numbers mapped to
// PPU ranges $0800-$0BFF and $0C00-$0FFF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Low four bits of the CHR bank value.
// -----------------------------------------------------------------------------
void
VRC4::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xc000:
		chr_[2] = (chr_[2] & 0xf0) | (value & 0x0f);
		set_chr_0800_0bff(chr_[2]);
		break;

	case 0xc004:
		chr_[2] = (chr_[2] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0800_0bff(chr_[2]);
		break;

	case 0xc008:
		chr_[3] = (chr_[3] & 0xf0) | (value & 0x0f);
		set_chr_0c00_0fff(chr_[3]);
		break;

	case 0xc00c:
		chr_[3] = (chr_[3] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_0c00_0fff(chr_[3]);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_d
//
// Handles writes to VRC4 CHR bank registers 4 and 5.
//
// Separate low- and high-nibble writes form the complete bank numbers mapped to
// PPU ranges $1000-$13FF and $1400-$17FF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Low four bits of the CHR bank value.
// -----------------------------------------------------------------------------
void
VRC4::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xd000:
		chr_[4] = (chr_[4] & 0xf0) | (value & 0x0f);
		set_chr_1000_13ff(chr_[4]);
		break;

	case 0xd004:
		chr_[4] = (chr_[4] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1000_13ff(chr_[4]);
		break;

	case 0xd008:
		chr_[5] = (chr_[5] & 0xf0) | (value & 0x0f);
		set_chr_1400_17ff(chr_[5]);
		break;

	case 0xd00c:
		chr_[5] = (chr_[5] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1400_17ff(chr_[5]);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_e
//
// Handles writes to VRC4 CHR bank registers 6 and 7.
//
// Separate low- and high-nibble writes form the complete bank numbers mapped to
// PPU ranges $1800-$1BFF and $1C00-$1FFF.
//
// Parameters:
//   address - CPU address selecting the CHR register nibble.
//   value   - Low four bits of the CHR bank value.
// -----------------------------------------------------------------------------
void
VRC4::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xe000:
		chr_[6] = (chr_[6] & 0xf0) | (value & 0x0f);
		set_chr_1800_1bff(chr_[6]);
		break;

	case 0xe004:
		chr_[6] = (chr_[6] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1800_1bff(chr_[6]);
		break;

	case 0xe008:
		chr_[7] = (chr_[7] & 0xf0) | (value & 0x0f);
		set_chr_1c00_1fff(chr_[7]);
		break;

	case 0xe00c:
		chr_[7] = (chr_[7] & 0x0f) | ((value & 0x0f) << 4);
		set_chr_1c00_1fff(chr_[7]);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::write_f
//
// Handles writes to the VRC4 IRQ registers.
//
// The first two registers load the low and high nibbles of the IRQ latch.
// The control register clears any pending mapper IRQ, configures IRQ operation,
// and reloads the counter and prescaler when IRQs are enabled. The acknowledge
// register clears the active IRQ and optionally re-enables the IRQ counter.
//
// Parameters:
//   address - CPU address selecting the IRQ register.
//   value   - Value written to the IRQ register.
// -----------------------------------------------------------------------------
void
VRC4::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf00c) {
	case 0xf000:
		irq_latch_ = (irq_latch_ & 0xf0) | (value & 0x0f);
		break;

	case 0xf004:
		irq_latch_ = (irq_latch_ & 0x0f) | ((value & 0x0f) << 4);
		break;

	case 0xf008:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.raw = value;

		if (irq_control_.enabled) {
			irq_counter_ = irq_latch_;
			irq_prescaler_ = 341;
		}
		break;

	case 0xf00c:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.enabled = irq_control_.a;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC4::cpu_sync
//
// Advances the VRC4 IRQ timing state by one CPU cycle.
//
// In cycle mode the IRQ counter is clocked once per CPU cycle. In scanline mode
// the prescaler subtracts three PPU clocks per CPU cycle and clocks the IRQ
// counter whenever approximately one scanline, 341 PPU clocks, has elapsed.
// -----------------------------------------------------------------------------
void
VRC4::cpu_sync()
{
	if (irq_control_.enabled) {
		if (irq_control_.mode) {
			clock_irq();
		} else {
			irq_prescaler_ -= 3;

			if (irq_prescaler_ <= 0) {
				clock_irq();
				irq_prescaler_ += 341;
			}
		}
	}
}


// -----------------------------------------------------------------------------
// VRC4::clock_irq
//
// Advances the VRC4 IRQ counter by one clock.
//
// When the counter reaches $FF, the next clock reloads it from the IRQ latch
// and asserts the mapper IRQ line. Otherwise the counter is simply incremented.
// -----------------------------------------------------------------------------
void
VRC4::clock_irq()
{
	if (irq_counter_ == 0xff) {
		irq_counter_ = irq_latch_;

		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	} else {
		++irq_counter_;
	}
}

