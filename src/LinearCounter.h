#ifndef LINEAR_COUNTER_20130206_H_
#define LINEAR_COUNTER_20130206_H_

#include <cstdint>

namespace nes::apu {

// Triangle-channel linear counter controlling whether waveform sequencing may advance.
class LinearCounter {
	public:
	uint8_t value() const { return value_; }
	void clock();
	void reload();
	void set_control(uint8_t value);

	private:
	// Current count, reload value, and pending-reload state.
	uint8_t value_   = 0;
	uint8_t control_ = 0;
	bool reload_     = false;
};

}


#endif
