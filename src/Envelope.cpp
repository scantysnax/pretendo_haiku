
#include "Envelope.h"

namespace nes::apu {

// -----------------------------------------------------------------------------
// Envelope::volume
//
// Returns the current envelope output volume.
//
// When constant-volume mode is enabled, the low four control bits are returned.
// Otherwise, the current envelope decay counter supplies the output volume.
//
// Parameters:
//   None.
//
// Returns:
//   Current 4-bit envelope volume.
// -----------------------------------------------------------------------------
uint8_t Envelope::volume() const {
	if (control_ & 0x10) {
		return control_ & 0x0f;
	} else {
		return counter_;
	}
}


// -----------------------------------------------------------------------------
// Envelope::set_control
//
// Stores the envelope control register value.
//
// Parameters:
//   value - Envelope control value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Envelope::set_control(uint8_t value) {
	control_ = value;
}


// -----------------------------------------------------------------------------
// Envelope::start
//
// Requests that the envelope restart on the next envelope clock.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Envelope::start() {
	start_ = true;
}


// -----------------------------------------------------------------------------
// Envelope::clock
//
// Advances the envelope generator by one envelope clock.
//
// A pending restart resets the decay counter and divider.  Otherwise, the
// existing divider and decay state are advanced normally.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Envelope::clock() {
	if (!start_) {
		clock_divider();
	} else {
		start_   = false;
		counter_ = 15;
		divider_ = (control_ & 0x0f) + 1;
	}
}


// -----------------------------------------------------------------------------
// Envelope::clock_divider
//
// Advances the envelope divider and decay counter.
//
// When the divider expires, it is reloaded from the envelope period.  The decay
// counter is then decremented, or reloaded to 15 when envelope looping is
// enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Envelope::clock_divider() {

	if (--divider_ == 0) {
		divider_ = (control_ & 0x0f) + 1;
		if (counter_) {
			--counter_;
		} else if (control_ & 0x20) {
			counter_ = 15;
		}
	}
}


}

