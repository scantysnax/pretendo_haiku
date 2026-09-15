
#include "MMC3.h"
#include "Cart.h"
#include "Nes.h"
#include "Ppu.h"


// TODO(eteran): implement the MMC6 SRAM stuff



//------------------------------------------------------------------------------
// Name: MMC3
//
// Initializes the common MMC3/MMC6 mapper state.
//
// The initial mapping places:
//
//   - The first 16 KB of PRG ROM at $8000-$BFFF.
//   - The final 16 KB of PRG ROM at $C000-$FFFF.
//   - Either CHR ROM or CHR RAM across $0000-$1FFF.
//
// MMC3 PRG RAM occupies $6000-$7FFF but begins disabled until enabled through
// the $A001 register.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
MMC3::MMC3()
{
	// Open the 8 KB save-RAM region used by MMC3-compatible cartridges.
	prg_ptr_ = open_sram(0x2000);

	// MMC3 PRG RAM occupies $6000-$7FFF and starts disabled.
	debug_set_prg_ram(false);

	// Initial PRG mapping:
	//
	//   $8000-$BFFF -> first 16 KB
	//   $C000-$FFFF -> final 16 KB
	set_prg_89ab(0);
	set_prg_cdef(-1);

	if (nes::cart.has_chr_rom()) {
		// Map the first 8 KB of CHR ROM at $0000-$1FFF.
		set_chr_0000_1fff(0);
	} else {
		// Cartridges without CHR ROM use mapper-provided CHR RAM.
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}
}


//------------------------------------------------------------------------------
// Name: prg_bank
//
// Resolves one of the four MMC3 8 KB PRG-ROM mapping slots.
//
// The logical bank layout is:
//
//   slot 0 -> programmable PRG register 0
//   slot 1 -> programmable PRG register 1
//   slot 2 -> second-to-last PRG-ROM bank
//   slot 3 -> final PRG-ROM bank
//
// Command bit 6 swaps the location of slot 0 and slot 2, allowing the fixed
// second-to-last bank to appear at either $8000-$9FFF or $C000-$DFFF.
//
// Parameters:
//   bank - Logical PRG slot index, 0 through 3.
//
// Returns:
//   The PRG-ROM bank number that should occupy the requested slot.
//------------------------------------------------------------------------------
int
MMC3::prg_bank (int bank) const
{
	const uint8_t banks[] = {
		prg_bank_[0],
		prg_bank_[1],
		static_cast<uint8_t>(-2),
		static_cast<uint8_t>(-1)
	};

	if (bank & 0x1) {
		// Odd slots are unaffected by PRG mode.
		return banks[bank];
	} else {
		// Command bit 6 exchanges the two even PRG slots.
		return banks[bank ^ ((command_ & 0x40) >> 5)];
	}
}


//------------------------------------------------------------------------------
// Name: chr_bank
//
// Resolves one of the eight 1 KB CHR mapping slots.
//
// Command bit 7 inverts the MMC3 CHR arrangement by swapping the lower and
// upper 4 KB pattern-table halves.
//
// Parameters:
//   bank - Logical 1 KB CHR slot index, 0 through 7.
//
// Returns:
//   The CHR bank number currently assigned to the requested slot.
//------------------------------------------------------------------------------
int
MMC3::chr_bank (int bank) const
{
	return chr_bank_[bank ^ ((command_ & 0x80) >> 5)];
}

//------------------------------------------------------------------------------
// Name: name
//
// Returns the human-readable MMC3/MMC6 hardware revision currently selected.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper revision name.
//------------------------------------------------------------------------------
std::string
MMC3::name() const
{
	switch (mode_) {
	case ModeA:
		return "MMC3A";

	case ModeB:
		return "MMC3B";

	case ModeMMC6:
		return "MMC6";
	}

	return "MMC3/MMC6";
}


