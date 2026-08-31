
#include "Bus.h"
#include "Apu.h"
#include "Cart.h"
#include "Cpu.h"
#include "Input.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"

//#include <algorithm>
#include <cstdlib>

namespace nes::bus {
namespace {

uint8_t ram_[0x800];

// -----------------------------------------------------------------------------
// write_0
//
// Writes a byte to internal CPU RAM in the $0000-$0FFF address range.
//
// The NES contains 2 KB of internal RAM mirrored throughout this range, so the
// supplied address is masked to the physical $0000-$07FF RAM area.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to store.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_0(uint_least16_t address, uint8_t value) {
	ram_[address & 0x7ff] = value;
}


// -----------------------------------------------------------------------------
// write_1
//
// Writes a byte to internal CPU RAM in the $1000-$1FFF address range.
//
// This region mirrors the same 2 KB internal RAM used by $0000-$07FF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to store.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_1(uint_least16_t address, uint8_t value) {
	ram_[address & 0x7ff] = value;
}


// -----------------------------------------------------------------------------
// write_2
//
// Writes to the $2000-$2FFF CPU address range.
//
// The write is first exposed to the active cartridge mapper, then routed to the
// appropriate mirrored PPU register according to the low three address bits.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_2(uint_least16_t address, uint8_t value) {

	nes::cart.mapper()->write_2(address, value);
	switch (address & 0x07) {
	case 0x00:
		nes::ppu::write2000(value);
		break;
	case 0x01:
		nes::ppu::write2001(value);
		break;
	case 0x02:
		nes::ppu::write2002(value);
		break;
	case 0x03:
		nes::ppu::write2003(value);
		break;
	case 0x04:
		nes::ppu::write2004(value);
		break;
	case 0x05:
		nes::ppu::write2005(value);
		break;
	case 0x06:
		nes::ppu::write2006(value);
		break;
	case 0x07:
		nes::ppu::write2007(value);
		break;
	default:
		abort();
	}
}


// -----------------------------------------------------------------------------
// write_3
//
// Writes to the $3000-$3FFF CPU address range.
//
// This region mirrors the PPU register range.  The write is first exposed to the
// cartridge mapper, then routed to the appropriate PPU register using the low
// three address bits.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_3(uint_least16_t address, uint8_t value) {

	nes::cart.mapper()->write_3(address, value);
	switch (address & 0x07) {
	case 0x00:
		nes::ppu::write2000(value);
		break;
	case 0x01:
		nes::ppu::write2001(value);
		break;
	case 0x02:
		nes::ppu::write2002(value);
		break;
	case 0x03:
		nes::ppu::write2003(value);
		break;
	case 0x04:
		nes::ppu::write2004(value);
		break;
	case 0x05:
		nes::ppu::write2005(value);
		break;
	case 0x06:
		nes::ppu::write2006(value);
		break;
	case 0x07:
		nes::ppu::write2007(value);
		break;
	default:
		abort();
	}
}


