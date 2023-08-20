
#include "Mapper011.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(11)

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Mapper11::Mapper11() {
	set_prg_89abcdef(0);
	set_chr_0000_1fff(0);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
std::string Mapper11::name() const {
	return "Color Dreams";
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_8(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_9(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_a(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_b(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_c(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_d(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_e(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_f(uint_least16_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper11::write_handler(uint_least16_t address, uint8_t value) {
	(void)address;
	set_prg_89abcdef(value & 0x3);
	set_chr_0000_1fff((value >> 4) & 0x0f);
}
