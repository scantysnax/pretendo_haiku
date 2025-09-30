
#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

#include "LengthCounter.h"
#include "LinearCounter.h"
#include "Timer.h"
#include <cstddef>
#include <cstdint>

namespace nes::apu {

class Triangle {
public:
	void enable();
	void disable();
	void set_enabled(bool value);

public:
	void write_reg0(uint8_t value);
	void write_reg2(uint8_t value);
	void write_reg3(uint8_t value);

public:
	bool enabled() const;

public:
	LengthCounter length_counter;
	LinearCounter linear_counter;

public:
	void tick();
	uint8_t output() const;
	
	void mute() { 
		channel_muted_ = true;
	}
	
	void unmute() {
		channel_muted_ = false;
	}
		

private:
	bool enabled_          = false;
	uint16_t timer_load_   = 0;
	size_t sequence_index_ = 0;
	Timer timer_;
	bool channel_muted_	= false;
};

}

#endif // _TRIANGLE_H_
