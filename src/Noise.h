
#ifndef _NOISE_H_
#define _NOISE_H_

#include "Envelope.h"
#include "LengthCounter.h"
#include "Lfsr.h"
#include "Timer.h"
#include <cstdint>

namespace nes::apu {

class Noise {
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
	void tick();
	uint8_t output() const;
	
	void mute() { 
		channel_muted_ = true;
	}
	
	void unmute() {
		channel_muted_ = false;
	}
	
	

public:
	LengthCounter length_counter;
	Envelope envelope;

private:
	bool enabled_ = false;
	Timer timer_;
	LFSR lfsr_;
	bool channel_muted_ = false;
};

}


#endif // _NOISE_H_
