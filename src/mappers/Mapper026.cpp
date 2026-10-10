#include "Mapper026.h"
#include "VRC6.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(26)


// -----------------------------------------------------------------------------
// Mapper26::name
//
// Returns the human-readable name of mapper 26.
//
// Mapper 26 uses the Konami VRC6b variant.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper name.
// -----------------------------------------------------------------------------
std::string
Mapper26::name() const
{
	return "VRC6b";
}


// -----------------------------------------------------------------------------
// Mapper26::write_8
//
// Translates mapper 26 register addressing in the $8000-$8FFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1 relative to the VRC6a register layout,
// so register offsets 1 and 2 are exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_8  (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_8(0x8000, value);
		break;

	case 0x0001:
		VRC6::write_8(0x8002, value);
		break;

	case 0x0002:
		VRC6::write_8(0x8001, value);
		break;

	case 0x0003:
		VRC6::write_8(0x8003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_9
//
// Translates mapper 26 register addressing in the $9000-$9FFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1, so register offsets 1 and 2 are exchanged
// before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_9(0x9000, value);
		break;

	case 0x0001:
		VRC6::write_9(0x9002, value);
		break;

	case 0x0002:
		VRC6::write_9(0x9001, value);
		break;

	case 0x0003:
		VRC6::write_9(0x9003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_a
//
// Translates mapper 26 register addressing in the $A000-$AFFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1, so register offsets 1 and 2 are exchanged
// before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_a(0xa000, value);
		break;

	case 0x0001:
		VRC6::write_a(0xa002, value);
		break;

	case 0x0002:
		VRC6::write_a(0xa001, value);
		break;

	case 0x0003:
		VRC6::write_a(0xa003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_b
//
// Translates mapper 26 register addressing in the $B000-$BFFF range to the
// common VRC6 register layout.
//
// This range contains the VRC6 sawtooth registers and $B003 mapper control.
// VRC6b swaps address lines A0 and A1 before the write reaches the common VRC6
// implementation.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_b(0xb000, value);
		break;

	case 0x0001:
		VRC6::write_b(0xb002, value);
		break;

	case 0x0002:
		VRC6::write_b(0xb001, value);
		break;

	case 0x0003:
		VRC6::write_b(0xb003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_c
//
// Translates mapper 26 register addressing in the $C000-$CFFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1, so register offsets 1 and 2 are exchanged
// before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_c(0xc000, value);
		break;

	case 0x0001:
		VRC6::write_c(0xc002, value);
		break;

	case 0x0002:
		VRC6::write_c(0xc001, value);
		break;

	case 0x0003:
		VRC6::write_c(0xc003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_d
//
// Translates mapper 26 register addressing in the $D000-$DFFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1, so register offsets 1 and 2 are exchanged
// before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_d(0xd000, value);
		break;

	case 0x0001:
		VRC6::write_d(0xd002, value);
		break;

	case 0x0002:
		VRC6::write_d(0xd001, value);
		break;

	case 0x0003:
		VRC6::write_d(0xd003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_e
//
// Translates mapper 26 register addressing in the $E000-$EFFF range to the
// common VRC6 register layout.
//
// VRC6b swaps address lines A0 and A1, so register offsets 1 and 2 are exchanged
// before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_e(0xe000, value);
		break;

	case 0x0001:
		VRC6::write_e(0xe002, value);
		break;

	case 0x0002:
		VRC6::write_e(0xe001, value);
		break;

	case 0x0003:
		VRC6::write_e(0xe003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper26::write_f
//
// Translates mapper 26 register addressing in the $F000-$FFFF range to the
// common VRC6 register layout.
//
// This range contains the VRC6 IRQ registers. VRC6b swaps address lines A0 and
// A1, so register offsets 1 and 2 are exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper26::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC6::write_f(0xf000, value);
		break;

	case 0x0001:
		VRC6::write_f(0xf002, value);
		break;

	case 0x0002:
		VRC6::write_f(0xf001, value);
		break;

	case 0x0003:
		VRC6::write_f(0xf003, value);
		break;
	}
}