//------------------------------------------------------------------------------
// Name: read_6
//
// Reads from CPU addresses $6000-$6FFF.
//
// When MMC3 PRG RAM is enabled, and the cartridge is not using four-screen
// nametable RAM, reads are serviced from the mapper's 8 KB save-RAM region.
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
MMC3::read_6 (uint_least16_t address)
{
	if (save_ram_enabled_ && nes::cart.mirroring() != Cart::MIR_4SCREEN) {
		return prg_ptr_[address & 0x1fff];
	} else {
		return Mapper::read_6(address);
	}
}


//------------------------------------------------------------------------------
// Name: read_7
//
// Reads from CPU addresses $7000-$7FFF.
//
// This range is the upper half of the same 8 KB PRG-RAM window handled by
// read_6().
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   The byte read from PRG RAM or the base mapper.
//------------------------------------------------------------------------------
uint8_t
MMC3::read_7 (uint_least16_t address)
{
	return read_6(address);
}


//------------------------------------------------------------------------------
// Name: write_6
//
// Writes to CPU addresses $6000-$6FFF.
//
// Writes are accepted only when PRG RAM is enabled, write protection is off,
// and the cartridge is not using the four-screen configuration handled by
// this implementation.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte to store.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_6 (uint_least16_t address, uint8_t value)
{
	if (save_ram_enabled_ &&
		save_ram_writable_ &&
		nes::cart.mirroring() != Cart::MIR_4SCREEN) {
		prg_ptr_[address & 0x1fff] = value;
	}
}


//------------------------------------------------------------------------------
// Name: write_7
//
// Writes to CPU addresses $7000-$7FFF.
//
// This range shares the same PRG-RAM behavior as write_6().
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte to store.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_7 (uint_least16_t address, uint8_t value)
{
	write_6(address, value);
}


//------------------------------------------------------------------------------
// Name: write_8
//
// Handles MMC3 bank-select and bank-data writes in the $8000-$8FFF range.
//
// Even addresses select the target bank register and set the PRG/CHR mode
// bits. Odd addresses write data into the selected register.
//
// Register selection:
//
//   0 -> CHR banks 0-1, treated as one 2 KB pair
//   1 -> CHR banks 2-3, treated as one 2 KB pair
//   2 -> CHR bank 4
//   3 -> CHR bank 5
//   4 -> CHR bank 6
//   5 -> CHR bank 7
//   6 -> PRG bank 0
//   7 -> PRG bank 1
//
// After a bank-data write, all resolved PRG and CHR mappings are synchronized
// so the active CPU and PPU mappings reflect the current MMC3 register state.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0001) {
	case 0x0000:
		// $8000: bank-select register.
		//
		// Bits 0-2 select the target register.
		// Bit 6 controls PRG-ROM arrangement.
		// Bit 7 controls CHR-ROM arrangement.
		command_ = value;
		break;

	case 0x0001:
		// $8001: bank-data register.
		switch (command_ & 0x07) {
		case 0:
			// Register 0 selects a 2 KB CHR pair.
			// The low bit is forced even/odd for the two 1 KB slots.
			chr_bank_[0] = (value & 0xfe) | 0x00;
			chr_bank_[1] = (value & 0xfe) | 0x01;
			break;

		case 1:
			// Register 1 selects the second 2 KB CHR pair.
			chr_bank_[2] = (value & 0xfe) | 0x00;
			chr_bank_[3] = (value & 0xfe) | 0x01;
			break;

		case 2:
			chr_bank_[4] = value;
			break;

		case 3:
			chr_bank_[5] = value;
			break;

		case 4:
			chr_bank_[6] = value;
			break;

		case 5:
			chr_bank_[7] = value;
			break;

		case 6:
			// PRG bank register 0.
			prg_bank_[0] = value & 0x3f;
			break;

		case 7:
			// PRG bank register 1.
			prg_bank_[1] = value & 0x3f;
			break;
		}

		// Resolve and synchronize the four 8 KB PRG-ROM slots.
		set_prg_89(prg_bank(0));
		set_prg_ab(prg_bank(1));
		set_prg_cd(prg_bank(2));
		set_prg_ef(prg_bank(3));

		if (nes::cart.has_chr_rom()) {
			// Synchronize the eight resolved 1 KB CHR-ROM slots.
			set_chr_0000_03ff(chr_bank(0));
			set_chr_0400_07ff(chr_bank(1));
			set_chr_0800_0bff(chr_bank(2));
			set_chr_0c00_0fff(chr_bank(3));
			set_chr_1000_13ff(chr_bank(4));
			set_chr_1400_17ff(chr_bank(5));
			set_chr_1800_1bff(chr_bank(6));
			set_chr_1c00_1fff(chr_bank(7));
		} else {
			// Synchronize the eight resolved 1 KB CHR-RAM slots.
			set_chr_0000_03ff_ram(chr_ram_, chr_bank(0));
			set_chr_0400_07ff_ram(chr_ram_, chr_bank(1));
			set_chr_0800_0bff_ram(chr_ram_, chr_bank(2));
			set_chr_0c00_0fff_ram(chr_ram_, chr_bank(3));
			set_chr_1000_13ff_ram(chr_ram_, chr_bank(4));
			set_chr_1400_17ff_ram(chr_ram_, chr_bank(5));
			set_chr_1800_1bff_ram(chr_ram_, chr_bank(6));
			set_chr_1c00_1fff_ram(chr_ram_, chr_bank(7));
		}

		break;
	}
}


