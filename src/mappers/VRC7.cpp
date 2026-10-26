#include "VRC7.h"

#include "Cpu.h"


// -----------------------------------------------------------------------------
// VRC7::VRC7
//
// Initializes the VRC7 mapper core.
//
// The initial PRG mapping places banks 0, 1, and 2 in the three switchable
// 8 KB CPU regions and fixes the final PRG bank at $E000-$FFFF.
//
// CHR is initially mapped as one contiguous 8 KB region beginning at bank 0.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VRC7::VRC7()
{
	set_prg_89(0);
	set_prg_ab(1);
	set_prg_cd(2);
	set_prg_ef(-1);

	set_chr_0000_1fff(0);
}


// -----------------------------------------------------------------------------
// VRC7::name
//
// Returns the generic mapper hardware name for the VRC7 core.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper hardware name.
// -----------------------------------------------------------------------------
std::string
VRC7::name() const
{
	return "Konami VRC7";
}


// -----------------------------------------------------------------------------
// VRC7::read_6
//
// Reads the lower half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC7::read_6 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC7::read_7
//
// Reads the upper half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC7::read_7 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC7::write_6
//
// Writes the lower half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_6 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC7::write_7
//
// Writes the upper half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_7 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC7::write_8
//
// Handles the first two VRC7 PRG bank registers.
//
// $8000 selects the 8 KB PRG bank mapped at $8000-$9FFF.
// $8010 selects the 8 KB PRG bank mapped at $A000-$BFFF.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0x8000:
		set_prg_89(value & 0x3f);
		break;

	case 0x8010:
		set_prg_ab(value & 0x3f);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_9
//
// Handles the third VRC7 PRG register and expansion-audio ports.
//
// FM register writes are decoded and retained, but synthesis is intentionally
// deferred until the mapper core has been validated.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0x9000:
		set_prg_cd(value & 0x3f);
		break;

	case 0x9010:
		audio_register_ = value;
		break;

	case 0x9030:
		/*
		 * VRC7 FM data write.
		 *
		 * audio_register_ contains the selected internal FM register.
		 * Actual synthesis will be implemented separately.
		 */
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_a
//
// Handles the first two VRC7 1 KB CHR bank registers.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xa000:
		set_chr_0000_03ff(value);
		break;

	case 0xa010:
		set_chr_0400_07ff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_b
//
// Handles the third and fourth VRC7 1 KB CHR bank registers.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xb000:
		set_chr_0800_0bff(value);
		break;

	case 0xb010:
		set_chr_0c00_0fff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_c
//
// Handles the fifth and sixth VRC7 1 KB CHR bank registers.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xc000:
		set_chr_1000_13ff(value);
		break;

	case 0xc010:
		set_chr_1400_17ff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_d
//
// Handles the seventh and eighth VRC7 1 KB CHR bank registers.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xd000:
		set_chr_1800_1bff(value);
		break;

	case 0xd010:
		set_chr_1c00_1fff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_e
//
// Handles the VRC7 mapper-control and IRQ-latch registers.
//
// $E000 controls nametable mirroring, expansion-audio silence/reset state, and
// PRG-RAM access.  $E010 programs the IRQ latch.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xe000:
		switch (value & 0x03) {
		case 0:
			set_mirroring(mirror_vertical);
			break;

		case 1:
			set_mirroring(mirror_horizontal);
			break;

		case 2:
			set_mirroring(mirror_single_low);
			break;

		case 3:
			set_mirroring(mirror_single_high);
			break;
		}

		audio_silenced_ = (value & 0x40) != 0;
		wram_enabled_ = (value & 0x80) != 0;
		break;

	case 0xe010:
		irq_latch_ = value;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_f
//
// Handles the VRC7 IRQ control and acknowledge registers.
//
// Parameters:
//   address - CPU address selecting the IRQ register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xf000:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.raw = value;

		if (irq_control_.enabled) {
			irq_counter_ = irq_latch_;
			irq_prescaler_ = 341;
		}
		break;

	case 0xf010:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.enabled = irq_control_.a;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::cpu_sync
//
// Advances the VRC7 IRQ timing state by one CPU cycle.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::cpu_sync()
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
// VRC7::clock_irq
//
// Advances the VRC7 IRQ counter by one clock.
//
// When the counter reaches $FF, the next clock reloads it from the programmed
// IRQ latch and asserts the mapper IRQ line.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::clock_irq()
{
	if (irq_counter_ == 0xff) {
		irq_counter_ = irq_latch_;

		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	} else {
		++irq_counter_;
	}
}