// -----------------------------------------------------------------------------
// write_4
//
// Writes to the $4000-$4FFF CPU address range.
//
// The write is exposed to the cartridge mapper and then dispatched to the APU,
// PPU DMA, controller, or frame-counter register when the address corresponds to
// a standard NES I/O register.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_4(uint_least16_t address, uint8_t value) {

	nes::cart.mapper()->write_4(address, value);
	switch (address) {
	case 0x4000:
		nes::apu::write4000(value);
		break;
	case 0x4001:
		nes::apu::write4001(value);
		break;
	case 0x4002:
		nes::apu::write4002(value);
		break;
	case 0x4003:
		nes::apu::write4003(value);
		break;
	case 0x4004:
		nes::apu::write4004(value);
		break;
	case 0x4005:
		nes::apu::write4005(value);
		break;
	case 0x4006:
		nes::apu::write4006(value);
		break;
	case 0x4007:
		nes::apu::write4007(value);
		break;
	case 0x4008:
		nes::apu::write4008(value);
		break;
	case 0x400a:
		nes::apu::write400A(value);
		break;
	case 0x400b:
		nes::apu::write400B(value);
		break;
	case 0x400c:
		nes::apu::write400C(value);
		break;
	case 0x400e:
		nes::apu::write400E(value);
		break;
	case 0x400f:
		nes::apu::write400F(value);
		break;
	case 0x4010:
		nes::apu::write4010(value);
		break;
	case 0x4011:
		nes::apu::write4011(value);
		break;
	case 0x4012:
		nes::apu::write4012(value);
		break;
	case 0x4013:
		nes::apu::write4013(value);
		break;
	case 0x4014:
		nes::ppu::write4014(value);
		break;
	case 0x4015:
		nes::apu::write4015(value);
		break;
	case 0x4016:
		nes::input::write4016(value);
		break;
	case 0x4017:
		nes::apu::write4017(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// write_5
//
// Forwards writes in the $5000-$5FFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_5(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_5(address, value);
}


// -----------------------------------------------------------------------------
// write_6
//
// Forwards writes in the $6000-$6FFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_6(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_6(address, value);
}


// -----------------------------------------------------------------------------
// write_7
//
// Forwards writes in the $7000-$7FFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_7(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_7(address, value);
}


// -----------------------------------------------------------------------------
// write_8
//
// Forwards writes in the $8000-$8FFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_8(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_8(address, value);
}


// -----------------------------------------------------------------------------
// write_9
//
// Forwards writes in the $9000-$9FFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_9(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_9(address, value);
}


// -----------------------------------------------------------------------------
// write_a
//
// Forwards writes in the $A000-$AFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_a(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_a(address, value);
}


// -----------------------------------------------------------------------------
// write_b
//
// Forwards writes in the $B000-$BFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_b(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_b(address, value);
}


// -----------------------------------------------------------------------------
// write_c
//
// Forwards writes in the $C000-$CFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_c(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_c(address, value);
}


// -----------------------------------------------------------------------------
// write_d
//
// Forwards writes in the $D000-$DFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_d(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_d(address, value);
}


// -----------------------------------------------------------------------------
// write_e
//
// Forwards writes in the $E000-$EFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_e(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_e(address, value);
}


// -----------------------------------------------------------------------------
// write_f
//
// Forwards writes in the $F000-$FFFF CPU address range to the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_f(uint_least16_t address, uint8_t value) {
	nes::cart.mapper()->write_f(address, value);
}


// -----------------------------------------------------------------------------
// read_0
//
// Reads a byte from internal CPU RAM in the $0000-$0FFF address range.
//
// The address is mirrored into the physical 2 KB internal RAM area.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Byte stored at the corresponding internal RAM location.
// -----------------------------------------------------------------------------
uint8_t read_0(uint_least16_t address) {
	return ram_[address & 0x7ff];
}


// -----------------------------------------------------------------------------
// read_1
//
// Reads a byte from internal CPU RAM in the $1000-$1FFF address range.
//
// This region mirrors the same physical 2 KB internal RAM used by $0000-$07FF.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Byte stored at the corresponding internal RAM location.
// -----------------------------------------------------------------------------
uint8_t read_1(uint_least16_t address) {
	return ram_[address & 0x7ff];
}


// -----------------------------------------------------------------------------
// read_2
//
// Reads from the mirrored PPU-register range at $2000-$2FFF.
//
// Registers $2002, $2004, and $2007 use their dedicated read handlers.  Reads
// from the remaining mirrored register addresses use the generic PPU register
// read behavior.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value returned by the selected PPU register.
// -----------------------------------------------------------------------------
uint8_t read_2(uint_least16_t address) {

	switch (address & 0x07) {
	case 0x02:
		return nes::ppu::read2002();
	case 0x04:
		return nes::ppu::read2004();
	case 0x07:
		return nes::ppu::read2007();
	default:
		return nes::ppu::read200x();
	}
}


