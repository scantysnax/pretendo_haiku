
#include "Cart.h"
#include "Compiler.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"
#include "Settings.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <cstdio>


// -----------------------------------------------------------------------------
// Mapper::Mapper
//
// Initializes the base mapper state.
//
// Nametable RAM is cleared and the cartridge's initial mirroring mode is applied.
// Mapper-controlled mirroring is left for the derived mapper implementation.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper::Mapper() {

	// NOTE: we're not supporting trainers anymore, that's why the code is gone

	std::fill(std::begin(nametables_), std::end(nametables_), 0);

	switch (nes::cart.mirroring()) {
	case Cart::MIR_VERTICAL:
		set_mirroring(mirror_vertical);
		break;
	case Cart::MIR_HORIZONTAL:
		set_mirroring(mirror_horizontal);
		break;
	case Cart::MIR_SINGLE_LOW:
		set_mirroring(mirror_single_low);
		break;
	case Cart::MIR_SINGLE_HIGH:
		set_mirroring(mirror_single_high);
		break;
	case Cart::MIR_4SCREEN:
		set_mirroring(mirror_4screen);
		break;
	case Cart::MIR_MAPPER:
		// Nothing, handled by Mapper
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper::create_mapper
//
// Creates the mapper implementation associated with an iNES mapper number.
//
// Parameters:
//   num - iNES mapper number.
//
// Returns:
//   Newly created mapper instance, or nullptr if the mapper is unsupported.
// -----------------------------------------------------------------------------
std::unique_ptr<Mapper> Mapper::create_mapper(int num) {

	create_func f = nullptr;

	const std::map<int, create_func> &mappers = registered_mappers_ines();
	auto it                                   = mappers.find(num);
	if (it != mappers.end() && (f = it->second)) {
		auto ret = f();
		std::cout << "[Mapper::create_mapper] mapper #" << num << " loaded, type: " << ret->name() << std::endl;
		return ret;
	} else {
		std::cout << "unsupported mapper hardware - iNES number: " << num << std::endl;
		return nullptr;
	}
}


// -----------------------------------------------------------------------------
// Mapper::read_memory
//
// Reads a byte through the mapper's CPU page table.
//
// Unmapped pages return a simulated open-bus value derived from the address.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value or simulated open-bus value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_memory(uint_least16_t address) {
	if (LIKELY(page_[address >> PageShift])) {
		return page_[address >> PageShift][address & PageMask];
	}

	// simulate open bus
	return (address >> 8) & 0xff;
}


// -----------------------------------------------------------------------------
// Mapper::write_2
//
// Default mapper write handler for the $2000-$2FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_2(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_3
//
// Default mapper write handler for the $3000-$3FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_3(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_4
//
// Default mapper write handler for the $4000-$4FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_4(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_5
//
// Default mapper write handler for the $5000-$5FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_5(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_6
//
// Default mapper write handler for the $6000-$6FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_6(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_7
//
// Default mapper write handler for the $7000-$7FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_7(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_8
//
// Default mapper write handler for the $8000-$8FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_8(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_9
//
// Default mapper write handler for the $9000-$9FFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_9(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_a
//
// Default mapper write handler for the $A000-$AFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_a(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_b
//
// Default mapper write handler for the $B000-$BFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_b(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_c
//
// Default mapper write handler for the $C000-$CFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_c(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_d
//
// Default mapper write handler for the $D000-$DFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_d(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_e
//
// Default mapper write handler for the $E000-$EFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_e(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::write_f
//
// Default mapper write handler for the $F000-$FFFF CPU address range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::write_f(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}


// -----------------------------------------------------------------------------
// Mapper::read_2
//
// Reads the $2000-$2FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_2(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_3
//
// Reads the $3000-$3FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_3(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_4
//
// Reads the $4000-$4FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_4(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_5
//
// Reads the $5000-$5FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_5(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_6
//
// Reads the $6000-$6FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_6(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_7
//
// Reads the $7000-$7FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_7(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_8
//
// Reads the $8000-$8FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_8(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_9
//
// Reads the $9000-$9FFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_9(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_a
//
// Reads the $A000-$AFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_a(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_b
//
// Reads the $B000-$BFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_b(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_c
//
// Reads the $C000-$CFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_c(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_d
//
// Reads the $D000-$DFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_d(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_e
//
// Reads the $E000-$EFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_e(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::read_f
//
// Reads the $F000-$FFFF CPU address range through the mapper page table.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Mapped byte value.
// -----------------------------------------------------------------------------
uint8_t Mapper::read_f(uint_least16_t address) {
	return read_memory(address);
}


