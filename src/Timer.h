#ifndef _TIMER_H_
#define _TIMER_H_

#include <cstdint>

namespace nes::apu {

class Timer {
public:
	// -------------------------------------------------------------------------
	// Timer::reset
	//
	// Resets the timer to its initial state.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void reset();

public:
	// -------------------------------------------------------------------------
	// Timer::tick
	//
	// Advances the timer by one clock.
	//
	// When the countdown reaches zero, the timer is reloaded from the current
	// frequency value and the supplied callback is invoked.
	//
	// Parameters:
	//   c - Callback invoked when the timer expires.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	template <class Callback>
	void tick(Callback c) {
		if (--timer_ == 0) {
			timer_ = frequency;
			c();
		}
	}

public:
	uint16_t frequency = 0xffff;

private:
	uint16_t timer_ = 0xffff;
};

}


#endif	// _TIMER_H_