// -----------------------------------------------------------------------------
// read_3
//
// Reads from the mirrored PPU-register range at $3000-$3FFF.
//
// Registers $2002, $2004, and $2007 use their dedicated read handlers.  Reads
// from the remaining mirrored register addresses use the generic PPU register
// read behavior.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value returned by the selected PPU register.
// -----------------------------------------------------------------------------
uint8_t read_3(uint_least16_t address) {
	switch (address & 0x07) {
	case 0x02:
		return nes::ppu::read2002();
	case 0x04:
		return nes::ppu::read2004();
	case 0x07:
		return nes::ppu::read2007();
	default:
		return nes::ppu::read200x();
	}
}


// -----------------------------------------------------------------------------
// read_4
//
// Reads from the $4000-$4FFF CPU address range.
//
// Standard APU and controller status registers are handled directly.  Other
// addresses are delegated to the active cartridge mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value returned by the selected I/O device or cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_4(uint_least16_t address) {

	switch (address) {
	case 0x4015:
		return nes::apu::read4015();
	case 0x4016:
		return nes::input::read4016();
	case 0x4017:
		return nes::input::read4017();
	default:
		return nes::cart.mapper()->read_4(address);
	}
}


// -----------------------------------------------------------------------------
// read_5
//
// Reads from the $5000-$5FFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_5(uint_least16_t address) {
	return nes::cart.mapper()->read_5(address);
}


// -----------------------------------------------------------------------------
// read_6
//
// Reads from the $6000-$6FFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_6(uint_least16_t address) {
	return nes::cart.mapper()->read_6(address);
}


// -----------------------------------------------------------------------------
// read_7
//
// Reads from the $7000-$7FFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_7(uint_least16_t address) {
	return nes::cart.mapper()->read_7(address);
}


// -----------------------------------------------------------------------------
// read_8
//
// Reads from the $8000-$8FFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_8(uint_least16_t address) {
	return nes::cart.mapper()->read_8(address);
}


// -----------------------------------------------------------------------------
// read_9
//
// Reads from the $9000-$9FFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_9(uint_least16_t address) {
	return nes::cart.mapper()->read_9(address);
}


// -----------------------------------------------------------------------------
// read_a
//
// Reads from the $A000-$AFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_a(uint_least16_t address) {
	return nes::cart.mapper()->read_a(address);
}


// -----------------------------------------------------------------------------
// read_b
//
// Reads from the $B000-$BFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_b(uint_least16_t address) {
	return nes::cart.mapper()->read_b(address);
}


// -----------------------------------------------------------------------------
// read_c
//
// Reads from the $C000-$CFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_c(uint_least16_t address) {
	return nes::cart.mapper()->read_c(address);
}


// -----------------------------------------------------------------------------
// read_d
//
// Reads from the $D000-$DFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_d(uint_least16_t address) {
	return nes::cart.mapper()->read_d(address);
}


// -----------------------------------------------------------------------------
// read_e
//
// Reads from the $E000-$EFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_e(uint_least16_t address) {
	return nes::cart.mapper()->read_e(address);
}


// -----------------------------------------------------------------------------
// read_f
//
// Reads from the $F000-$FFFF CPU address range through the active cartridge
// mapper.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value supplied by the cartridge mapper.
// -----------------------------------------------------------------------------
uint8_t read_f(uint_least16_t address) {
	return nes::cart.mapper()->read_f(address);
}


}