//------------------------------------------------------------------------------
// Name: write_9
//
// Mirrors the MMC3 $8000/$8001 bank-control registers throughout
// $9000-$9FFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_9 (uint_least16_t address, uint8_t value)
{
	write_8(address, value);
}


//------------------------------------------------------------------------------
// Name: write_a
//
// Handles MMC3 writes in the $A000-$AFFF range.
//
// Even addresses control nametable mirroring:
//
//   bit 0 clear -> vertical mirroring
//   bit 0 set   -> horizontal mirroring
//
// Four-screen cartridges ignore mapper mirroring changes.
//
// Odd addresses control PRG RAM:
//
//   bit 7 -> PRG-RAM enable
//   bit 6 -> PRG-RAM write protect, where clear means writable
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0001) {
	case 0x0000:
		// $A000: nametable mirroring control.
		if (nes::cart.mirroring() != Cart::MIR_4SCREEN) {
			if (value & 0x01) {
				set_mirroring(mirror_horizontal);
			} else {
				set_mirroring(mirror_vertical);
			}
		}
		break;

	case 0x0001:
		// $A001: PRG-RAM enable and write-protect control.
		save_ram_enabled_  = ((value & 0x80) != 0);
		save_ram_writable_ = ((value & 0x40) == 0);

		// Keep the Mapper Explorer state synchronized with the effective
		// PRG-RAM mapping and write-protection state.
		debug_set_prg_ram(
			save_ram_enabled_ &&
			nes::cart.mirroring() != Cart::MIR_4SCREEN,
			save_ram_writable_);
		break;
	}
}


//------------------------------------------------------------------------------
// Name: write_b
//
// Mirrors the MMC3 $A000/$A001 register behavior throughout $B000-$BFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_b (uint_least16_t address, uint8_t value)
{
	write_a(address, value);
}


//------------------------------------------------------------------------------
// Name: write_c
//
// Handles MMC3 IRQ-counter writes in the $C000-$CFFF range.
//
// Even addresses set the IRQ reload value.
// Odd addresses request that the IRQ counter be reloaded.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0001) {
	case 0x0000:
		// $C000: set the IRQ reload value.
		irq_latch_ = value;
		break;

	case 0x0001:
		// $C001: request an IRQ-counter reload.
		irq_counter_ = 0x00;
		irq_reload_  = true;
		break;
	}
}


//------------------------------------------------------------------------------
// Name: write_d
//
// Mirrors the MMC3 $C000/$C001 IRQ-counter registers throughout
// $D000-$DFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_d (uint_least16_t address, uint8_t value)
{
	write_c(address, value);
}


