#ifndef _LENGTH_COUNTER_H_
#define _LENGTH_COUNTER_H_

#include <cstdint>


namespace nes::apu {

// APU length counter shared by pulse, triangle, and noise channels.
class LengthCounter
{
	public:
	void clear();
	void halt();
	void load(uint8_t index);
	void resume();
	void clock();

	public:
	// Current counter value; debug_value() exposes it without affecting emulation state.
	uint8_t value() const;
	uint8_t debug_value() const;

	private:
	// Timing and deferred-reload state used to reproduce APU length-counter behavior.
	uint64_t halt_cycle_   = static_cast<uint64_t>(-1);
	uint64_t reload_cycle_ = static_cast<uint64_t>(-1);
	uint8_t reload_value_  = 0;
	mutable uint8_t value_ = 0;
	bool halt_             = false;
	bool prev_halt_        = false;
	mutable bool reload_   = false;
};


}


#endif // _LENGTH_COUNTER_H_
