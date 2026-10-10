
#include "Mapper025.h"


namespace {

// -----------------------------------------------------------------------------
// translate_vrc2c_address
//
// Converts the reversed CPU A0/A1 register selection used by VRC2c into the
// canonical A1/A0 ordering expected by the VRC2 base implementation.
//
// Mapping:
//
//   $x000 -> $x000
//   $x001 -> $x002
//   $x002 -> $x001
//   $x003 -> $x003
//
// Parameters:
//   address - Original CPU address.
//
// Returns:
//   Equivalent canonical VRC2 address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc2c_address (uint_least16_t address)
{
	const uint_least16_t selector = address & 0x0003;
	const uint_least16_t translated = ((selector & 0x0001) << 1) | ((selector & 0x0002) >> 1);

	return (address & 0xfffc) | translated;
}


// -----------------------------------------------------------------------------
// translate_vrc4b_address
//
// Converts VRC4b's CPU A0/A1 register selection into the canonical
// $x000/$x004/$x008/$x00C arrangement expected by VRC4.
//
// Mapping:
//
//   $x000 -> $x000
//   $x001 -> $x008
//   $x002 -> $x004
//   $x003 -> $x00C
//
// Parameters:
//   address - Original CPU address.
//
// Returns:
//   Equivalent canonical VRC4 address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc4b_address (uint_least16_t address)
{
	const uint_least16_t selector = address & 0x0003;
	const uint_least16_t translated = ((selector & 0x0001) << 3) | ((selector & 0x0002) << 1);

	return (address & 0xfff0) | translated;
}


// -----------------------------------------------------------------------------
// translate_vrc4d_address
//
// Converts VRC4d's reversed CPU A2/A3 register selection into the canonical
// $x000/$x004/$x008/$x00C arrangement expected by VRC4.
//
// Mapping:
//
//   $x000 -> $x000
//   $x004 -> $x008
//   $x008 -> $x004
//   $x00C -> $x00C
//
// Parameters:
//   address - Original CPU address.
//
// Returns:
//   Equivalent canonical VRC4 address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc4d_address (uint_least16_t address)
{
	const uint_least16_t selector = (address >> 2) & 0x0003;
	const uint_least16_t translated = ((selector & 0x0001) << 3) | ((selector & 0x0002) << 1);

	return (address & 0xfff0) | translated;
}

}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::Mapper25VRC2c
//
// Initializes mapper 25 as VRC2c.
//
// VRC2c uses the full CHR bank value, so no CHR shift is applied.
// -----------------------------------------------------------------------------
Mapper25VRC2c::Mapper25VRC2c()
	: VRC2(0)
{
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper25VRC2c::name() const
{
	return "VRC2c";
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::read_6
//
// Translates VRC2c's reversed A0/A1 selection and forwards the read to VRC2.
// -----------------------------------------------------------------------------
uint8_t
Mapper25VRC2c::read_6 (uint_least16_t address)
{
	return VRC2::read_6(translate_vrc2c_address(address));
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_6
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_6 (uint_least16_t address, uint8_t value)
{
	VRC2::write_6(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_8
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_8 (uint_least16_t address, uint8_t value)
{
	VRC2::write_8(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_9
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_9 (uint_least16_t address, uint8_t value)
{
	VRC2::write_9(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_a
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_a (uint_least16_t address, uint8_t value)
{
	VRC2::write_a(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_b
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_b (uint_least16_t address, uint8_t value)
{
	VRC2::write_b(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_c
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_c (uint_least16_t address, uint8_t value)
{
	VRC2::write_c(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_d
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_d (uint_least16_t address, uint8_t value)
{
	VRC2::write_d(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC2c::write_e
//
// Translates VRC2c's reversed A0/A1 selection and forwards the write to VRC2.
// -----------------------------------------------------------------------------
void
Mapper25VRC2c::write_e (uint_least16_t address, uint8_t value)
{
	VRC2::write_e(translate_vrc2c_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::Mapper25VRC4b
//
// Initializes mapper 25 as VRC4b.
// -----------------------------------------------------------------------------
Mapper25VRC4b::Mapper25VRC4b()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper25VRC4b::name() const
{
	return "VRC4b";
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_8
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_8 (uint_least16_t address, uint8_t value)
{
	VRC4::write_8(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_9
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_9 (uint_least16_t address, uint8_t value)
{
	VRC4::write_9(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_a
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_a (uint_least16_t address, uint8_t value)
{
	VRC4::write_a(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_b
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_b (uint_least16_t address, uint8_t value)
{
	VRC4::write_b(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_c
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_c (uint_least16_t address, uint8_t value)
{
	VRC4::write_c(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_d
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_d (uint_least16_t address, uint8_t value)
{
	VRC4::write_d(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_e
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_e (uint_least16_t address, uint8_t value)
{
	VRC4::write_e(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4b::write_f
//
// Translates the VRC4b address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4b::write_f (uint_least16_t address, uint8_t value)
{
	VRC4::write_f(translate_vrc4b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::Mapper25VRC4d
//
// Initializes mapper 25 as VRC4d.
// -----------------------------------------------------------------------------
Mapper25VRC4d::Mapper25VRC4d()
	: VRC4()
{
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::name
//
// Returns the mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper25VRC4d::name() const
{
	return "VRC4d";
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_8
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_8 (uint_least16_t address, uint8_t value)
{
	VRC4::write_8(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_9
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_9 (uint_least16_t address, uint8_t value)
{
	VRC4::write_9(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_a
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_a (uint_least16_t address, uint8_t value)
{
	VRC4::write_a(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_b
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_b (uint_least16_t address, uint8_t value)
{
	VRC4::write_b(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_c
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_c (uint_least16_t address, uint8_t value)
{
	VRC4::write_c(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_d
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_d (uint_least16_t address, uint8_t value)
{
	VRC4::write_d(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_e
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_e (uint_least16_t address, uint8_t value)
{
	VRC4::write_e(translate_vrc4d_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper25VRC4d::write_f
//
// Translates the VRC4d address and forwards the write to VRC4.
// -----------------------------------------------------------------------------
void
Mapper25VRC4d::write_f (uint_least16_t address, uint8_t value)
{
	VRC4::write_f(translate_vrc4d_address(address), value);
}