//------------------------------------------------------------------------------
// Name: write_e
//
// Handles MMC3 IRQ enable/disable writes in the $E000-$EFFF range.
//
// Even addresses disable mapper IRQ generation and acknowledge any currently
// asserted mapper IRQ.
//
// Odd addresses enable mapper IRQ generation.
//
// Parameters:
//   address - CPU address being written.
//   value   - Written value; not used by these registers.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_e (uint_least16_t address, uint8_t value)
{
	(void)value;

	switch (address & 0x0001) {
	case 0x0000:
		// $E000: disable mapper IRQs and acknowledge the current IRQ.
		irq_enabled_ = false;
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);
		break;

	case 0x0001:
		// $E001: enable mapper IRQ generation.
		irq_enabled_ = true;
		break;
	}
}


//------------------------------------------------------------------------------
// Name: write_f
//
// Mirrors the MMC3 $E000/$E001 IRQ-control registers throughout
// $F000-$FFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte written by the CPU.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::write_f (uint_least16_t address, uint8_t value)
{
	write_e(address, value);
}


//------------------------------------------------------------------------------
// Name: vram_change_hook
//
// Monitors PPU VRAM address changes for rising edges of pattern-table address
// bit A12.
//
// MMC3 uses qualified A12 rising edges to clock its scanline IRQ counter. A
// transition is accepted only when enough PPU cycles have elapsed since the
// previous qualified edge.
//
// Parameters:
//   vram_address - New PPU VRAM address.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::vram_change_hook (uint_least16_t vram_address)
{
	// Detect a rising edge of PPU address bit A12.
	if ((vram_address & 0x1000) && !(prev_vram_address_ & 0x1000)) {
		// Require sufficient spacing between qualified A12 edges.
		if ((nes::ppu::cycle_count() - prev_ppu_cycle_) >= 16) {
			clock_irq();
		}

		prev_ppu_cycle_ = nes::ppu::cycle_count();
	}

	prev_vram_address_ = vram_address;
}


//------------------------------------------------------------------------------
// Name: clock_irqA
//
// Clocks the MMC3A-style IRQ counter.
//
// When the counter is zero, it is reloaded from irq_latch_. Otherwise it is
// decremented. MMC3A IRQ generation additionally depends on irq_reload_ being
// set when the resulting counter value reaches zero.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::clock_irqA()
{
	if (irq_counter_ == 0x00) {
		// Reload a zero counter from the programmed latch value.
		irq_counter_ = irq_latch_;
	} else {
		// Otherwise decrement the active counter.
		--irq_counter_;

		// Record that a counter transition/reload condition occurred.
		irq_reload_ = true;
	}

	// MMC3A requires both a zero counter and the reload condition before
	// asserting an enabled mapper IRQ.
	if (irq_enabled_ && irq_counter_ == 0 && irq_reload_) {
		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	}

	irq_reload_ = false;
}


//------------------------------------------------------------------------------
// Name: clock_irqB
//
// Clocks the MMC3B-style IRQ counter.
//
// The counter reload/decrement behavior is similar to MMC3A, but MMC3B can
// assert the IRQ whenever the resulting counter value reaches zero while IRQs
// are enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::clock_irqB()
{
	if (irq_counter_ == 0x00) {
		// Reload a zero counter from the programmed latch value.
		irq_counter_ = irq_latch_;
	} else {
		// Otherwise decrement the active counter.
		--irq_counter_;
		irq_reload_ = true;
	}

	if (irq_enabled_ && irq_counter_ == 0) {
		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	}

	irq_reload_ = false;
}


//------------------------------------------------------------------------------
// Name: clock_irq
//
// Dispatches one qualified MMC3 IRQ-counter clock to the implementation
// appropriate for the selected MMC3 hardware revision.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
void
MMC3::clock_irq()
{
	if (mode_ == ModeA) {
		clock_irqA();
	} else {
		clock_irqB();
	}
}

