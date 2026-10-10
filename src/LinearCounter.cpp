
#include "LinearCounter.h"

namespace nes::apu {

// -----------------------------------------------------------------------------
// LinearCounter::clock
//
// Advances the triangle channel's linear counter by one quarter-frame clock.
//
// When a reload is pending, the counter is loaded from the low seven control
// bits.  Otherwise, a nonzero counter is decremented.  If the control flag is
// clear, the reload request is then cleared.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void LinearCounter::clock() {

	if (reload_) {
		value_ = (control_ & 0x7f);
	} else if (value_) {
		--value_;
	}

	if (!(control_ & 0x80)) {
		reload_ = false;
	}
}


// -----------------------------------------------------------------------------
// LinearCounter::set_control
//
// Stores the triangle linear-counter control register value.
//
// Parameters:
//   value - Linear-counter control value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void LinearCounter::set_control(uint8_t value) {
	control_ = value;
}


// -----------------------------------------------------------------------------
// LinearCounter::reload
//
// Requests that the linear counter reload on its next clock.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void LinearCounter::reload() {
	reload_ = true;
}

// -----------------------------------------------------------------------------
// LinearCounter::reset
//
// Resets the triangle linear counter's current value and pending reload state.
//
// The programmed control value is preserved so this diagnostic does not alter
// the emulated $4008 register contents.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
LinearCounter::reset()
{
	value_ = 0;
	reload_ = false;
}

}

