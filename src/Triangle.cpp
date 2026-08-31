#include "Triangle.h"

namespace nes::apu {
namespace {


const uint8_t sequence[32] = {
	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};


}


// -----------------------------------------------------------------------------
// Triangle::set_enabled
//
// Enables or disables the triangle channel.
//
// Parameters:
//   value - true to enable the channel, false to disable it.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::set_enabled(bool value) {
	if (value) {
		enable();
	} else {
		disable();
	}
}


// -----------------------------------------------------------------------------
// Triangle::enable
//
// Enables the triangle channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::enable() {
	enabled_ = true;
}


// -----------------------------------------------------------------------------
// Triangle::disable
//
// Disables the triangle channel and clears its length counter.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::disable() {
	enabled_ = false;
	length_counter.clear();
}


// -----------------------------------------------------------------------------
// Triangle::write_reg0
//
// Writes the triangle channel's linear-counter and length-counter control
// register.
//
// Bit 7 controls the length-counter halt state, and the complete value is passed
// to the linear counter as its control value.
//
// Parameters:
//   value - Value written to triangle register $4008.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::write_reg0(uint8_t value) {

	if (value & 0x80) {
		length_counter.halt();
	} else {
		length_counter.resume();
	}

	linear_counter.set_control(value);
}


// -----------------------------------------------------------------------------
// Triangle::write_reg2
//
// Writes the low eight bits of the triangle channel timer period.
//
// Parameters:
//   value - Low eight bits of the timer reload value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::write_reg2(uint8_t value) {
	timer_load_      = (timer_load_ & 0xff00) | value;
	timer_.frequency = (timer_load_ + 1);
}


// -----------------------------------------------------------------------------
// Triangle::write_reg3
//
// Writes the high timer-period bits and length-counter load value.
//
// When enabled, the length counter is loaded from the encoded table index.  The
// timer period is updated and the linear counter is marked for reload.
//
// Parameters:
//   value - High timer-period bits and length-table index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::write_reg3(uint8_t value) {

	if (enabled_) {
		length_counter.load((value >> 3) & 0x1f);
	}

	timer_load_      = (timer_load_ & 0x00ff) | ((value & 0x07) << 8);
	timer_.frequency = (timer_load_ + 1);

	linear_counter.reload();
}


// -----------------------------------------------------------------------------
// Triangle::enabled
//
// Reports whether the triangle channel is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true if the triangle channel is enabled.
// -----------------------------------------------------------------------------
bool Triangle::enabled() const {
	return enabled_;
}


// -----------------------------------------------------------------------------
// Triangle::tick
//
// Advances the triangle channel timer.
//
// Each timer expiration advances the 32-step waveform sequencer only while both
// the length counter and linear counter are nonzero.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Triangle::tick() {

	timer_.tick([this]() {
		if (length_counter.value() && linear_counter.value()) {
			sequence_index_ = (sequence_index_ + 1) % 32;
		}
	});
}


// -----------------------------------------------------------------------------
// Triangle::output
//
// Returns the current triangle-channel DAC output level.
//
// Muted channels and timer periods below the supported range produce zero.
// Otherwise, the current value from the 32-step triangle waveform is returned.
//
// Parameters:
//   None.
//
// Returns:
//   Current triangle-channel output level.
// -----------------------------------------------------------------------------
uint8_t Triangle::output() const {
	
	if (channel_muted_) {
		return 0x00;
	}
	
	if (timer_.frequency < 4) {
		return 0x00;
	} else {
		return sequence[sequence_index_];
	}
}

}