// -----------------------------------------------------------------------------
// Mapper::write_vram
//
// Writes a byte through the mapper's PPU address space.
//
// PPU address mirroring and palette mirroring are applied before the access.
// Palette writes are directed to PPU palette RAM; other accesses are written to
// the currently mapped VRAM/CHR bank when that bank is writable.
//
// Parameters:
//   address - PPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
Mapper::write_vram(uint_least16_t address, uint8_t value)
{
	address &= 0x3fff;
	
	// mirror $2000-$2eff
	if (address >= 0x3000 && address < 0x3f00) {
		address -= 0x1000; 
	}
	
	// palette mirroring
	if (address >= 0x3f00) {
		address = 0x3f00 | (address & 0x1f);
		
		switch (address) {
			case 0x3f10:
				address = 0x3f00;
				break;
				
			case 0x3f14:
				address = 0x3f04;
				break;
				
			case 0x3f18:
				address = 0x3f08;
				break;
				
			case 0x3f1c:
				address = 0x3f0c;
				break;
		}
		
		address &= 0x1f;
		nes::ppu::set_palette_ram(address, value);
		return;
	}

	// "normal" VRAM
	VRAMBank &bank = vram_banks_[(address >> 10) & 0x0f];
	
	if (LIKELY(bank && bank.writeable())) {
		bank[address & 0x03ff] = value;
	}
}


