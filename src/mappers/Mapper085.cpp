
#include "Mapper085.h"


namespace {


// -----------------------------------------------------------------------------
// translate_vrc7b_address
//
// Converts VRC7b's A3-selected mapper register addresses into the canonical
// A4-selected layout used internally by VRC7.
//
// Mapping:
//
//   $x000 -> $x000
//   $x008 -> $x010
//
// The translation is intended only for mapper-control registers.  VRC7 audio
// ports at $9010/$9030 use their normal addresses and are not passed through
// this function.
//
// Parameters:
//   address - Original VRC7b CPU address.
//
// Returns:
//   Equivalent canonical VRC7 mapper-register address.
// -----------------------------------------------------------------------------
uint_least16_t
translate_vrc7b_address (uint_least16_t address)
{
	if (address & 0x0008) {
		return (address & ~static_cast<uint_least16_t>(0x0008)) | 0x0010;
	}

	return address;
}


} // end anonymous namespace


// -----------------------------------------------------------------------------
// Mapper85VRC7a::name
//
// Returns the mapper hardware variant name.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper85VRC7a::name() const
{
	return "Konami VRC7a";
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::name
//
// Returns the mapper hardware variant name.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper hardware variant name.
// -----------------------------------------------------------------------------
std::string
Mapper85VRC7b::name() const
{
	return "Konami VRC7b";
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_8
//
// Translates VRC7b's A3-selected PRG register address into the canonical VRC7
// layout and forwards the write to the base implementation.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_8 (uint_least16_t address, uint8_t value)
{
	VRC7::write_8(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_a
//
// Translates VRC7b CHR register addressing into the canonical VRC7 layout.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_a (uint_least16_t address, uint8_t value)
{
	VRC7::write_a(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_b
//
// Translates VRC7b CHR register addressing into the canonical VRC7 layout.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_b (uint_least16_t address, uint8_t value)
{
	VRC7::write_b(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_c
//
// Translates VRC7b CHR register addressing into the canonical VRC7 layout.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_c (uint_least16_t address, uint8_t value)
{
	VRC7::write_c(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_d
//
// Translates VRC7b CHR register addressing into the canonical VRC7 layout.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_d (uint_least16_t address, uint8_t value)
{
	VRC7::write_d(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_e
//
// Translates the VRC7b IRQ-latch register address into the canonical VRC7
// layout.  The $E000 mapper-control register is unchanged.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_e (uint_least16_t address, uint8_t value)
{
	VRC7::write_e(translate_vrc7b_address(address), value);
}


// -----------------------------------------------------------------------------
// Mapper85VRC7b::write_f
//
// Translates the VRC7b IRQ-acknowledge register address into the canonical VRC7
// layout.  The $F000 IRQ-control register is unchanged.
//
// Parameters:
//   address - Original CPU address.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper85VRC7b::write_f (uint_least16_t address, uint8_t value)
{
	VRC7::write_f(translate_vrc7b_address(address), value);
}

