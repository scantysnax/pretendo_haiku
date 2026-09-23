
#include "Mapper005.h"
#include "Cart.h"
#include "Nes.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(5)

namespace {

// Vertical split control bits used by $5200.
enum : uint8_t {
	VSPLIT_ENABLE = 0x80,
	VSPLIT_RIGHT  = 0x40,
	VSPLIT_TILE   = 0x1f
};

}


// -----------------------------------------------------------------------------
// Mapper5::Mapper5
//
// Initializes the MMC5 mapper.
//
// PRG-ROM slots are initialized to the final PRG bank until the cartridge
// programs the MMC5 PRG registers. The debugger CHR mapping is also initialized
// so the Mapper Explorer has a valid resolved CHR view from startup.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper5::Mapper5()
{
	set_prg_89(-1);
	set_prg_ab(-1);
	set_prg_cd(-1);
	set_prg_ef(-1);

	debug_update_chr_mapping();
}


// -----------------------------------------------------------------------------
// Mapper5::name
//
// Returns the display name for the MMC5 mapper.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper name.
// -----------------------------------------------------------------------------
std::string 
Mapper5::name() const
{
	return "Nintendo MMC5";
}


// -----------------------------------------------------------------------------
// Mapper5::write_5
//
// Handles CPU writes in the $5000-$5FFF range.
//
// This range contains the MMC5 control registers for PRG/CHR banking, PRG-RAM
// protection, ExRAM, nametable mapping, fill mode, vertical split, scanline
// IRQs, and the hardware multiplier.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_5 (uint_least16_t address, uint8_t value)
{
	switch (address) {
	// MMC5 operating modes and memory-control registers.
	case 0x5100:
		prg_mode_ = value & 0x03;
		break;

	case 0x5101:
		chr_mode_ = value & 0x03;
		debug_update_chr_mapping();
		break;

	case 0x5102:
		prg_ram_protect1_ = value & 0x03;
		break;

	case 0x5103:
		prg_ram_protect2_ = value & 0x03;
		break;

	case 0x5104:
		exram_mode_ = value & 0x3;
		break;

	case 0x5105:
		mirroring_mode_ = value;
		set_mirroring(value);
		break;

	case 0x5106:
		fill_mode_tile_ = value;
		break;

	case 0x5107:
		fill_mode_attr_ = value & 0x03;
		break;

	// PRG-RAM/ROM bank registers ($5113-$5117).
	case 0x5113:
		prg_bank_[0] = value;

		prg_ram_banks_[0x06] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
		prg_ram_banks_[0x07] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
		break;

	case 0x5114:
		prg_bank_[1] = value;

		switch (prg_mode_) {
		case 0x00:
		case 0x01:
		case 0x02:
			break;

		case 0x03:
			if (value & 0x80) {
				set_prg_89(value & 0x7f);
				prg_ram_banks_[0x08] = nullptr;
				prg_ram_banks_[0x09] = nullptr;
			} else {
				prg_ram_banks_[0x08] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
				prg_ram_banks_[0x09] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
			}
			break;
		}
		break;

	case 0x5115:
		prg_bank_[2] = value;

		switch (prg_mode_) {
		case 0x00:
			break;

		case 0x01:
		case 0x02:
			if (value & 0x80) {
				set_prg_89ab((value & 0x7f) >> 1);

				prg_ram_banks_[0x08] = nullptr;
				prg_ram_banks_[0x09] = nullptr;
				prg_ram_banks_[0x0a] = nullptr;
				prg_ram_banks_[0x0b] = nullptr;
			} else {
				prg_ram_banks_[0x08] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x4000) & 0x7fff) + 0x0000;
				prg_ram_banks_[0x09] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x4000) & 0x7fff) + 0x1000;
				prg_ram_banks_[0x0a] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x4000) & 0x7fff) + 0x2000;
				prg_ram_banks_[0x0b] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x4000) & 0x7fff) + 0x3000;
			}
			break;

		case 0x03:
			if (value & 0x80) {
				set_prg_ab(value & 0x7f);

				prg_ram_banks_[0x0a] = nullptr;
				prg_ram_banks_[0x0b] = nullptr;
			} else {
				prg_ram_banks_[0x0a] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
				prg_ram_banks_[0x0b] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
			}
			break;
		}
		break;

	case 0x5116:
		prg_bank_[3] = value;

		switch (prg_mode_) {
		case 0x00:
		case 0x01:
			break;

		case 0x02:
		case 0x03:
			if (value & 0x80) {
				set_prg_cd(value & 0x7f);

				prg_ram_banks_[0x0c] = nullptr;
				prg_ram_banks_[0x0d] = nullptr;
			} else {
				prg_ram_banks_[0x0c] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
				prg_ram_banks_[0x0d] = prg_ram_[(value >> 2) & 0x01] + (((value & 0x03) * 0x2000) & 0x7fff) + 0x0000;
			}
			break;
		}
		break;

	case 0x5117:
		prg_bank_[4] = value;

		switch (prg_mode_) {
		case 0x00:
			set_prg_89abcdef((value & 0x7f) >> 2);

			prg_ram_banks_[0x08] = nullptr;
			prg_ram_banks_[0x09] = nullptr;
			prg_ram_banks_[0x0a] = nullptr;
			prg_ram_banks_[0x0b] = nullptr;
			prg_ram_banks_[0x0c] = nullptr;
			prg_ram_banks_[0x0d] = nullptr;
			break;

		case 0x01:
			set_prg_cdef((value & 0x7f) >> 1);

			prg_ram_banks_[0x0c] = nullptr;
			prg_ram_banks_[0x0d] = nullptr;
			break;

		case 0x02:
		case 0x03:
			set_prg_ef(value & 0x7f);
			break;
		}
		break;

	// Sprite CHR bank registers ($5120-$5127).
	case 0x5120:
	case 0x5121:
	case 0x5122:
	case 0x5123:
	case 0x5124:
	case 0x5125:
	case 0x5126:
	case 0x5127:
		last_chr_write_               = CHR_BANK_A;
		sp_chr_banks_[address & 0x07] = value;

		debug_update_chr_mapping();
		break;

	// Background CHR bank registers ($5128-$512F).
	case 0x5128:
	case 0x5129:
	case 0x512a:
	case 0x512b:
	case 0x512c: // $512c-$512f are not part of the official spec
	case 0x512d: // but are consistent with what makes sense, perhaps
	case 0x512e: // $5128-$512b are partially decoded?
	case 0x512f:
		last_chr_write_                        = CHR_BANK_B;
		bg_chr_banks_[(address & 0x07) ^ 0x00] = value;
		bg_chr_banks_[(address & 0x07) ^ 0x04] = value;

		debug_update_chr_mapping();
		break;

	// Upper CHR bank bits.
	case 0x5130:
		bg_char_upper_ = (value << 8);
		debug_update_chr_mapping();
		break;

	// Vertical split, IRQ, and multiplier registers.
	case 0x5200:
		vertical_split_mode_ = value;
		break;

	case 0x5201:
		vertical_split_scroll_ = value;
		break;

	case 0x5202:
		vertical_split_bank_ = value;
		break;

	case 0x5203:
		irq_target_ = value;
		break;

	case 0x5204:
		irq_enabled_ = (value & 0x80);
		break;

	case 0x5205:
		multiplier_1_ = value;
		break;

	case 0x5206:
		multiplier_2_ = value;
		break;

	default:
		if (address >= 0x5c00 && (exram_mode_ & 0x03) == 0x02) {
			exram_[address & 0x03ff] = value;
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::write_6
//
// Routes CPU writes in the $6000-$6FFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper5::write_6 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_7
//
// Routes CPU writes in the $7000-$7FFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper5::write_7 (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_8
//
// Routes CPU writes in the $8000-$8FFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_8(uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_9
//
// Routes CPU writes in the $9000-$9FFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_9(uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_a
//
// Routes CPU writes in the $A000-$AFFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper5::write_a (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_b
//
// Routes CPU writes in the $B000-$BFFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper5::write_b (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_c
//
// Routes CPU writes in the $C000-$CFFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
Mapper5::write_c (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_d
//
// Routes CPU writes in the $D000-$DFFF range through the MMC5 PRG-RAM handler.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_d (uint_least16_t address, uint8_t value)
{
	write_handler(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_handler
//
// Writes to the currently mapped MMC5 PRG-RAM bank when PRG-RAM writes are
// unlocked by the two protection registers.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
Mapper5::write_handler (uint_least16_t address, uint8_t value)
{
	const uint8_t bank = (address >> 12) & 0x0f;

	// PRG-RAM writes are enabled only by the MMC5 protection unlock sequence.
	if (prg_ram_protect1_ == 0x02 && prg_ram_protect2_ == 0x01 && prg_ram_banks_[bank]) {
		prg_ram_banks_[bank][address & 0x0fff] = value;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::read_5
//
// Handles CPU reads in the $5000-$5FFF range.
//
// This includes IRQ status, the hardware multiplier result, and ExRAM access.
// Reading the IRQ status register also acknowledges the mapper IRQ.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value visible to the CPU.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_5(uint_least16_t address)
{

	uint8_t ret = (address >> 8);

	switch (address) {
	// Reading IRQ status acknowledges the mapper IRQ and clears Pending.
	case 0x5204:
		ret = irq_status_.raw;
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);
		irq_status_.pending = false;
		break;

	// The multiplier result is exposed as low/high bytes at $5205/$5206.
	case 0x5205:
		do {
			const uint16_t x = multiplier_1_ * multiplier_2_;
			ret              = x & 0xff;
		} while (0);
		break;

	case 0x5206:
		do {
			const uint16_t x = multiplier_1_ * multiplier_2_;
			ret              = (x >> 8) & 0xff;
		} while (0);
		break;

	default:
		if (address >= 0x5c00) {
			switch (exram_mode_) {
			case 0x00:
			case 0x01:
				break;
			case 0x02:
			case 0x03:
				ret = exram_[address & 0x03ff];
				break;
			}
		}
	}
	return ret;
}


// -----------------------------------------------------------------------------
// Mapper5::read_6
//
// Routes CPU reads in the $6000-$6FFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t Mapper5::read_6 (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_7
//
// Routes CPU reads in the $7000-$7FFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t Mapper5::read_7 (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_8
//
// Routes CPU reads in the $8000-$8FFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_8 (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_9
//
// Routes CPU reads in the $9000-$9FFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_9 (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_a
//
// Routes CPU reads in the $A000-$AFFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_a (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_b
//
// Routes CPU reads in the $B000-$BFFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_b (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_c
//
// Routes CPU reads in the $C000-$CFFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_c (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_d
//
// Routes CPU reads in the $D000-$DFFF range through the MMC5 memory handler.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from mapped PRG-RAM or PRG-ROM.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_d (uint_least16_t address)
{
	return read_handler(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_handler
//
// Reads from an MMC5 PRG-RAM bank when one is mapped at the requested CPU page.
// Otherwise the read falls through to the normal Mapper PRG mapping.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from PRG-RAM or the base mapper memory mapping.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_handler (uint_least16_t address) {

	const uint8_t bank = (address >> 12) & 0x0f;

	// A non-null page pointer selects PRG-RAM; otherwise use normal PRG mapping.
	if (prg_ram_banks_[bank]) {
		return prg_ram_banks_[bank][address & 0x0fff];
	}

	return Mapper::read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_vram
//
// Handles MMC5 PPU address-space reads.
//
// Nametable reads are routed according to the MMC5 nametable mapping register,
// ExRAM mode, and fill-mode registers. Pattern-table reads resolve the active
// background or sprite CHR register set according to the current CHR mode.
//
// Parameters:
//   address - PPU address being read.
//
// Returns:
//   Value visible to the PPU.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_vram (uint_least16_t address)
{
	if (vertical_split_bank_ & VSPLIT_ENABLE) {
		printf("VSPLIT\n");
	}

	// VSPLIT_RIGHT  = 0x40,
	// VSPLIT_TILE   = 0x1f

	switch ((address >> 10) & 0x0f) {
	// Nametable pages can select CIRAM, ExRAM, or fill mode independently.
	case 0x08:
	case 0x0c:
		// $2000
		switch (mirroring_mode_ & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);
		case 0x01:
			return Mapper::read_vram(address);
		case 0x02:
			return (exram_mode_ & 0x02) ? 0x00 : exram_[address & 0x03ff];
		case 0x03:
			return (address & 0x03ff) < 0x03c0 ? fill_mode_tile_ : fill_mode_attr_;
		}
		break;

	case 0x09:
	case 0x0d:
		// $2400
		switch ((mirroring_mode_ >> 2) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);
		case 0x01:
			return Mapper::read_vram(address);
		case 0x02:
			return (exram_mode_ & 0x02) ? 0x00 : exram_[address & 0x03ff];
		case 0x03:
			return (address & 0x03ff) < 0x03c0 ? fill_mode_tile_ : fill_mode_attr_;
		}
		break;

	case 0x0a:
	case 0x0e:
		// $2800
		switch ((mirroring_mode_ >> 4) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);
		case 0x01:
			return Mapper::read_vram(address);
		case 0x02:
			return (exram_mode_ & 0x02) ? 0x00 : exram_[address & 0x03ff];
		case 0x03:
			return (address & 0x03ff) < 0x03c0 ? fill_mode_tile_ : fill_mode_attr_;
		}
		break;

	case 0x0b:
	case 0x0f:
		// $2c00
		switch ((mirroring_mode_ >> 6) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);
		case 0x01:
			return Mapper::read_vram(address);
		case 0x02:
			return (exram_mode_ & 0x02) ? 0x00 : exram_[address & 0x03ff];
		case 0x03:
			return (address & 0x03ff) < 0x03c0 ? fill_mode_tile_ : fill_mode_attr_;
		}
		break;

	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		// Pattern-table reads select the active MMC5 background/sprite CHR set.
		// CHR-ROM ($0000 - $1fff)
		const uint8_t *chr_selector;

		if (large_sprites_) {
			// there seems to be 128 fetches of tiles before sprites..
			if (fetch_count_ > 128 && fetch_count_ < 160) {
				chr_selector = sp_chr_banks_;
			} else {
				chr_selector = bg_chr_banks_;
			}
		} else {
			if (last_chr_write_ == CHR_BANK_A) {
				chr_selector = sp_chr_banks_;
			} else {
				chr_selector = bg_chr_banks_;
			}
		}

		// Resolve the selected MMC5 CHR registers into eight 1 KB PPU pages.
		const uint8_t *chr_rom_banks[8];
		const uint8_t *const chr_rom = nes::cart.chr();
		const uint32_t chr_mask      = nes::cart.chr_mask();

		switch (chr_mode_ & 0x03) {
		case 0x00:                                                                                            // 8K mode
			chr_rom_banks[0] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x0000; // $0000
			chr_rom_banks[1] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x0400; // $0400
			chr_rom_banks[2] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x0800; // $0800
			chr_rom_banks[3] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x0c00; // $0c00
			chr_rom_banks[4] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x1000; // $1000
			chr_rom_banks[5] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x1400; // $1400
			chr_rom_banks[6] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x1800; // $1800
			chr_rom_banks[7] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x2000) & chr_mask) + 0x1c00; // $1c00
			break;
		case 0x01:                                                                                            // 4K mode
			chr_rom_banks[0] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0000; // $0000
			chr_rom_banks[1] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0400; // $0400
			chr_rom_banks[2] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0800; // $0800
			chr_rom_banks[3] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0c00; // $0c00
			chr_rom_banks[4] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0000; // $1000
			chr_rom_banks[5] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0400; // $1400
			chr_rom_banks[6] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0800; // $1800
			chr_rom_banks[7] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x1000) & chr_mask) + 0x0c00; // $1c00
			break;
		case 0x02:                                                                                            // 2K mode
			chr_rom_banks[0] = chr_rom + (((chr_selector[1] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0000; // $0000
			chr_rom_banks[1] = chr_rom + (((chr_selector[1] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0400; // $0400
			chr_rom_banks[2] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0000; // $0800
			chr_rom_banks[3] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0400; // $0c00
			chr_rom_banks[4] = chr_rom + (((chr_selector[5] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0000; // $1000
			chr_rom_banks[5] = chr_rom + (((chr_selector[5] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0400; // $1400
			chr_rom_banks[6] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0000; // $1800
			chr_rom_banks[7] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x0800) & chr_mask) + 0x0400; // $1c00
			break;
		case 0x03:                                                                                            // 1K mode
			chr_rom_banks[0] = chr_rom + (((chr_selector[0] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $0000
			chr_rom_banks[1] = chr_rom + (((chr_selector[1] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $0400
			chr_rom_banks[2] = chr_rom + (((chr_selector[2] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $0800
			chr_rom_banks[3] = chr_rom + (((chr_selector[3] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $0c00
			chr_rom_banks[4] = chr_rom + (((chr_selector[4] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $1000
			chr_rom_banks[5] = chr_rom + (((chr_selector[5] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $1400
			chr_rom_banks[6] = chr_rom + (((chr_selector[6] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $1800
			chr_rom_banks[7] = chr_rom + (((chr_selector[7] + bg_char_upper_) * 0x0400) & chr_mask) + 0x0000; // $1c00
			break;
		}

		return chr_rom_banks[(address >> 10) & 0x0f][address & 0x03ff];
	}

	return Mapper::read_vram(address);
}


// -----------------------------------------------------------------------------
// Mapper5::write_vram
//
// Handles MMC5 PPU address-space writes.
//
// Nametable writes are routed to CIRAM or ExRAM according to the MMC5 nametable
// and ExRAM configuration. CHR-ROM writes are ignored by the MMC5-specific
// path before the base mapper receives the write.
//
// Parameters:
//   address - PPU address being written.
//   value   - Value written by the PPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
Mapper5::write_vram (uint_least16_t address, uint8_t value)
{
	switch ((address >> 10) & 0x0f) {
	// Route nametable writes according to the per-page MMC5 nametable selector.
	case 0x08:
	case 0x0c:
		// $2000
		switch ((mirroring_mode_)&0x03) {
		case 0x00:
			Mapper::write_vram(address, value);
			break;
		case 0x01:
			Mapper::write_vram(address, value);
			break;
		case 0x02:
			if (!(exram_mode_ & 0x02)) exram_[address & 0x03ff] = value;
			break;
		case 0x03:
			break;
		}
		break;
	case 0x09:
	case 0x0d:
		// $2400
		switch ((mirroring_mode_ >> 2) & 0x03) {
		case 0x00:
			Mapper::write_vram(address, value);
			break;
		case 0x01:
			Mapper::write_vram(address, value);
			break;
		case 0x02:
			if (!(exram_mode_ & 0x02)) exram_[address & 0x03ff] = value;
			break;
		case 0x03:
			break;
		}
		break;
	case 0x0a:
	case 0x0e:
		// $2800
		switch ((mirroring_mode_ >> 4) & 0x03) {
		case 0x00:
			Mapper::write_vram(address, value);
			break;
		case 0x01:
			Mapper::write_vram(address, value);
			break;
		case 0x02:
			if (!(exram_mode_ & 0x02)) exram_[address & 0x03ff] = value;
			break;
		case 0x03:
			break;
		}
		break;
	case 0x0b:
	case 0x0f:
		// $2c00
		switch ((mirroring_mode_ >> 6) & 0x03) {
		case 0x00:
			Mapper::write_vram(address, value);
			break;
		case 0x01:
			Mapper::write_vram(address, value);
			break;
		case 0x02:
			if ((exram_mode_ & 0x02)) exram_[address & 0x03ff] = value;
			break;
		case 0x03:
			break;
		}
		break;
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		// CHR-ROM is read-only
		break;
	}

	Mapper::write_vram(address, value);
}


// -----------------------------------------------------------------------------
// Mapper5::write_2
//
// Observes PPU-control register writes mirrored through the $2000 page.
//
// MMC5 uses the sprite-size bit to select how background and sprite CHR banks
// are interpreted. Disabling both background and sprite rendering also clears
// the MMC5 in-frame state.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_2 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x07) {
	case 0x00:
		large_sprites_ = (value & 0x20);
		debug_update_chr_mapping();
		break;

	case 0x01:
		// sprites and background disabled
		if (!(value & 0x18)) {
			irq_status_.in_frame = false;
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::write_3
//
// Observes PPU-control register writes mirrored through the $3000 page.
//
// This mirrors the MMC5 state tracking performed for writes through the $2000
// page.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_3 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x07) {
	case 0x00:
		large_sprites_ = (value & 0x20);
		debug_update_chr_mapping();
		break;

	case 0x01:
		// sprites and background disabled
		if (!(value & 0x18)) {
			irq_status_.in_frame = false;
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::vram_change_hook
//
// Tracks PPU address activity used by the current MMC5 scanline detector and
// background/sprite CHR fetch selection.
//
// Three consecutive accesses to the same nametable address are treated as a
// scanline clock for the mapper IRQ counter.
//
// Parameters:
//   vram_address - Current PPU address.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::vram_change_hook (uint_least16_t vram_address)
{
	// Track fetch position so 8x16-sprite rendering can distinguish the sprite
	// CHR fetch phase from background fetches.
	// when this is > 128 (32 * 4), we are fetching sprites, not BG tiles
	++fetch_count_;

	// The current implementation treats three consecutive reads of the same
	// nametable address as the MMC5 scanline clock condition.
	// 3 consecutive reads!
	if (vram_address == prev_vram_address_[0] && vram_address == prev_vram_address_[1] && (vram_address & 0x2000)) {
		clock_irq();
		fetch_count_ = 0;
	}

	// shift things down
	prev_vram_address_[1] = prev_vram_address_[0];
	prev_vram_address_[0] = vram_address;
}


// -----------------------------------------------------------------------------
// Mapper5::clock_irq
//
// Advances the MMC5 scanline IRQ state.
//
// The first detected scanline establishes the in-frame state and resets the
// counter. Subsequent scanlines increment the counter and set IRQ pending when
// the programmed target is reached.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::clock_irq()
{
	// if the In Frame signal is clear
	if (!irq_status_.in_frame) {

		// set it, reset the IRQ counter to 0, and clear the IRQ Pending flag
		irq_status_.in_frame = true;
		irq_status_.pending  = false;
		irq_counter_         = 0;
	} else {

		// otherwise, increment the IRQ counter. If it now equals the IRQ scanline ($5203),
		// raise IRQ Pending flag
		if (++irq_counter_ == irq_target_) {
			if (irq_enabled_) {
				nes::cpu::irq(nes::cpu::MAPPER_IRQ);
			}

			irq_status_.pending = true;
		}
	}
}


// -----------------------------------------------------------------------------
// Mapper5::ppu_end_frame
//
// Clears the MMC5 in-frame flag at the end of a PPU frame.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::ppu_end_frame()
{
	// since we have no idea how MMC5 detects the end of the frame,
	// we use this hook for now
	irq_status_.in_frame = false;
}


// -----------------------------------------------------------------------------
// Mapper5::debug_state
//
// Captures the MMC5-specific state exposed by the Mapper Explorer.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot of the current MMC5 debugger-visible state.
// -----------------------------------------------------------------------------
mmc5_debug_state_t
Mapper5::debug_state() const
{
	mmc5_debug_state_t state;

	state.prg_mode = prg_mode_;
	state.chr_mode = chr_mode_;

	for (int i = 0; i < 5; ++i) {
		state.prg_bank[i] = prg_bank_[i];
	}

	for (int i = 0; i < 8; ++i) {
		state.bg_chr_bank[i] = bg_chr_banks_[i];
		state.sp_chr_bank[i] = sp_chr_banks_[i];
	}

	state.bg_char_upper = bg_char_upper_;

	state.last_chr_write_bg = (last_chr_write_ == CHR_BANK_B);

	state.prg_ram_protect1 = prg_ram_protect1_;
	state.prg_ram_protect2 = prg_ram_protect2_;

	state.mirroring_mode = mirroring_mode_;
	state.exram_mode = exram_mode_;

	state.fill_mode_tile = fill_mode_tile_;
	state.fill_mode_attr = fill_mode_attr_;

	state.vertical_split_mode = vertical_split_mode_;
	state.vertical_split_scroll = vertical_split_scroll_;
	state.vertical_split_bank = vertical_split_bank_;

	state.large_sprites = large_sprites_;
	state.fetch_count = fetch_count_;

	state.irq_enabled = irq_enabled_;
	state.irq_counter = irq_counter_;
	state.irq_target = irq_target_;
	state.irq_in_frame = irq_status_.in_frame;
	state.irq_pending = irq_status_.pending;

	state.multiplier_1 = multiplier_1_;
	state.multiplier_2 = multiplier_2_;

	// $5205/$5206 expose the 16-bit product of the two 8-bit operands.
	state.multiplier_result = static_cast<uint16_t>(multiplier_1_) * static_cast<uint16_t>(multiplier_2_);

	return state;
}


// -----------------------------------------------------------------------------
// Mapper5::debug_update_chr_mapping
//
// Updates the generic Mapper Explorer CHR mapping from the current MMC5 CHR
// registers and mode.
//
// In 8x16 sprite mode the generic table intentionally represents the stable
// background mapping; both background and sprite register sets remain visible
// in the MMC5-specific diagnostics panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::debug_update_chr_mapping()
{
	const uint8_t *chr_selector;

	// In 8x16 sprite mode MMC5 uses separate background and sprite CHR
	// register sets during rendering. Keep the generic Mapper Explorer table
	// stable by representing the background mapping; the MMC5-specific panel
	// displays both sets independently.
	if (large_sprites_) {
		chr_selector = bg_chr_banks_;
	} else {
		chr_selector = last_chr_write_ == CHR_BANK_A ? sp_chr_banks_ : bg_chr_banks_;
	}

	const uint32_t chr_mask = nes::cart.chr_mask();

	for (int i = 0; i < 8; ++i) {
		uint32_t offset = 0;

		switch (chr_mode_ & 0x03) {
			case 0x00:
				// 8 KB mode.
				offset = ((static_cast<uint32_t>(chr_selector[7] + bg_char_upper_) * 0x2000) +
						   static_cast<uint32_t>(i * 0x0400)) & chr_mask;
				break;

			case 0x01:
				// 4 KB mode.
				if (i < 4) {
					offset = ((static_cast<uint32_t>(chr_selector[3] + bg_char_upper_) * 0x1000) +
							   static_cast<uint32_t>(i * 0x0400)) & chr_mask;
				} else {
					offset = ((static_cast<uint32_t>(chr_selector[7] + bg_char_upper_) * 0x1000) +
							   static_cast<uint32_t>((i - 4) * 0x0400)) & chr_mask;
				}
				break;

			case 0x02:
				// 2 KB mode.
				offset = ((static_cast<uint32_t>(chr_selector[((i >> 1) << 1) + 1] + bg_char_upper_) * 0x0800) +
						   static_cast<uint32_t>((i & 0x01) * 0x0400)) & chr_mask;
				break;

			case 0x03:
				// 1 KB mode.
				offset = (static_cast<uint32_t>(chr_selector[i] + bg_char_upper_) * 0x0400) & chr_mask;
				break;
		}

		debug_set_chr_bank(i, static_cast<uint16_t>(i * 0x0400), offset / 0x0400, MapperDebugMemoryType::CHRROM);
	}
}