// -----------------------------------------------------------------------------
// Mapper::read_vram
//
// Reads a byte through the mapper's PPU address space.
//
// PPU and palette mirroring are applied before the access.  Palette reads are
// returned directly from palette RAM; other reads use the currently mapped
// VRAM/CHR bank.  Unmapped banks return a simulated open-bus value.
//
// Parameters:
//   address - PPU address being read.
//
// Returns:
//   Byte value from the mapped PPU address.
// -----------------------------------------------------------------------------
uint8_t
Mapper::read_vram (uint_least16_t address)
{
	address &= 0x3fff;
	
	// mirror $2000-$2eff
	if (address >= 0x3000 && address < 0x3f00) { 
		address -= 0x1000;
	}
	
	// prioritise palette reads
	if (address >= 0x3f00) {
		address = 0x3f00 | (address & 0x1f); 
		
		switch (address) { 
			case 0x3f10:
			address = 0x3f00; 
			break;
			
			case 0x3f14:
			address = 0x3f04;
			break;
			
			case 0x3f18:
			address = 0x3f08;
			break;
			
			case 0x3f1c:
			address = 0x3f0c;
			break;
		} 
		
		return nes::ppu::palette_ram(address & 0x1f);
	}
	
	// "normal" vram 
	const VRAMBank &bank = vram_banks_[(address >> 10) & 0xf];
	
	if (LIKELY(bank)) {
		return bank[address & 0x3ff];
	}
	
	// simulate open bus 
	return (address >> 8) & 0xff;
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_67
//
// Maps an 8 KB PRG ROM bank into CPU address range $6000-$7FFF.
//
// Parameters:
//   num - 8 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_67(int num) {
	num *= (8 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_67(nes::cart.prg() + (num & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_89
//
// Maps an 8 KB PRG ROM bank into CPU address range $8000-$9FFF.
//
// Parameters:
//   num - 8 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_89(int num) {
	num *= (8 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_89(nes::cart.prg() + (num & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_ab
//
// Maps an 8 KB PRG ROM bank into CPU address range $A000-$BFFF.
//
// Parameters:
//   num - 8 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_ab(int num) {
	num *= (8 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_ab(nes::cart.prg() + (num & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_cd
//
// Maps an 8 KB PRG ROM bank into CPU address range $C000-$DFFF.
//
// Parameters:
//   num - 8 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_cd(int num) {
	num *= (8 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_cd(nes::cart.prg() + (num & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_ef
//
// Maps an 8 KB PRG ROM bank into CPU address range $E000-$FFFF.
//
// Parameters:
//   num - 8 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_ef(int num) {
	num *= (8 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_ef(nes::cart.prg() + (num & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_89ab
//
// Maps a 16 KB PRG ROM bank into CPU address range $8000-$BFFF.
//
// Parameters:
//   num - 16 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_89ab(int num) {
	num *= (16 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_89(nes::cart.prg() + ((num + 0x0000) & mask));
	swap_ab(nes::cart.prg() + ((num + 0x2000) & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_cdef
//
// Maps a 16 KB PRG ROM bank into CPU address range $C000-$FFFF.
//
// Parameters:
//   num - 16 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_cdef(int num) {
	num *= (16 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_cd(nes::cart.prg() + ((num + 0x0000) & mask));
	swap_ef(nes::cart.prg() + ((num + 0x2000) & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_prg_89abcdef
//
// Maps a 32 KB PRG ROM bank into CPU address range $8000-$FFFF.
//
// Parameters:
//   num - 32 KB PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_prg_89abcdef(int num) {
	num *= (32 * 1024);
	const uint32_t mask = nes::cart.prg_mask();
	swap_89(nes::cart.prg() + ((num + 0x0000) & mask));
	swap_ab(nes::cart.prg() + ((num + 0x2000) & mask));
	swap_cd(nes::cart.prg() + ((num + 0x4000) & mask));
	swap_ef(nes::cart.prg() + ((num + 0x6000) & mask));
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_03ff
//
// Maps a 1 KB CHR ROM bank into PPU address range $0000-$03FF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_03ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x00]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0400_07ff
//
// Maps a 1 KB CHR ROM bank into PPU address range $0400-$07FF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0400_07ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x01]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0800_0bff
//
// Maps a 1 KB CHR ROM bank into PPU address range $0800-$0BFF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0800_0bff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x02]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0c00_0fff
//
// Maps a 1 KB CHR ROM bank into PPU address range $0C00-$0FFF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0c00_0fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x03]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_13ff
//
// Maps a 1 KB CHR ROM bank into PPU address range $1000-$13FF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_13ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x04]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1400_17ff
//
// Maps a 1 KB CHR ROM bank into PPU address range $1400-$17FF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1400_17ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x05]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1800_1bff
//
// Maps a 1 KB CHR ROM bank into PPU address range $1800-$1BFF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1800_1bff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x06]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1c00_1fff
//
// Maps a 1 KB CHR ROM bank into PPU address range $1C00-$1FFF.
//
// Parameters:
//   num - 1 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1c00_1fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (1 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x07]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_07ff
//
// Maps a 2 KB CHR ROM bank into PPU address range $0000-$07FF.
//
// Parameters:
//   num - 2 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_07ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (2 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x00]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x01]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0800_0fff
//
// Maps a 2 KB CHR ROM bank into PPU address range $0800-$0FFF.
//
// Parameters:
//   num - 2 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0800_0fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (2 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x02]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x03]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_17ff
//
// Maps a 2 KB CHR ROM bank into PPU address range $1000-$17FF.
//
// Parameters:
//   num - 2 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_17ff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (2 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x04]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x05]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1800_1fff
//
// Maps a 2 KB CHR ROM bank into PPU address range $1800-$1FFF.
//
// Parameters:
//   num - 2 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1800_1fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (2 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x06]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x07]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_0fff
//
// Maps a 4 KB CHR ROM bank into PPU address range $0000-$0FFF.
//
// Parameters:
//   num - 4 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_0fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (4 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x00]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x01]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
		vram_banks_[0x02]   = {nes::cart.chr() + ((num + 0x0800) & mask), VRAMBank::Rom};
		vram_banks_[0x03]   = {nes::cart.chr() + ((num + 0x0c00) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_1fff
//
// Maps a 4 KB CHR ROM bank into PPU address range $1000-$1FFF.
//
// Parameters:
//   num - 4 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_1fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (4 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x04]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x05]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
		vram_banks_[0x06]   = {nes::cart.chr() + ((num + 0x0800) & mask), VRAMBank::Rom};
		vram_banks_[0x07]   = {nes::cart.chr() + ((num + 0x0c00) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_1fff
//
// Maps an 8 KB CHR ROM bank across the entire PPU pattern-table range.
//
// Parameters:
//   num - 8 KB CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_1fff(int num) {
	if (LIKELY(nes::cart.has_chr_rom())) {
		num *= (8 * 1024);
		const uint32_t mask = nes::cart.chr_mask();
		vram_banks_[0x00]   = {nes::cart.chr() + ((num + 0x0000) & mask), VRAMBank::Rom};
		vram_banks_[0x01]   = {nes::cart.chr() + ((num + 0x0400) & mask), VRAMBank::Rom};
		vram_banks_[0x02]   = {nes::cart.chr() + ((num + 0x0800) & mask), VRAMBank::Rom};
		vram_banks_[0x03]   = {nes::cart.chr() + ((num + 0x0c00) & mask), VRAMBank::Rom};
		vram_banks_[0x04]   = {nes::cart.chr() + ((num + 0x1000) & mask), VRAMBank::Rom};
		vram_banks_[0x05]   = {nes::cart.chr() + ((num + 0x1400) & mask), VRAMBank::Rom};
		vram_banks_[0x06]   = {nes::cart.chr() + ((num + 0x1800) & mask), VRAMBank::Rom};
		vram_banks_[0x07]   = {nes::cart.chr() + ((num + 0x1c00) & mask), VRAMBank::Rom};
	}
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_03ff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $0000-$03FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_03ff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x00] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0400_07ff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $0400-$07FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0400_07ff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x01] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0800_0bff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $0800-$0BFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0800_0bff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x02] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0c00_0fff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $0C00-$0FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0c00_0fff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x03] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_13ff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $1000-$13FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_13ff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x04] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1400_17ff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $1400-$17FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1400_17ff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x05] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1800_1bff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $1800-$1BFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1800_1bff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x06] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1c00_1fff_ram
//
// Maps a writable 1 KB CHR RAM bank into PPU address range $1C00-$1FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 1 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1c00_1fff_ram(uint8_t *p, int num) {
	num *= (1 * 1024);
	vram_banks_[0x07] = {p + num + 0x0000, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_07ff_ram
//
// Maps a writable 2 KB CHR RAM bank into PPU address range $0000-$07FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 2 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_07ff_ram(uint8_t *p, int num) {
	num *= (2 * 1024);
	vram_banks_[0x00] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x01] = {p + num + 0x0400, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0800_0fff_ram
//
// Maps a writable 2 KB CHR RAM bank into PPU address range $0800-$0FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 2 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0800_0fff_ram(uint8_t *p, int num) {
	num *= (2 * 1024);
	vram_banks_[0x02] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x03] = {p + num + 0x0400, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_17ff_ram
//
// Maps a writable 2 KB CHR RAM bank into PPU address range $1000-$17FF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 2 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_17ff_ram(uint8_t *p, int num) {
	num *= (2 * 1024);
	vram_banks_[0x04] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x05] = {p + num + 0x0400, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1800_1fff_ram
//
// Maps a writable 2 KB CHR RAM bank into PPU address range $1800-$1FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 2 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1800_1fff_ram(uint8_t *p, int num) {
	num *= (2 * 1024);
	vram_banks_[0x06] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x07] = {p + num + 0x0400, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_0fff_ram
//
// Maps a writable 4 KB CHR RAM bank into PPU address range $0000-$0FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 4 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_0fff_ram(uint8_t *p, int num) {
	num *= (4 * 1024);
	vram_banks_[0x00] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x01] = {p + num + 0x0400, VRAMBank::Ram};
	vram_banks_[0x02] = {p + num + 0x0800, VRAMBank::Ram};
	vram_banks_[0x03] = {p + num + 0x0c00, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_1000_1fff_ram
//
// Maps a writable 4 KB CHR RAM bank into PPU address range $1000-$1FFF.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 4 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_1000_1fff_ram(uint8_t *p, int num) {
	num *= (4 * 1024);
	vram_banks_[0x04] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x05] = {p + num + 0x0400, VRAMBank::Ram};
	vram_banks_[0x06] = {p + num + 0x0800, VRAMBank::Ram};
	vram_banks_[0x07] = {p + num + 0x0c00, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_chr_0000_1fff_ram
//
// Maps a writable 8 KB CHR RAM bank across the entire PPU pattern-table range.
//
// Parameters:
//   p   - Base pointer to CHR RAM.
//   num - 8 KB CHR RAM bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_chr_0000_1fff_ram(uint8_t *p, int num) {
	num *= (8 * 1024);
	vram_banks_[0x00] = {p + num + 0x0000, VRAMBank::Ram};
	vram_banks_[0x01] = {p + num + 0x0400, VRAMBank::Ram};
	vram_banks_[0x02] = {p + num + 0x0800, VRAMBank::Ram};
	vram_banks_[0x03] = {p + num + 0x0c00, VRAMBank::Ram};
	vram_banks_[0x04] = {p + num + 0x1000, VRAMBank::Ram};
	vram_banks_[0x05] = {p + num + 0x1400, VRAMBank::Ram};
	vram_banks_[0x06] = {p + num + 0x1800, VRAMBank::Ram};
	vram_banks_[0x07] = {p + num + 0x1c00, VRAMBank::Ram};
}


// -----------------------------------------------------------------------------
// Mapper::set_mirroring
//
// Configures nametable mirroring using the mapper mirroring-control byte.
//
// Each two-bit field selects the backing 1 KB nametable RAM page for one PPU
// nametable region.  The $3000-$3FFF VRAM banks are then mapped as mirrors of
// the corresponding $2000-$2FFF banks.
//
// Parameters:
//   mir - Encoded nametable mirroring control value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::set_mirroring(uint8_t mir) {
	// utilizes the concept of a mirroring control byte
	// each pair of bits represents a table to be mirrored from
	// bits 0/1 are for bank 8/c
	// bits 2/3 are for bank 9/d
	// bits 4/5 are for bank a/e
	// bits 6/7 are for bank b/f

	// so:
	// vertical:   01000100b, or $44
	// horizontal: 01010000b, or $50
	// 4 screen:   11100100b, or $e4
	// single lo:  00000000b, or $00
	// single hi:  01010101b, or $55

	vram_banks_[0x08] = {&nametables_[(mir << 10) & 0x0c00], VRAMBank::Ram};
	vram_banks_[0x09] = {&nametables_[(mir << 8) & 0x0c00], VRAMBank::Ram};
	vram_banks_[0x0a] = {&nametables_[(mir << 6) & 0x0c00], VRAMBank::Ram};
	vram_banks_[0x0b] = {&nametables_[(mir << 4) & 0x0c00], VRAMBank::Ram};

	// [$3000, $4000) mirrors [$2000, $3000)
	vram_banks_[0x0c] = vram_banks_[0x08];
	vram_banks_[0x0d] = vram_banks_[0x09];
	vram_banks_[0x0e] = vram_banks_[0x0a];
	vram_banks_[0x0f] = vram_banks_[0x0b];
}


// -----------------------------------------------------------------------------
// Mapper::cpu_sync
//
// Provides a mapper hook for CPU-cycle synchronization.
//
// The base mapper implementation performs no work.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::cpu_sync() {
	// default does nothing
}


// -----------------------------------------------------------------------------
// Mapper::ppu_end_frame
//
// Provides a mapper hook called at the end of each PPU frame.
//
// The base mapper implementation performs no work.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::ppu_end_frame() {
	// default does nothing
}


// -----------------------------------------------------------------------------
// Mapper::vram_change_hook
//
// Provides a mapper hook for changes to the current PPU VRAM address.
//
// The base mapper implementation ignores the address.
//
// Parameters:
//   vram_address - Current PPU VRAM address.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::vram_change_hook(uint_least16_t vram_address) {
	(void)vram_address;
	// default does nothing
}


// -----------------------------------------------------------------------------
// Mapper::register_mapper
//
// Registers a mapper factory function for an iNES mapper number.
//
// Parameters:
//   num        - iNES mapper number.
//   create_ptr - Factory function used to create the mapper.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Mapper::register_mapper(int num, create_func create_ptr) {
	assert(create_ptr);
	registered_mappers_ines().emplace(num, create_ptr);
}


// -----------------------------------------------------------------------------
// Mapper::open_sram
//
// Opens or creates the persistent save-RAM file associated with the current ROM.
//
// The save filename is derived from the ROM hash and placed in the emulator's
// cache directory.
//
// Parameters:
//   size - Requested save-RAM file size in bytes.
//
// Returns:
//   Memory-mapped save-RAM file.
// -----------------------------------------------------------------------------
MemoryMappedFile Mapper::open_sram(size_t size) {

	const std::filesystem::path cache_path = Settings::cacheDirectory();
	std::filesystem::create_directories(cache_path);

	char hex_buf[32];
	snprintf(hex_buf, sizeof(hex_buf), "%08x", nes::cart.rom_hash());
	std::filesystem::path save_file = cache_path / (std::string(hex_buf) + ".sav");

	return MemoryMappedFile(save_file.string(), size);
}

