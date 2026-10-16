#include "Mapper027.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(27)


// -----------------------------------------------------------------------------
// Mapper27::name
//
// Returns the human-readable mapper name.
//
// Mapper 27 uses the Konami VRC4a variant.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper name.
// -----------------------------------------------------------------------------
std::string
Mapper27::name() const
{
	return "VRC4a";
}


// -----------------------------------------------------------------------------
// Mapper27::write_8
//
// Translates mapper 27 register addressing in the $8000-$8FFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_8(0x8000, value);
		break;

	case 0x0001:
		VRC4::write_8(0x8002, value);
		break;

	case 0x0002:
		VRC4::write_8(0x8004, value);
		break;

	case 0x0003:
		VRC4::write_8(0x8006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_9
//
// Translates mapper 27 register addressing in the $9000-$9FFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_9(0x9000, value);
		break;

	case 0x0001:
		VRC4::write_9(0x9002, value);
		break;

	case 0x0002:
		VRC4::write_9(0x9004, value);
		break;

	case 0x0003:
		VRC4::write_9(0x9006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_a
//
// Translates mapper 27 register addressing in the $A000-$AFFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_a(0xa000, value);
		break;

	case 0x0001:
		VRC4::write_a(0xa002, value);
		break;

	case 0x0002:
		VRC4::write_a(0xa004, value);
		break;

	case 0x0003:
		VRC4::write_a(0xa006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_b
//
// Translates mapper 27 register addressing in the $B000-$BFFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_b(0xb000, value);
		break;

	case 0x0001:
		VRC4::write_b(0xb002, value);
		break;

	case 0x0002:
		VRC4::write_b(0xb004, value);
		break;

	case 0x0003:
		VRC4::write_b(0xb006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_c
//
// Translates mapper 27 register addressing in the $C000-$CFFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_c(0xc000, value);
		break;

	case 0x0001:
		VRC4::write_c(0xc002, value);
		break;

	case 0x0002:
		VRC4::write_c(0xc004, value);
		break;

	case 0x0003:
		VRC4::write_c(0xc006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_d
//
// Translates mapper 27 register addressing in the $D000-$DFFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_d(0xd000, value);
		break;

	case 0x0001:
		VRC4::write_d(0xd002, value);
		break;

	case 0x0002:
		VRC4::write_d(0xd004, value);
		break;

	case 0x0003:
		VRC4::write_d(0xd006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_e
//
// Translates mapper 27 register addressing in the $E000-$EFFF range to the
// common VRC4 register layout.
//
// Mapper 27 maps its two low register-select bits onto VRC4 register offsets
// $0, $2, $4, and $6 before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_e(0xe000, value);
		break;

	case 0x0001:
		VRC4::write_e(0xe002, value);
		break;

	case 0x0002:
		VRC4::write_e(0xe004, value);
		break;

	case 0x0003:
		VRC4::write_e(0xe006, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper27::write_f
//
// Translates mapper 27 register addressing in the $F000-$FFFF range to the
// common VRC4 register layout.
//
// This range contains the VRC4 IRQ registers. Mapper 27 maps its two low
// register-select bits onto VRC4 register offsets $0, $2, $4, and $6 before
// forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper27::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC4::write_f(0xf000, value);
		break;

	case 0x0001:
		VRC4::write_f(0xf002, value);
		break;

	case 0x0002:
		VRC4::write_f(0xf004, value);
		break;

	case 0x0003:
		VRC4::write_f(0xf006, value);
		break;
	}
}

