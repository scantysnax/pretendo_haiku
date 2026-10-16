#include "Mapper022.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(22)


// -----------------------------------------------------------------------------
// Mapper22::Mapper22
//
// Initializes mapper 22 as a Konami VRC2a implementation.
//
// VRC2a uses the VRC2 core with a CHR shift value of 1.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Mapper22::Mapper22()
	: VRC2(1)
{
}


// -----------------------------------------------------------------------------
// Mapper22::name
//
// Returns the human-readable mapper name.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper name.
// -----------------------------------------------------------------------------
std::string
Mapper22::name() const
{
	return "VRC2a";
}


// -----------------------------------------------------------------------------
// Mapper22::read_6
//
// Translates mapper 22 addressing in the $6000-$6FFF range to the common
// VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the read.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Value returned by the common VRC2 implementation.
// -----------------------------------------------------------------------------
uint8_t
Mapper22::read_6(uint_least16_t address)
{
	switch (address & 0x0003) {
	case 0x0000:
		return VRC2::read_6(0x6000);

	case 0x0001:
		return VRC2::read_6(0x6002);

	case 0x0002:
		return VRC2::read_6(0x6001);

	case 0x0003:
		return VRC2::read_6(0x6003);
	}

	return 0xff;
}


// -----------------------------------------------------------------------------
// Mapper22::write_6
//
// Translates mapper 22 addressing in the $6000-$6FFF range to the common
// VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_6(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_6(0x6000, value);
		break;

	case 0x0001:
		VRC2::write_6(0x6002, value);
		break;

	case 0x0002:
		VRC2::write_6(0x6001, value);
		break;

	case 0x0003:
		VRC2::write_6(0x6003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_8
//
// Translates mapper 22 register addressing in the $8000-$8FFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_8(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_8(0x8000, value);
		break;

	case 0x0001:
		VRC2::write_8(0x8002, value);
		break;

	case 0x0002:
		VRC2::write_8(0x8001, value);
		break;

	case 0x0003:
		VRC2::write_8(0x8003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_9
//
// Translates mapper 22 register addressing in the $9000-$9FFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_9(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_9(0x9000, value);
		break;

	case 0x0001:
		VRC2::write_9(0x9002, value);
		break;

	case 0x0002:
		VRC2::write_9(0x9001, value);
		break;

	case 0x0003:
		VRC2::write_9(0x9003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_a
//
// Translates mapper 22 register addressing in the $A000-$AFFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_a(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_a(0xa000, value);
		break;

	case 0x0001:
		VRC2::write_a(0xa002, value);
		break;

	case 0x0002:
		VRC2::write_a(0xa001, value);
		break;

	case 0x0003:
		VRC2::write_a(0xa003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_b
//
// Translates mapper 22 register addressing in the $B000-$BFFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_b(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_b(0xb000, value);
		break;

	case 0x0001:
		VRC2::write_b(0xb002, value);
		break;

	case 0x0002:
		VRC2::write_b(0xb001, value);
		break;

	case 0x0003:
		VRC2::write_b(0xb003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_c
//
// Translates mapper 22 register addressing in the $C000-$CFFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_c(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_c(0xc000, value);
		break;

	case 0x0001:
		VRC2::write_c(0xc002, value);
		break;

	case 0x0002:
		VRC2::write_c(0xc001, value);
		break;

	case 0x0003:
		VRC2::write_c(0xc003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_d
//
// Translates mapper 22 register addressing in the $D000-$DFFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_d(uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_d(0xd000, value);
		break;

	case 0x0001:
		VRC2::write_d(0xd002, value);
		break;

	case 0x0002:
		VRC2::write_d(0xd001, value);
		break;

	case 0x0003:
		VRC2::write_d(0xd003, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// Mapper22::write_e
//
// Translates mapper 22 register addressing in the $E000-$EFFF range to the
// common VRC2 register layout.
//
// Mapper 22 swaps address lines A0 and A1, so register offsets 1 and 2 are
// exchanged before forwarding the write.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the mapper register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Mapper22::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0x0003) {
	case 0x0000:
		VRC2::write_e(0xe000, value);
		break;

	case 0x0001:
		VRC2::write_e(0xe002, value);
		break;

	case 0x0002:
		VRC2::write_e(0xe001, value);
		break;

	case 0x0003:
		VRC2::write_e(0xe003, value);
		break;
	}
}

