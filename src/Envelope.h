#ifndef _ENVELOPE_H_
#define _ENVELOPE_H_


#include <cstdint>


namespace nes::apu {

class Envelope {
public:
	void clock();
	void start();
	void set_control(uint8_t value);
	uint8_t volume() const;

public:
	uint8_t debug_volume() const {
		return volume();	// 4-bit volume
	}

	uint8_t debug_control() const {
		return control_;	// envelope
	}

	uint8_t debug_counter() const {
		return counter_;	// decay
	}

	uint8_t debug_divider() const {
		return divider_;	// envelope divider
	}

	bool debug_start_pending() const {
		return start_;	// pending envelope restart
	}

private:
	void clock_divider();

private:
	bool start_      = false;
	uint8_t counter_ = 0;
	uint8_t divider_ = 0xff;
	uint8_t control_ = 0;
};


}


#endif

