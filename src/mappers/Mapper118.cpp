
#include "Mapper118.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(118)

// TODO: mirroring crap

//------------------------------------------------------------------------------
// Name: name
//------------------------------------------------------------------------------
std::string Mapper118::name() const {
	return "TxSROM";
}

//------------------------------------------------------------------------------
// Name: Mapper4
//------------------------------------------------------------------------------
Mapper118::Mapper118() {
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper118::write_8(uint_least16_t address, uint8_t value) {
	MMC3::write_8(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper118::write_9(uint_least16_t address, uint8_t value) {
	MMC3::write_9(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper118::write_a(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper118::write_b(uint_least16_t address, uint8_t value) {
	(void)address;
	(void)value;
}