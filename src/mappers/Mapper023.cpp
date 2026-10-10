
#include "Mapper023.h"


namespace {

// -----------------------------------------------------------------------------
// translate_vrc4f_address
//
// Converts the CPU A1/A0 register selection used by VRC4f into the A3/A2
// selection expected by the VRC4 base implementation.
//
// VRC4f:
//   CPU A1 A0
//
// VRC4 base:
//   CPU A3 A2
//
// The upper address bits are preserved.
//
// Parameters:
//   address - Original CPU address written by the game.
//
// Returns:
//   Equivalent canonical address understood by VRC4.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc4f_address(uint_least16_t address)
{
	const uint_least16_t selector = address & 0x0003;

	return (address & 0xfff0) | (selector << 2);
}

}


// -----------------------------------------------------------------------------
// Mapper23VRC2b::Mapper23VRC2b
//
// Initializes mapper 23 submapper 3 as a VRC2b.
//
// VRC2b does not discard the low CHR bank bit, so chr_shift is zero.
// -----------------------------------------------------------------------------
Mapper23VRC2b::Mapper23VRC2b()
	: VRC2(0)
{
}


// -----------------------------------------------------------------------------
// Mapper23VRC2b::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper23VRC2b::name() const
{
	return "VRC2b";
}


// -----------------------------------------------------------------------------
// Mapper23VRC4e::Mapper23VRC4e
//
// Initializes mapper 23 submapper 2 as a VRC4e.
// -----------------------------------------------------------------------------
Mapper23VRC4e::Mapper23VRC4e()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper23VRC4e::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper23VRC4e::name() const
{
	return "VRC4e";
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::Mapper23VRC4f
//
// Initializes mapper 23 submapper 1 as a VRC4f.
// -----------------------------------------------------------------------------
Mapper23VRC4f::Mapper23VRC4f()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper23VRC4f::name() const
{
	return "VRC4f";
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_8
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_8 (uint_least16_t address, uint8_t value)
{
	VRC4::write_8(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_9
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_9 (uint_least16_t address, uint8_t value)
{
	VRC4::write_9(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_a
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_a (uint_least16_t address, uint8_t value)
{
	VRC4::write_a(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_b
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_b (uint_least16_t address, uint8_t value)
{
	VRC4::write_b(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_c
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_c (uint_least16_t address, uint8_t value)
{
	VRC4::write_c(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_d
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_d (uint_least16_t address, uint8_t value)
{
	VRC4::write_d(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_e
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_e (uint_least16_t address, uint8_t value)
{
	VRC4::write_e(translate_vrc4f_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper23VRC4f::write_f
//
// Translates VRC4f CPU A1/A0 register selection into the canonical VRC4
// address representation and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper23VRC4f::write_f (uint_least16_t address, uint8_t value)
{
	VRC4::write_f(translate_vrc4f_address(address), value);
}

