
#ifndef LFSR_20110314_H_
#define LFSR_20110314_H_

#include <cstdint>

namespace nes::apu {

class LFSR {
	public:
	// -------------------------------------------------------------------------
	// LFSR::set_mode
	//
	// Sets the feedback mode used by the linear-feedback shift register.
	//
	// Parameters:
	//   value - Mode value selecting the feedback tap configuration.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void set_mode(uint8_t value) {
		mode_ = value;
	}

	// -------------------------------------------------------------------------
	// LFSR::load
	//
	// Loads a new value into the linear-feedback shift register.
	//
	// Parameters:
	//   value - Value to load into the shift register.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void load(uint8_t value) {
		value_ = value;
	}

	// -------------------------------------------------------------------------
	// LFSR::clock
	//
	// Advances the linear-feedback shift register by one step.
	//
	// The current feedback bit is calculated, the register is shifted right,
	// and the feedback result is inserted into bit 14.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void clock() {
		const uint16_t f = feedback();
		value_ >>= 1;
		value_ = (value_ & ~0x4000) | (f << 14);
	}

	// -------------------------------------------------------------------------
	// LFSR::value
	//
	// Returns the current 15-bit linear-feedback shift-register value.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Current LFSR value with all bits above bit 14 cleared.
	// -------------------------------------------------------------------------
	uint16_t value() const {
		return value_ & 0x7fff;
	}

	private:
	// -------------------------------------------------------------------------
	// LFSR::feedback
	//
	// Calculates the next feedback bit for the linear-feedback shift register.
	//
	// Normal mode XORs bits 0 and 1.  Alternate mode XORs bits 0 and 6,
	// matching the two feedback configurations used by the NES noise channel.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Next single-bit feedback value.
	// -------------------------------------------------------------------------
	uint16_t feedback() const {
		if (mode_) {
			return (value_ & 0x01) ^ ((value_ >> 6) & 0x01);
		} else {
			return (value_ & 0x01) ^ ((value_ >> 1) & 0x01);
		}
	}

	private:
	uint16_t value_ = 1;
	uint8_t mode_   = 0;
};

}


#endif
