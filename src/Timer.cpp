#include "Timer.h"

namespace nes::apu {

// -----------------------------------------------------------------------------
// Timer::reset
//
// Reloads the timer from its current frequency value.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Timer::reset() {
	timer_ = frequency;
}

}
