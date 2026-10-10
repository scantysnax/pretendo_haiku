#include "Mapper021.h"


namespace {

// -----------------------------------------------------------------------------
// translate_vrc4a_address
//
// Converts VRC4a's CPU A2/A1 register selection into the canonical
// $x000/$x004/$x008/$x00C arrangement expected by VRC4.
//
// Mapping:
//
//   $x000 -> $x000
//   $x002 -> $x004
//   $x004 -> $x008
//   $x006 -> $x00C
//
// Parameters:
//   address - Original CPU address.
//
// Returns:
//   Equivalent canonical VRC4 address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc4a_address(uint_least16_t address)
{
	const uint_least16_t selector = address & 0x0006;

	return (address & 0xfff0) | (selector << 1);
}


// -----------------------------------------------------------------------------
// translate_vrc4c_address
//
// Converts VRC4c's CPU A7/A6 register selection into the canonical
// $x000/$x004/$x008/$x00C arrangement expected by VRC4.
//
// Mapping:
//
//   $x000 -> $x000
//   $x040 -> $x004
//   $x080 -> $x008
//   $x0C0 -> $x00C
//
// Parameters:
//   address - Original CPU address.
//
// Returns:
//   Equivalent canonical VRC4 address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc4c_address(uint_least16_t address)
{
	const uint_least16_t selector = address & 0x00c0;

	return (address & 0xff00) | (selector >> 4);
}

}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::Mapper21VRC4a
//
// Initializes mapper 21 as VRC4a.
// -----------------------------------------------------------------------------
Mapper21VRC4a::Mapper21VRC4a()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper21VRC4a::name() const
{
	return "VRC4a";
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_8
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_8 (uint_least16_t address, uint8_t value)
{
	VRC4::write_8(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_9
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_9 (uint_least16_t address, uint8_t value)
{
	VRC4::write_9(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_a
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_a (uint_least16_t address, uint8_t value)
{
	VRC4::write_a(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_b
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_b (uint_least16_t address, uint8_t value)
{
	VRC4::write_b(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_c
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_c (uint_least16_t address, uint8_t value)
{
	VRC4::write_c(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_d
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_d (uint_least16_t address, uint8_t value)
{
	VRC4::write_d(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_e
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_e (uint_least16_t address, uint8_t value)
{
	VRC4::write_e(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4a::write_f
//
// Translates the VRC4a address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4a::write_f (uint_least16_t address, uint8_t value)
{
	VRC4::write_f(translate_vrc4a_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::Mapper21VRC4c
//
// Initializes mapper 21 as VRC4c.
// -----------------------------------------------------------------------------
Mapper21VRC4c::Mapper21VRC4c()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper21VRC4c::name() const
{
	return "VRC4c";
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_8
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_8 (uint_least16_t address, uint8_t value)
{
	VRC4::write_8(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_9
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_9 (uint_least16_t address, uint8_t value)
{
	VRC4::write_9(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_a
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_a (uint_least16_t address, uint8_t value)
{
	VRC4::write_a(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_b
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_b (uint_least16_t address, uint8_t value)
{
	VRC4::write_b(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_c
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_c (uint_least16_t address, uint8_t value)
{
	VRC4::write_c(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_d
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_d (uint_least16_t address, uint8_t value)
{
	VRC4::write_d(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_e
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_e (uint_least16_t address, uint8_t value)
{
	VRC4::write_e(translate_vrc4c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper21VRC4c::write_f
//
// Translates the VRC4c address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper21VRC4c::write_f (uint_least16_t address, uint8_t value)
{
	VRC4::write_f(translate_vrc4c_address(address), value);
}

