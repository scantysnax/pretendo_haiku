
#include "Envelope.h"

namespace nes {
namespace apu {

//------------------------------------------------------------------------------
// Name: volume
//------------------------------------------------------------------------------
uint8_t Envelope::volume() const {
	if(control_ & 0x10) {
		return control_ & 0x0f;
	} else {
		return counter_;
	}
}

//------------------------------------------------------------------------------
// Name: set_loop
//------------------------------------------------------------------------------
void Envelope::set_control(uint8_t value) {
	control_ = value;
}

//------------------------------------------------------------------------------
// Name: start
//------------------------------------------------------------------------------
void Envelope::start() {
	start_ = true;
}

//------------------------------------------------------------------------------
// Name: clock
//------------------------------------------------------------------------------
void Envelope::clock() {
	if(!start_) {
		clock_divider();
	} else {
		start_   = false;
		counter_ = 15;
		divider_ = (control_ & 0x0f) + 1;
	}
}

//------------------------------------------------------------------------------
// Name: clock_divider
//------------------------------------------------------------------------------
void Envelope::clock_divider() {

	if(--divider_ == 0) {
		divider_ = (control_ & 0x0f) + 1;
		if(counter_) {
			--counter_;
		} else if(control_ & 0x20) {
			counter_ = 15;
		}
	}
}

}
}