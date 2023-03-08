
#include "Mapper041.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(41)

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Mapper41::Mapper41() {

	set_prg_89abcdef(0);
	set_chr_0000_1fff(0);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
std::string Mapper41::name() const {
	return "Caltron 6-in-1";
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_6(uint32_t address, uint8_t value) {
	(void)value;

	if (!(address & 0x800)) {
		register1_ = static_cast<uint8_t>(address & 0x3f);
		set_prg_89abcdef(register1_ & 0x7);

		if (value & 0x10) {
			set_mirroring(mirror_horizontal);
		} else {
			set_mirroring(mirror_vertical);
		}
	}
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_8(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_9(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_a(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_b(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_c(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_d(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_e(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_f(uint32_t address, uint8_t value) {
	write_handler(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper41::write_handler(uint32_t address, uint8_t value) {
	(void)address;
	register2_ = value;
	if (register1_ & 0x04) {
		set_chr_0000_1fff(((register2_ & 0x03) | (register1_ >> 1)) & 0xc);
	}
}