// -----------------------------------------------------------------------------
// write_memory
//
// Dispatches a CPU memory write according to the upper four address bits.
//
// Each 4 KB CPU address region is routed to its corresponding write handler,
// which then performs RAM, PPU, APU, controller, DMA, or mapper-specific work.
//
// Parameters:
//   address - CPU address being written.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write_memory(uint_least16_t address, uint8_t value) {
	switch ((address >> 12) & 0xf) {
	case 0x0000:
		write_0(address, value);
		break;
	case 0x0001:
		write_1(address, value);
		break;
	case 0x0002:
		write_2(address, value);
		break;
	case 0x0003:
		write_3(address, value);
		break;
	case 0x0004:
		write_4(address, value);
		break;
	case 0x0005:
		write_5(address, value);
		break;
	case 0x0006:
		write_6(address, value);
		break;
	case 0x0007:
		write_7(address, value);
		break;
	case 0x0008:
		write_8(address, value);
		break;
	case 0x0009:
		write_9(address, value);
		break;
	case 0x000a:
		write_a(address, value);
		break;
	case 0x000b:
		write_b(address, value);
		break;
	case 0x000c:
		write_c(address, value);
		break;
	case 0x000d:
		write_d(address, value);
		break;
	case 0x000e:
		write_e(address, value);
		break;
	case 0x000f:
		write_f(address, value);
		break;
	default:
		abort();
	}
}


// -----------------------------------------------------------------------------
// read_memory
//
// Dispatches a CPU memory read according to the upper four address bits.
//
// Each 4 KB CPU address region is routed to its corresponding read handler,
// which performs the appropriate RAM, PPU, APU, controller, or cartridge-mapper
// access.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Byte value read from the selected CPU memory region.
// -----------------------------------------------------------------------------
uint8_t read_memory(uint_least16_t address) {
	switch ((address >> 12) & 0xf) {
	case 0x0000:
		return read_0(address);
	case 0x0001:
		return read_1(address);
	case 0x0002:
		return read_2(address);
	case 0x0003:
		return read_3(address);
	case 0x0004:
		return read_4(address);
	case 0x0005:
		return read_5(address);
	case 0x0006:
		return read_6(address);
	case 0x0007:
		return read_7(address);
	case 0x0008:
		return read_8(address);
	case 0x0009:
		return read_9(address);
	case 0x000a:
		return read_a(address);
	case 0x000b:
		return read_b(address);
	case 0x000c:
		return read_c(address);
	case 0x000d:
		return read_d(address);
	case 0x000e:
		return read_e(address);
	case 0x000f:
		return read_f(address);
	default:
		abort();
	}
}


// -----------------------------------------------------------------------------
// debug_read_memory
//
// Reads CPU memory for debugger inspection without triggering side effects from
// hardware registers.
//
// Internal RAM may be read normally.  PPU registers and standard APU/controller
// I/O registers return zero rather than invoking side-effectful hardware reads.
// Cartridge-backed regions are read through the mapper when one is available.
//
// Parameters:
//   address - CPU address to inspect.
//
// Returns:
//   Debug-safe byte value for the requested address.
// -----------------------------------------------------------------------------
uint8_t
debug_read_memory(uint_least16_t address)
{
	switch ((address >> 12) & 0xf) {
		case 0x0:
			return read_0(address);

		case 0x1:
			return read_1(address);

		case 0x2:
		case 0x3:
			// PPU register range.  Do not perform side-effectful PPU reads.
			return 0x00;

		case 0x4:
			// APU / IO range.  Do not perform side-effectful APU, controller,
			// or DMA reads.  Cartridge expansion space starts at $4020.
			if (address < 0x4020) {
				return 0x00;
			}

			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_4(address);

		case 0x5:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_5(address);

		case 0x6:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_6(address);

		case 0x7:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_7(address);

		case 0x8:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_8(address);

		case 0x9:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_9(address);

		case 0xa:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_a(address);

		case 0xb:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_b(address);

		case 0xc:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_c(address);

		case 0xd:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_d(address);

		case 0xe:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_e(address);

		case 0xf:
			if (!nes::cart.mapper()) {
				return 0x00;
			}

			return read_f(address);

		default:
			return 0x00;
	}
}


// -----------------------------------------------------------------------------
// trash_ram
//
// Clears the NES internal 2 KB CPU RAM.
//
// A deterministic zero-filled pattern is used instead of random power-on data.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void trash_ram() {
	// NOTE(eteran): this could be "random" bytes, but all zeros
	// is just an good as any other patterns
	std::fill_n(ram_, 0x800, 0);
}

}
