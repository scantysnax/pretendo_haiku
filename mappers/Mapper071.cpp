
#include "Mapper071.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(71)

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Mapper71::Mapper71() {
	set_prg_89ab(0);
	set_prg_cdef(-1);
	set_chr_0000_1fff_ram(chr_ram_, 0);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
std::string Mapper71::name() const {
	return "Camerica/Codemasters";
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_8(uint32_t address, uint8_t value) {
	(void)address;
	(void)value;
#if 0
	// firehawk only
	if(value & 0x10) {
		set_mirroring(mirror_single_high);
	} else {
		set_mirroring(mirror_single_low);
	}
#endif
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_9(uint32_t address, uint8_t value) {
	write_8(address, value);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_c(uint32_t address, uint8_t value) {
	(void)address;
	set_prg_89ab(value & 0x0f);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_d(uint32_t address, uint8_t value) {
	(void)address;
	set_prg_89ab(value & 0x0f);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_e(uint32_t address, uint8_t value) {
	(void)address;
	set_prg_89ab(value & 0x0f);
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Mapper71::write_f(uint32_t address, uint8_t value) {
	(void)address;
	set_prg_89ab(value & 0x0f);
}
