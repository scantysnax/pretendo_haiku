
#include "Mapper007.h"
#include "Cart.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(7)

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Mapper7::Mapper7() {
	set_prg_89abcdef(-1);
	set_chr_0000_1fff_ram(chr_ram_, 0);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
std::string Mapper7::name() const {
	return "AxROM";
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_8(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_9(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_a(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_b(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_c(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_d(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_e(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_f(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper7::write_handler(uint32_t address, uint8_t value) {
	(void)address;
	set_prg_89abcdef(value & 0x07);

	if (value & 0x10) {
		set_mirroring(mirror_single_high);
	} else {
		set_mirroring(mirror_single_low);
	}
}
