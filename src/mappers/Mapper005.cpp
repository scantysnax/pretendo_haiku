
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


// -----------------------------------------------------------------------------
// kMMC5LengthTable
//
// Contains the MMC5 pulse-channel length-counter reload values.
//
// The table is selected by bits 7-3 of $5003/$5007.  It matches the NTSC APU
// length table except for entry $1C, which loads 16 on MMC5 hardware.
// -----------------------------------------------------------------------------
const uint8_t kMMC5LengthTable[32] = {
	10, 254, 20, 2,
	40, 4, 80, 6,
	160, 8, 60, 10,
	14, 12, 26, 14,

	12, 16, 24, 18,
	48, 20, 96, 22,
	192, 24, 72, 26,
	16, 28, 32, 30
};


// -----------------------------------------------------------------------------
// kMMC5DutyTable
//
// Contains the four eight-step MMC5 pulse duty sequences.
//
// The table is arranged for the forward sequence_step counter used by the MMC5
// audio implementation.
// -----------------------------------------------------------------------------
const uint8_t kMMC5DutyTable[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{1, 0, 0, 1, 1, 1, 1, 1}
};

constexpr double kMMC5NTSCClock = 1789773.0;


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
// This range contains the MMC5 expansion-audio registers, PRG/CHR banking,
// PRG-RAM protection, ExRAM, nametable mapping, fill mode, vertical split,
// scanline IRQs, and hardware multiplier registers.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::write_5(uint_least16_t address, uint8_t value)
{
	switch (address) {
	// MMC5 pulse 1.
	case 0x5000:
		pulse_[0].duty = (value >> 6) & 0x03;
		pulse_[0].length_halt = (value & 0x20) != 0;
		pulse_[0].constant_volume = (value & 0x10) != 0;
		pulse_[0].volume = value & 0x0f;
		break;

	case 0x5002:
		pulse_[0].period =
			(pulse_[0].period & 0x0700) |
			value;
		break;

	case 0x5003:
		pulse_[0].period =
			(pulse_[0].period & 0x00ff) |
			((static_cast<uint16_t>(value & 0x07)) << 8);

		if (pulse_[0].enabled) {
			pulse_[0].length_counter =
				kMMC5LengthTable[(value >> 3) & 0x1f];
		}

		pulse_[0].sequence_step = 0;
		pulse_[0].envelope_start = true;
		break;


	// MMC5 pulse 2.
	case 0x5004:
		pulse_[1].duty = (value >> 6) & 0x03;
		pulse_[1].length_halt = (value & 0x20) != 0;
		pulse_[1].constant_volume = (value & 0x10) != 0;
		pulse_[1].volume = value & 0x0f;
		break;

	case 0x5006:
		pulse_[1].period =
			(pulse_[1].period & 0x0700) |
			value;
		break;

	case 0x5007:
		pulse_[1].period =
			(pulse_[1].period & 0x00ff) |
			((static_cast<uint16_t>(value & 0x07)) << 8);

		if (pulse_[1].enabled) {
			pulse_[1].length_counter =
				kMMC5LengthTable[(value >> 3) & 0x1f];
		}

		pulse_[1].sequence_step = 0;
		pulse_[1].envelope_start = true;
		break;


	// MMC5 PCM.
	case 0x5010:
		pcm_.read_mode = (value & 0x01) != 0;
		pcm_.irq_enabled = (value & 0x80) != 0;

		if (!pcm_.irq_enabled) {
			pcm_.irq_pending = false;
		}

		update_irq_line();
		break;

	case 0x5011:
    	if (!pcm_.read_mode && value != 0) {
        	pcm_.value = value;
    	}
    	break;

	case 0x5015:
		pulse_[0].enabled = (value & 0x01) != 0;
		pulse_[1].enabled = (value & 0x02) != 0;

		if (!pulse_[0].enabled) {
			pulse_[0].length_counter = 0;
		}

		if (!pulse_[1].enabled) {
			pulse_[1].length_counter = 0;
		}
		break;


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
		exram_mode_ = value & 0x03;
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


	// PRG-RAM bank at $6000-$7FFF.
	case 0x5113:
		prg_bank_[0] = value;

		prg_ram_banks_[0x06] =
			prg_ram_[(value >> 2) & 0x01] +
			(((value & 0x03) * 0x2000) & 0x7fff) +
			0x0000;

		prg_ram_banks_[0x07] =
			prg_ram_[(value >> 2) & 0x01] +
			(((value & 0x03) * 0x2000) & 0x7fff) +
			0x1000;
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
				prg_ram_banks_[0x08] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x0000;

				prg_ram_banks_[0x09] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x1000;
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
			if (value & 0x80) {
				set_prg_89ab((value & 0x7f) >> 1);

				prg_ram_banks_[0x08] = nullptr;
				prg_ram_banks_[0x09] = nullptr;
				prg_ram_banks_[0x0a] = nullptr;
				prg_ram_banks_[0x0b] = nullptr;
			} else {
				prg_ram_banks_[0x08] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x0000;

				prg_ram_banks_[0x09] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x1000;

				prg_ram_banks_[0x0a] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x0000;

				prg_ram_banks_[0x0b] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x1000;
			}
			break;

		case 0x02:
		case 0x03:
			if (value & 0x80) {
				set_prg_ab(value & 0x7f);

				prg_ram_banks_[0x0a] = nullptr;
				prg_ram_banks_[0x0b] = nullptr;
			} else {
				prg_ram_banks_[0x0a] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x0000;

				prg_ram_banks_[0x0b] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x1000;
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
				prg_ram_banks_[0x0c] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x0000;

				prg_ram_banks_[0x0d] =
					prg_ram_[(value >> 2) & 0x01] +
					(((value & 0x03) * 0x2000) & 0x7fff) +
					0x1000;
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


	// Sprite CHR bank registers.
	case 0x5120:
	case 0x5121:
	case 0x5122:
	case 0x5123:
	case 0x5124:
	case 0x5125:
	case 0x5126:
	case 0x5127:
		last_chr_write_ = CHR_BANK_A;
		sp_chr_banks_[address & 0x07] = value;

		debug_update_chr_mapping();
		break;


	// Background CHR bank registers.
	case 0x5128:
	case 0x5129:
	case 0x512a:
	case 0x512b:
	case 0x512c:
	case 0x512d:
	case 0x512e:
	case 0x512f:
		last_chr_write_ = CHR_BANK_B;

		bg_chr_banks_[(address & 0x07) ^ 0x00] = value;
		bg_chr_banks_[(address & 0x07) ^ 0x04] = value;

		debug_update_chr_mapping();
		break;


	case 0x5130:
		bg_char_upper_ = value << 8;
		debug_update_chr_mapping();
		break;


	// Vertical split.
	case 0x5200:
		vertical_split_mode_ = value;
		break;

	case 0x5201:
		vertical_split_scroll_ = value;
		break;

	case 0x5202:
		vertical_split_bank_ = value;
		break;


	// Scanline IRQ.
	case 0x5203:
		irq_target_ = value;
		break;

	case 0x5204:
		irq_enabled_ = (value & 0x80) != 0;
		update_irq_line();
		break;


	// Hardware multiplier.
	case 0x5205:
		multiplier_1_ = value;
		break;

	case 0x5206:
		multiplier_2_ = value;
		break;


	default:
		if (address >= 0x5c00) {
			switch (exram_mode_ & 0x03) {
			case 0x00:
			case 0x01:
			case 0x02:
				exram_[address & 0x03ff] = value;
				break;

			case 0x03:
				break;
			}
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
// This includes MMC5 audio status, scanline IRQ status, the hardware
// multiplier result, and ExRAM access.  Reading either IRQ-status register
// acknowledges only that IRQ source; the shared mapper IRQ line remains
// asserted if another enabled MMC5 IRQ source is still pending.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value visible to the CPU.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_5 (uint_least16_t address)
{
	uint8_t ret = address >> 8;

	switch (address) {
	// PCM IRQ status.
	case 0x5010:
		ret = 0x01;

		if (pcm_.irq_enabled && pcm_.irq_pending) {
			ret |= 0x80;
		}

		pcm_.irq_pending = false;
		update_irq_line();
		break;

	// MMC5 pulse-channel length status.
	case 0x5015:
		ret = 0;

		if (pulse_[0].length_counter > 0) {
			ret |= 0x01;
		}

		if (pulse_[1].length_counter > 0) {
			ret |= 0x02;
		}
		break;

	// Reading scanline IRQ status acknowledges only the scanline IRQ.
	case 0x5204:
		ret = irq_status_.raw;
		irq_status_.pending = false;
		update_irq_line();
		break;

	// The multiplier result is exposed as low/high bytes at $5205/$5206.
	case 0x5205:
	{
		const uint16_t x =
			static_cast<uint16_t>(multiplier_1_) *
			static_cast<uint16_t>(multiplier_2_);

		ret = x & 0xff;
		break;
	}

	case 0x5206:
	{
		const uint16_t x =
			static_cast<uint16_t>(multiplier_1_) *
			static_cast<uint16_t>(multiplier_2_);

		ret = (x >> 8) & 0xff;
		break;
	}

	default:
		if (address >= 0x5c00) {
			switch (exram_mode_ & 0x03) {
			case 0x00:
			case 0x01:
				break;

			case 0x02:
			case 0x03:
				ret = exram_[address & 0x03ff];
				break;
			}
		}
		break;
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
// Routes CPU reads in the $8000-$8FFF range through the MMC5 memory handler and
// applies PCM read-mode behavior when enabled.
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
	return read_pcm_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_9
//
// Routes CPU reads in the $9000-$9FFF range through the MMC5 memory handler and
// applies PCM read-mode behavior when enabled.
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
	return read_pcm_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_a
//
// Routes CPU reads in the $A000-$AFFF range through the MMC5 memory handler and
// applies PCM read-mode behavior when enabled.
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
	return read_pcm_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper5::read_b
//
// Routes CPU reads in the $B000-$BFFF range through the MMC5 memory handler and
// applies PCM read-mode behavior when enabled.
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
	return read_pcm_memory(address);
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
// Mapper5::read_pcm_memory
//
// Reads from the normal MMC5 PRG mapping and applies PCM read-mode behavior.
//
// When PCM read mode is enabled, CPU reads from $8000-$BFFF feed the returned
// byte into the MMC5 PCM DAC.  A nonzero byte becomes the new DAC value.  A
// zero byte leaves the DAC unchanged and raises the PCM IRQ-pending state.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value read from the normal MMC5 PRG mapping.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_pcm_memory (uint_least16_t address)
{
	const uint8_t value = read_handler(address);

	if (pcm_.read_mode) {
		if (value == 0) {
			pcm_.irq_pending = true;
		} else {
			pcm_.value = value;
			pcm_.irq_pending = false;
		}

		update_irq_line();
	}

	return value;
}


// -----------------------------------------------------------------------------
// Mapper5::read_vram
//
// Handles MMC5 PPU address-space reads.
//
// Nametable reads are routed according to MMC5 nametable mapping, ExRAM mode,
// fill mode, Extended Attribute mode, and vertical-split state.
//
// Pattern-table reads select the appropriate MMC5 background or sprite CHR
// register set, with special handling for Extended Attribute and vertical-split
// background fetches.
//
// Parameters:
//   address - PPU address being read.
//
// Returns:
//   Value visible to the PPU.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::read_vram(uint_least16_t address)
{
	update_vertical_split(address);

	const uint16_t offset = address & 0x03ff;


	// Extended Attribute mode associates one ExRAM byte with each background
	// tile. Latch that byte when the PPU performs the nametable tile fetch.
	//
	// Do not update the Extended Attribute latch while rendering the vertical
	// split region; split mode uses ExRAM as its nametable source and does not
	// use Extended Attribute mode inside the split region.
	if (irq_status_.in_frame &&
		(exram_mode_ & 0x03) == 0x01 &&
		!vertical_split_active_ &&
		(address & 0x2000) &&
		offset < 0x03c0) {

		extended_attribute_ = exram_[offset];
	}


	switch ((address >> 10) & 0x0f) {
	case 0x08:
	case 0x0c:
		// $2000 / $3000

		if (vertical_split_active_) {
			if (offset < 0x03c0) {
				return exram_[vertical_split_tile_ & 0x03ff];
			}

			const uint8_t shift =
				((vertical_split_tile_ >> 4) & 0x04) |
				(vertical_split_tile_ & 0x02);

			const uint16_t attribute_address =
				0x03c0 |
				((vertical_split_tile_ & 0x0380) >> 4) |
				((vertical_split_tile_ & 0x001f) >> 2);

			const uint8_t palette =
				(exram_[attribute_address] >> shift) & 0x03;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		// Extended Attribute mode supplies the palette from the latched
		// ExRAM byte rather than from the normal attribute table.
		if (irq_status_.in_frame &&
			(exram_mode_ & 0x03) == 0x01 &&
			offset >= 0x03c0) {

			const uint8_t palette =
				extended_attribute_ >> 6;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		switch (mirroring_mode_ & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);

		case 0x01:
			return Mapper::read_vram(address);

		case 0x02:
			return (exram_mode_ & 0x02)
				? 0x00
				: exram_[offset];

		case 0x03:
			if (offset < 0x03c0) {
				return fill_mode_tile_;
			}

			{
				const uint8_t palette =
					fill_mode_attr_ & 0x03;

				return palette |
					(palette << 2) |
					(palette << 4) |
					(palette << 6);
			}
		}
		break;


	case 0x09:
	case 0x0d:
		// $2400 / $3400

		if (vertical_split_active_) {
			if (offset < 0x03c0) {
				return exram_[vertical_split_tile_ & 0x03ff];
			}

			const uint8_t shift =
				((vertical_split_tile_ >> 4) & 0x04) |
				(vertical_split_tile_ & 0x02);

			const uint16_t attribute_address =
				0x03c0 |
				((vertical_split_tile_ & 0x0380) >> 4) |
				((vertical_split_tile_ & 0x001f) >> 2);

			const uint8_t palette =
				(exram_[attribute_address] >> shift) & 0x03;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		if (irq_status_.in_frame &&
			(exram_mode_ & 0x03) == 0x01 &&
			offset >= 0x03c0) {

			const uint8_t palette =
				extended_attribute_ >> 6;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		switch ((mirroring_mode_ >> 2) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);

		case 0x01:
			return Mapper::read_vram(address);

		case 0x02:
			return (exram_mode_ & 0x02)
				? 0x00
				: exram_[offset];

		case 0x03:
			if (offset < 0x03c0) {
				return fill_mode_tile_;
			}

			{
				const uint8_t palette =
					fill_mode_attr_ & 0x03;

				return palette |
					(palette << 2) |
					(palette << 4) |
					(palette << 6);
			}
		}
		break;


	case 0x0a:
	case 0x0e:
		// $2800 / $3800

		if (vertical_split_active_) {
			if (offset < 0x03c0) {
				return exram_[vertical_split_tile_ & 0x03ff];
			}

			const uint8_t shift =
				((vertical_split_tile_ >> 4) & 0x04) |
				(vertical_split_tile_ & 0x02);

			const uint16_t attribute_address =
				0x03c0 |
				((vertical_split_tile_ & 0x0380) >> 4) |
				((vertical_split_tile_ & 0x001f) >> 2);

			const uint8_t palette =
				(exram_[attribute_address] >> shift) & 0x03;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		if (irq_status_.in_frame &&
			(exram_mode_ & 0x03) == 0x01 &&
			offset >= 0x03c0) {

			const uint8_t palette =
				extended_attribute_ >> 6;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		switch ((mirroring_mode_ >> 4) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);

		case 0x01:
			return Mapper::read_vram(address);

		case 0x02:
			return (exram_mode_ & 0x02)
				? 0x00
				: exram_[offset];

		case 0x03:
			if (offset < 0x03c0) {
				return fill_mode_tile_;
			}

			{
				const uint8_t palette =
					fill_mode_attr_ & 0x03;

				return palette |
					(palette << 2) |
					(palette << 4) |
					(palette << 6);
			}
		}
		break;


	case 0x0b:
	case 0x0f:
		// $2c00 / $3c00

		if (vertical_split_active_) {
			if (offset < 0x03c0) {
				return exram_[vertical_split_tile_ & 0x03ff];
			}

			const uint8_t shift =
				((vertical_split_tile_ >> 4) & 0x04) |
				(vertical_split_tile_ & 0x02);

			const uint16_t attribute_address =
				0x03c0 |
				((vertical_split_tile_ & 0x0380) >> 4) |
				((vertical_split_tile_ & 0x001f) >> 2);

			const uint8_t palette =
				(exram_[attribute_address] >> shift) & 0x03;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		if (irq_status_.in_frame &&
			(exram_mode_ & 0x03) == 0x01 &&
			offset >= 0x03c0) {

			const uint8_t palette =
				extended_attribute_ >> 6;

			return palette |
				(palette << 2) |
				(palette << 4) |
				(palette << 6);
		}


		switch ((mirroring_mode_ >> 6) & 0x03) {
		case 0x00:
			return Mapper::read_vram(address);

		case 0x01:
			return Mapper::read_vram(address);

		case 0x02:
			return (exram_mode_ & 0x02)
				? 0x00
				: exram_[offset];

		case 0x03:
			if (offset < 0x03c0) {
				return fill_mode_tile_;
			}

			{
				const uint8_t palette =
					fill_mode_attr_ & 0x03;

				return palette |
					(palette << 2) |
					(palette << 4) |
					(palette << 6);
			}
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
	{
		// CHR-ROM ($0000-$1fff).

		const bool sprite_fetch =
			ppu_fetch_type_ == PPUFetchType::SPRITE;

		const uint8_t *const chr_rom =
			nes::cart.chr();

		const uint32_t chr_mask =
			nes::cart.chr_mask();


		// Vertical-split background pattern fetches use the independent
		// 4 KB CHR bank selected by $5202.
		//
		// The low three CHR address bits come from the MMC5 split-region
		// vertical position rather than the PPU's normal fine-Y value.
		if (vertical_split_active_ && !sprite_fetch) {
			const uint16_t split_pattern_address =
				(address & 0x0ff8) |
				(vertical_split_y_ & 0x07);

			const uint32_t chr_address =
				(static_cast<uint32_t>(vertical_split_bank_) * 0x1000) +
				split_pattern_address;

			return chr_rom[chr_address & chr_mask];
		}

		// Extended Attribute mode supplies six CHR bank bits from the
		// currently latched ExRAM byte. $5130 supplies the upper two bits.
		if (irq_status_.in_frame &&
			(exram_mode_ & 0x03) == 0x01 &&
			!sprite_fetch) {

			const uint16_t bank =
				static_cast<uint16_t>(
					extended_attribute_ & 0x3f
				) |
				(
					static_cast<uint16_t>(
						bg_char_upper_ >> 2
					) & 0x00c0
				);

			const uint32_t chr_address =
				(static_cast<uint32_t>(bank) * 0x1000) +
				(address & 0x0fff);

			return chr_rom[chr_address & chr_mask];
		}


		const uint8_t *chr_selector;

		// During active rendering with 8x16 sprites, use A for sprite
		// pattern fetches and B for background pattern fetches.
		//
		// Outside active rendering, CPU $2007 accesses see whichever CHR
		// register set was written most recently.
		if (irq_status_.in_frame && large_sprites_) {
			if (sprite_fetch) {
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


		const uint8_t *chr_rom_banks[8];

		switch (chr_mode_ & 0x03) {
		case 0x00:
			// 8 KB mode.
			chr_rom_banks[0] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x0000;

			chr_rom_banks[1] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x0400;

			chr_rom_banks[2] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x0800;

			chr_rom_banks[3] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x0c00;

			chr_rom_banks[4] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x1000;

			chr_rom_banks[5] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x1400;

			chr_rom_banks[6] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x1800;

			chr_rom_banks[7] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x2000) &
					chr_mask) +
				0x1c00;
			break;


		case 0x01:
			// 4 KB mode.
			chr_rom_banks[0] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0000;

			chr_rom_banks[1] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0400;

			chr_rom_banks[2] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0800;

			chr_rom_banks[3] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0c00;

			chr_rom_banks[4] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0000;

			chr_rom_banks[5] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0400;

			chr_rom_banks[6] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0800;

			chr_rom_banks[7] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x1000) &
					chr_mask) +
				0x0c00;
			break;


		case 0x02:
			// 2 KB mode.
			chr_rom_banks[0] =
				chr_rom +
				(((chr_selector[1] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0000;

			chr_rom_banks[1] =
				chr_rom +
				(((chr_selector[1] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0400;

			chr_rom_banks[2] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0000;

			chr_rom_banks[3] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0400;

			chr_rom_banks[4] =
				chr_rom +
				(((chr_selector[5] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0000;

			chr_rom_banks[5] =
				chr_rom +
				(((chr_selector[5] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0400;

			chr_rom_banks[6] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0000;

			chr_rom_banks[7] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x0800) &
					chr_mask) +
				0x0400;
			break;


		case 0x03:
			// 1 KB mode.
			chr_rom_banks[0] =
				chr_rom +
				(((chr_selector[0] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[1] =
				chr_rom +
				(((chr_selector[1] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[2] =
				chr_rom +
				(((chr_selector[2] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[3] =
				chr_rom +
				(((chr_selector[3] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[4] =
				chr_rom +
				(((chr_selector[4] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[5] =
				chr_rom +
				(((chr_selector[5] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[6] =
				chr_rom +
				(((chr_selector[6] + bg_char_upper_) * 0x0400) &
					chr_mask);

			chr_rom_banks[7] =
				chr_rom +
				(((chr_selector[7] + bg_char_upper_) * 0x0400) &
					chr_mask);
			break;
		}


		return chr_rom_banks[(address >> 10) & 0x07]
			[address & 0x03ff];
		}
	}

	return Mapper::read_vram(address);
}


// -----------------------------------------------------------------------------
// Mapper5::write_vram
//
// Handles MMC5 PPU address-space writes.
//
// Nametable writes are routed to CIRAM or ExRAM according to the MMC5 nametable
// and ExRAM configuration.  Fill-mode nametables are read-only from the PPU's
// point of view.  CHR-ROM pattern-table writes are ignored.
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
	// -------------------------------------------------------------------------
	// $2000 / $3000
	// -------------------------------------------------------------------------

	case 0x08:
	case 0x0c:
		switch (mirroring_mode_ & 0x03) {
		case 0x00:
		case 0x01:
			Mapper::write_vram(address, value);
			break;

		case 0x02:
			if (!(exram_mode_ & 0x02)) {
				exram_[address & 0x03ff] = value;
			}
			break;

		case 0x03:
			break;
		}
		return;


	// -------------------------------------------------------------------------
	// $2400 / $3400
	// -------------------------------------------------------------------------

	case 0x09:
	case 0x0d:
		switch ((mirroring_mode_ >> 2) & 0x03) {
		case 0x00:
		case 0x01:
			Mapper::write_vram(address, value);
			break;

		case 0x02:
			if (!(exram_mode_ & 0x02)) {
				exram_[address & 0x03ff] = value;
			}
			break;

		case 0x03:
			break;
		}
		return;


	// -------------------------------------------------------------------------
	// $2800 / $3800
	// -------------------------------------------------------------------------

	case 0x0a:
	case 0x0e:
		switch ((mirroring_mode_ >> 4) & 0x03) {
		case 0x00:
		case 0x01:
			Mapper::write_vram(address, value);
			break;

		case 0x02:
			if (!(exram_mode_ & 0x02)) {
				exram_[address & 0x03ff] = value;
			}
			break;

		case 0x03:
			break;
		}
		return;


	// -------------------------------------------------------------------------
	// $2C00 / $3C00
	// -------------------------------------------------------------------------

	case 0x0b:
	case 0x0f:
		switch ((mirroring_mode_ >> 6) & 0x03) {
		case 0x00:
		case 0x01:
			Mapper::write_vram(address, value);
			break;

		case 0x02:
			if (!(exram_mode_ & 0x02)) {
				exram_[address & 0x03ff] = value;
			}
			break;

		case 0x03:
			break;
		}
		return;


	// -------------------------------------------------------------------------
	// Pattern tables
	// -------------------------------------------------------------------------

	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		// MMC5 cartridges use CHR-ROM here; pattern-table writes have no effect.
		return;
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
	// When this is > 128 (32 * 4), we are fetching sprites, not BG tiles.
	++fetch_count_;

	// Three consecutive reads of the same nametable address are treated as the
	// MMC5 scanline clock condition.
	if (vram_address == prev_vram_address_[0] &&
		vram_address == prev_vram_address_[1] &&
		(vram_address & 0x2000)) {

		clock_irq();
		fetch_count_ = 0;
	}

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
// The MMC5 vertical-split background-fetch sequence is restarted at each
// detected scanline.
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
	vertical_split_tile_number_ = 0xff;
	vertical_split_active_ = false;

	if (!irq_status_.in_frame) {
		irq_status_.in_frame = true;
		irq_status_.pending = false;
		irq_counter_ = 0;

		update_irq_line();
		return;
	}

	if (++irq_counter_ == irq_target_) {
		irq_status_.pending = true;
		update_irq_line();
	}
}


// -----------------------------------------------------------------------------
// Mapper5::ppu_end_frame
//
// Clears the MMC5 rendering-frame and vertical-split fetch state at the end of
// a PPU frame.
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
	irq_status_.in_frame = false;

	vertical_split_active_ = false;
	vertical_split_tile_number_ = 0;
	vertical_split_y_ = 0;
	vertical_split_tile_ = 0;
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


// -----------------------------------------------------------------------------
// Mapper5::clock_audio_pulse
//
// Advances one MMC5 pulse-channel timer by one APU audio clock.
//
// The MMC5 pulse timer operates like the native NES pulse timer.  When the
// timer expires it is reloaded from the programmed 11-bit period and the
// eight-step duty sequencer advances by one position.
//
// Parameters:
//   channel - MMC5 pulse channel to advance.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::clock_audio_pulse (mmc5_pulse_channel_t &channel)
{
	if (channel.timer == 0) {
		channel.timer = channel.period;

		channel.sequence_step =
			(channel.sequence_step + 1) & 0x07;
	} else {
		--channel.timer;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::cpu_sync
//
// Advances the MMC5 expansion-audio state by one CPU cycle.
//
// The pulse timers are clocked once every two CPU cycles.  The MMC5 envelope
// and length-counter units are clocked independently from a fixed 7424-cycle
// divider, corresponding to approximately 241 Hz on an NTSC CPU.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::cpu_sync()
{
	audio_half_clock_ = !audio_half_clock_;

	if (audio_half_clock_) {
		clock_audio_pulse(pulse_[0]);
		clock_audio_pulse(pulse_[1]);
	}

	if (audio_frame_counter_ > 0) {
		--audio_frame_counter_;
	}

	if (audio_frame_counter_ == 0) {
		audio_frame_counter_ = 7424;
		clock_audio_frame();
	}
}


// -----------------------------------------------------------------------------
// Mapper5::audio_pulse_output
//
// Returns the current raw output level of one MMC5 pulse channel.
//
// A channel is silent when disabled, when its length counter has expired, or
// when the active duty-sequence step is low.
//
// MMC5 pulse channels do not have the native APU's minimum-period silence rule,
// so timer periods below 8 are allowed to produce output.
//
// Envelope timing has not yet been added, so constant-volume mode currently
// uses the programmed volume directly.  Envelope mode will use envelope_decay
// once the envelope clock is implemented.
//
// Parameters:
//   channel - MMC5 pulse channel whose output is requested.
//
// Returns:
//   Current raw pulse output in the range 0-15.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::audio_pulse_output (const mmc5_pulse_channel_t &channel) const
{
	if (!channel.enabled ||
		channel.length_counter == 0) {

		return 0;
	}

	if (kMMC5DutyTable[channel.duty & 0x03]
			[channel.sequence_step & 0x07] == 0) {

		return 0;
	}

	if (channel.constant_volume) {
		return channel.volume & 0x0f;
	}

	return channel.envelope_decay & 0x0f;
}


// -----------------------------------------------------------------------------
// Mapper5::clock_audio_envelope
//
// Advances one MMC5 pulse-channel envelope by one fixed audio-frame clock.
//
// Writing the channel's high timer register sets envelope_start.  On the next
// envelope clock the decay level is reloaded to 15 and the divider is loaded
// from the programmed envelope period.
//
// When the divider expires, the decay level decreases until it reaches zero.
// If length_halt is set, the envelope loops from zero back to 15.
//
// Parameters:
//   channel - MMC5 pulse channel whose envelope is being advanced.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::clock_audio_envelope (mmc5_pulse_channel_t &channel)
{
	if (channel.envelope_start) {
		channel.envelope_start = false;
		channel.envelope_decay = 15;
		channel.envelope_divider = channel.volume;

		return;
	}

	if (channel.envelope_divider > 0) {
		--channel.envelope_divider;
		return;
	}

	channel.envelope_divider = channel.volume;

	if (channel.envelope_decay > 0) {
		--channel.envelope_decay;
	} else if (channel.length_halt) {
		channel.envelope_decay = 15;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::clock_audio_length
//
// Advances one MMC5 pulse-channel length counter.
//
// The counter decrements while nonzero unless the channel's length-halt flag is
// set.  Reaching zero silences the channel through audio_pulse_output().
//
// Parameters:
//   channel - MMC5 pulse channel whose length counter is being advanced.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::clock_audio_length (mmc5_pulse_channel_t &channel)
{
	if (channel.length_counter > 0 &&
		!channel.length_halt) {

		--channel.length_counter;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::clock_audio_frame
//
// Advances the MMC5 pulse-channel envelope and length units.
//
// MMC5 does not use the native APU frame sequencer.  Both units are instead
// clocked from the mapper's fixed audio timing divider.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::clock_audio_frame()
{
	clock_audio_envelope(pulse_[0]);
	clock_audio_envelope(pulse_[1]);

	clock_audio_length(pulse_[0]);
	clock_audio_length(pulse_[1]);
}


// -----------------------------------------------------------------------------
// Mapper5::audio_channel_count
//
// Returns the number of independently controllable MMC5 audio channels.
//
// Parameters:
//   None.
//
// Returns:
//   Number of MMC5 expansion-audio channels.
// -----------------------------------------------------------------------------
size_t
Mapper5::audio_channel_count() const
{
	return 3;
}


// -----------------------------------------------------------------------------
// Mapper5::audio_channel_name
//
// Returns the display name for an MMC5 expansion-audio channel.
//
// Parameters:
//   index - Zero-based MMC5 audio-channel index.
//
// Returns:
//   Human-readable channel name, or nullptr for an invalid index.
// -----------------------------------------------------------------------------
const char *
Mapper5::audio_channel_name (size_t index) const
{
	static const char *kChannelNames[3] = {
		"Pulse 1",
		"Pulse 2",
		"PCM"
	};

	if (index >= 3) {
		return nullptr;
	}

	return kChannelNames[index];
}


// -----------------------------------------------------------------------------
// Mapper5::audio_channel_muted
//
// Returns whether an MMC5 expansion-audio channel is currently muted.
//
// Parameters:
//   index - Zero-based MMC5 audio-channel index.
//
// Returns:
//   true if the channel is muted; false otherwise.
// -----------------------------------------------------------------------------
bool
Mapper5::audio_channel_muted (size_t index) const
{
	switch (index) {
	case 0:
	case 1:
		return pulse_[index].muted;

	case 2:
		return pcm_.muted;

	default:
		return false;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::set_audio_channel_muted
//
// Changes the mute state of one MMC5 expansion-audio channel.
//
// Muting affects only the final expansion-audio mix.  Channel timers,
// envelopes, length counters, and PCM state continue advancing normally.
//
// Parameters:
//   index - Zero-based MMC5 audio-channel index.
//   muted - New mute state.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::set_audio_channel_muted (size_t index, bool muted)
{
	switch (index) {
	case 0:
	case 1:
		pulse_[index].muted = muted;
		break;

	case 2:
		pcm_.muted = muted;
		break;

	default:
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper5::audio_output
//
// Returns the current composite MMC5 expansion-audio output.
//
// The two pulse channels contribute their current 0-15 output levels.  The PCM
// channel contributes its direct eight-bit output value.  Muted channels are
// omitted from the audible mix while their internal state continues running.
//
// Parameters:
//   None.
//
// Returns:
//   Current MMC5 expansion-audio sample.
// -----------------------------------------------------------------------------
uint8_t
Mapper5::audio_output() const
{
	uint32_t output = 0;

	if (!pulse_[0].muted) {
		output += audio_pulse_output(pulse_[0]);
	}

	if (!pulse_[1].muted) {
		output += audio_pulse_output(pulse_[1]);
	}

	if (!pcm_.muted) {
		output += pcm_.value;
	}

	if (output > 255) {
		output = 255;
	}

	return static_cast<uint8_t>(output);
}


// -----------------------------------------------------------------------------
// Mapper5::debug_audio_scope_sample
//
// Returns the current raw MMC5 expansion-audio channel levels for the
// oscilloscope.
//
// Scope samples intentionally ignore channel mute state so the debugger
// continues to show the underlying channel waveform even when that channel is
// muted from the audible mix.
//
// Parameters:
//   None.
//
// Returns:
//   Current MMC5 expansion-audio scope sample.
// -----------------------------------------------------------------------------
mapper_audio_scope_sample_t
Mapper5::debug_audio_scope_sample() const
{
	mapper_audio_scope_sample_t sample;

	sample.channel_count = 3;

	sample.channels[0] = audio_pulse_output(pulse_[0]);
	sample.channels[1] = audio_pulse_output(pulse_[1]);
	sample.channels[2] = pcm_.value;

	return sample;
}


// -----------------------------------------------------------------------------
// Mapper5::debug_audio_state
//
// Returns debugger-facing state for the MMC5 expansion-audio channels.
//
// The snapshot includes combined expansion-audio output information together
// with the live timer, duty, envelope, length-counter, and PCM state for the
// two pulse channels and direct PCM channel.
//
// Parameters:
//   None.
//
// Returns:
//   Current MMC5 expansion-audio debug state.
// -----------------------------------------------------------------------------
mapper_audio_debug_state_t
Mapper5::debug_audio_state() const
{
	mapper_audio_debug_state_t state;

	state.type = ExpansionAudioType::MMC5;
	state.name = "MMC5";
	state.output = audio_output();
	state.output_maximum = 255;


	// -------------------------------------------------------------------------
	// Global MMC5 audio state
	// -------------------------------------------------------------------------

	state.field_count = 4;

	state.fields[0].label = "Combined Output";
	state.fields[0].value = state.output;
	state.fields[0].type = MapperAudioDebugFieldType::Output;

	state.fields[1].label = "Output Level";
	state.fields[1].value = state.output;
	state.fields[1].maximum = state.output_maximum;
	state.fields[1].type = MapperAudioDebugFieldType::Percent;

	state.fields[2].label = "Frame Counter";
	state.fields[2].value = audio_frame_counter_;
	state.fields[2].type = MapperAudioDebugFieldType::Timing;

	state.fields[3].label = "PCM IRQ";
	state.fields[3].value = pcm_.irq_pending;
	state.fields[3].type = MapperAudioDebugFieldType::Boolean;


	// -------------------------------------------------------------------------
	// Pulse channels
	// -------------------------------------------------------------------------

	state.channel_count = 3;

	for (size_t i = 0; i < 2; ++i) {
		const mmc5_pulse_channel_t &pulse = pulse_[i];
		auto &channel = state.channels[i];

		channel.name = (i == 0) ? "MMC5 Pulse 1" : "MMC5 Pulse 2";
		channel.period = pulse.period;
		channel.output = audio_pulse_output(pulse);
		channel.output_maximum = 15;
		channel.enabled = pulse.enabled;
		channel.muted = pulse.muted;

		channel.frequency_hz =
			kMMC5NTSCClock /
			(16.0 * (static_cast<double>(pulse.period) + 1.0));

		channel.frequency_valid = true;

		channel.field_count = 8;

		channel.fields[0].label = "Output Level";
		channel.fields[0].value = channel.output;
		channel.fields[0].maximum = 15;
		channel.fields[0].type = MapperAudioDebugFieldType::Percent;

		channel.fields[1].label = "Volume";
		channel.fields[1].value =
			pulse.constant_volume ? pulse.volume : pulse.envelope_decay;

		channel.fields[2].label = "Duty";
		channel.fields[2].value = pulse.duty;

		channel.fields[3].label = "Duty Cycle";

		switch (pulse.duty & 0x03) {
		case 0:
			channel.fields[3].text = "12.5%";
			break;

		case 1:
			channel.fields[3].text = "25.0%";
			break;

		case 2:
			channel.fields[3].text = "50.0%";
			break;

		default:
			channel.fields[3].text = "75.0%";
			break;
		}

		channel.fields[3].type = MapperAudioDebugFieldType::Text;

		channel.fields[4].label = "Volume Mode";
		channel.fields[4].text =
			pulse.constant_volume ? "Constant" : "Envelope";
		channel.fields[4].type = MapperAudioDebugFieldType::Text;

		channel.fields[5].label = "Timer";
		channel.fields[5].value = pulse.timer;

		channel.fields[6].label = "Sequence Step";
		channel.fields[6].value = pulse.sequence_step;

		channel.fields[7].label = "Length Counter";
		channel.fields[7].value = pulse.length_counter;
	}


	// -------------------------------------------------------------------------
	// PCM
	// -------------------------------------------------------------------------

	state.channels[2].name = "MMC5 PCM";
	state.channels[2].output = pcm_.value;
	state.channels[2].output_maximum = 255;
	state.channels[2].enabled = pcm_.enabled;
	state.channels[2].muted = pcm_.muted;

	state.channels[2].field_count = 5;

	state.channels[2].fields[0].label = "Output Level";
	state.channels[2].fields[0].value = pcm_.value;
	state.channels[2].fields[0].maximum = 255;
	state.channels[2].fields[0].type = MapperAudioDebugFieldType::Percent;

	state.channels[2].fields[1].label = "Value";
	state.channels[2].fields[1].value = pcm_.value;
	state.channels[2].fields[1].type = MapperAudioDebugFieldType::Hex;

	state.channels[2].fields[2].label = "Read Mode";
	state.channels[2].fields[2].value = pcm_.read_mode;
	state.channels[2].fields[2].type = MapperAudioDebugFieldType::Boolean;

	state.channels[2].fields[3].label = "IRQ Enabled";
	state.channels[2].fields[3].value = pcm_.irq_enabled;
	state.channels[2].fields[3].type = MapperAudioDebugFieldType::Boolean;

	state.channels[2].fields[4].label = "IRQ Pending";
	state.channels[2].fields[4].value = pcm_.irq_pending;
	state.channels[2].fields[4].type = MapperAudioDebugFieldType::Boolean;

	return state;
}


// -----------------------------------------------------------------------------
// Mapper5::update_irq_line
//
// Updates the CPU mapper IRQ line from the MMC5's independent IRQ sources.
//
// The MMC5 scanline IRQ and PCM IRQ share the cartridge IRQ output.  The CPU
// IRQ line therefore remains asserted while either enabled source is pending.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::update_irq_line()
{
	const bool scanline_irq =
		irq_enabled_ && irq_status_.pending;

	const bool pcm_irq =
		pcm_.irq_enabled && pcm_.irq_pending;

	if (scanline_irq || pcm_irq) {
		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	} else {
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);
	}
}


// -----------------------------------------------------------------------------
// Mapper5::update_vertical_split
//
// Updates the MMC5 vertical-split state during PPU nametable fetches.
//
// Tracks the background tile position, determines whether the current tile
// belongs to the vertical-split region, and calculates the corresponding
// ExRAM nametable address.
//
// Includes temporary diagnostics for examining the rightmost tile columns
// during the SDF introduction.
//
// Parameters:
//   address - PPU address being fetched.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper5::update_vertical_split(uint_least16_t address)
{
	const uint16_t offset = address & 0x03ff;

	const bool nametable_fetch =
		(address & 0x2000) &&
		offset < 0x03c0;

	if (!nametable_fetch) {
		return;
	}

	++vertical_split_tile_number_;

	if (!irq_status_.in_frame ||
		!(vertical_split_mode_ & VSPLIT_ENABLE) ||
		(exram_mode_ & 0x02)) {

		vertical_split_active_ = false;
		return;
	}

	const uint8_t scanline =
		vertical_split_tile_number_ >= 41 ?
			static_cast<uint8_t>(irq_counter_ + 1) :
			irq_counter_;

	vertical_split_y_ =
		static_cast<uint8_t>(
			(scanline + vertical_split_scroll_) % 240);

	const uint8_t column =
		static_cast<uint8_t>(
			(vertical_split_tile_number_ + 2) % 42);

	const bool right_side =
		(vertical_split_mode_ & VSPLIT_RIGHT) != 0;

	const uint8_t delimiter =
		vertical_split_mode_ & VSPLIT_TILE;

	if (column == 0) {
		vertical_split_active_ = !right_side;
	}

	if (column == delimiter &&
		vertical_split_tile_number_ < 42) {
		vertical_split_active_ = !vertical_split_active_;
	} else if (column > 32) {
		vertical_split_active_ = false;
	}

	if (vertical_split_active_) {
		vertical_split_tile_ =
			((static_cast<uint16_t>(vertical_split_y_) & 0x00f8) << 2) |
			column;
	}
}


