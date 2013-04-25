
#include "Noise.h"

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Noise::Noise() : enabled_(false) {

}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
Noise::~Noise() {

}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Noise::enable() {
	enabled_ = true;
}

//------------------------------------------------------------------------------
// Name:
//------------------------------------------------------------------------------
void Noise::disable() {
	enabled_ = false;
	length_counter_.clear();
}

//------------------------------------------------------------------------------
// Name: write_reg0
//------------------------------------------------------------------------------
void Noise::write_reg0(uint8_t value) {

	if(value & 0x20) {
		length_counter_.halt();
	} else {
		length_counter_.resume();
	}
}

//------------------------------------------------------------------------------
// Name: write_reg2
//------------------------------------------------------------------------------
void Noise::write_reg2(uint8_t value) {
	(void)value;
}

//------------------------------------------------------------------------------
// Name: write_reg3
//------------------------------------------------------------------------------
void Noise::write_reg3(uint8_t value) {

	if(enabled_) {
		length_counter_.load((value >> 3) & 0x1f);
	}
}

//------------------------------------------------------------------------------
// Name: enabled
//------------------------------------------------------------------------------
bool Noise::enabled() const {
	return enabled_;
}

//------------------------------------------------------------------------------
// Name: length_counter
//------------------------------------------------------------------------------
LengthCounter &Noise::length_counter() {
	return length_counter_;
}

//------------------------------------------------------------------------------
// Name: tick
//------------------------------------------------------------------------------
void Noise::tick() {

}
