
#ifndef _MEMORY_H_
#define _MEMORY_H_

// -----------------------------------------------------------------------------
// read_byte
//
// Reads a value from CPU memory, including mapper and I/O handling.
//
// Real CPU reads are checked against debugger memory-read watchpoints before the
// bus access is performed.
//
// Parameters:
//   address - CPU address to read.
//
// Returns:
//   Value read from CPU memory.
// -----------------------------------------------------------------------------
inline uint8_t read_byte(uint_least16_t address)
{
	const uint16_t cpuAddress = static_cast<uint16_t>(address);
	nes::cpu::debug_check_memory_read(cpuAddress);

	return nes::bus::read_memory(address);
}


// -----------------------------------------------------------------------------
// read_byte_zp
//
// Reads a value from zero-page CPU memory.
//
// Zero-page accesses use a dedicated helper in the CPU core, so watchpoint
// checking must also be performed here or zero-page reads would bypass memory
// read watchpoints.
//
// Parameters:
//   address - Zero-page CPU address to read.
//
// Returns:
//   Value read from zero-page memory.
// -----------------------------------------------------------------------------
inline uint8_t
read_byte_zp(uint8_t address)
{
	nes::cpu::debug_check_memory_read(static_cast<uint16_t>(address));

	return nes::bus::read_memory(address);
}


// -----------------------------------------------------------------------------
// write_byte
//
// Writes a value to CPU memory, including mapper and I/O handling.
//
// Real CPU writes are checked against debugger memory-write watchpoints before
// the bus write is performed.
//
// Parameters:
//   address - CPU address to write.
//   value   - Value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
inline void
write_byte(uint_least16_t address, uint8_t value)
{
	const uint16_t cpuAddress = static_cast<uint16_t>(address);
	nes::cpu::debug_check_memory_write(cpuAddress);

	nes::bus::write_memory(address, value);
}


// -----------------------------------------------------------------------------
// write_byte_zp
//
// Writes a value to zero-page CPU memory.
//
// Zero-page accesses use a dedicated helper in the CPU core, so watchpoint
// checking must also be performed here or zero-page writes would bypass memory
// write watchpoints.
//
// Parameters:
//   address - Zero-page CPU address to write.
//   value   - Value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
inline void
write_byte_zp(uint8_t address, uint8_t value)
{
	nes::cpu::debug_check_memory_write(static_cast<uint16_t>(address));

	nes::bus::write_memory(address, value);
}

#endif	// _MEMORY_H_

