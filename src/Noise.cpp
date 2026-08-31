
#include "Noise.h"

namespace nes::apu {

namespace {

// NTSC period table
const uint16_t frequency_table[16] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

}

// -----------------------------------------------------------------------------
// Noise::set_enabled
//
// Enables or disables the noise channel.
//
// Parameters:
//   value - true to enable the channel, false to disable it.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::set_enabled(bool value) {
	if (value) {
		enable();
	} else {
		disable();
	}
}


// -----------------------------------------------------------------------------
// Noise::enable
//
// Enables the noise channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::enable() {
	enabled_ = true;
}


// -----------------------------------------------------------------------------
// Noise::disable
//
// Disables the noise channel and clears its length counter.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::disable() {
	enabled_ = false;
	length_counter.clear();
}


// -----------------------------------------------------------------------------
// Noise::write_reg0
//
// Writes the noise channel's envelope and length-counter control register.
//
// The length counter is halted or resumed according to the control flag, and the
// full register value is passed to the envelope generator.
//
// Parameters:
//   value - Value written to noise register $400C.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::write_reg0(uint8_t value) {

	if (value & 0x20) {
		length_counter.halt();
	} else {
		length_counter.resume();
	}

	envelope.set_control(value);
}


// -----------------------------------------------------------------------------
// Noise::write_reg2
//
// Writes the noise channel's mode and timer-period register.
//
// The high bit selects the LFSR feedback mode, while the low four bits select
// the timer period from the noise frequency table.
//
// Parameters:
//   value - Value written to noise register $400E.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::write_reg2(uint8_t value) {

	lfsr_.set_mode(value & 0x80);
	timer_.frequency = frequency_table[value & 0x0f];
}


// -----------------------------------------------------------------------------
// Noise::write_reg3
//
// Writes the noise channel's length-counter load register.
//
// When the channel is enabled, the length counter is loaded from the encoded
// table index.  The envelope generator is restarted on every write.
//
// Parameters:
//   value - Value written to noise register $400F.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::write_reg3(uint8_t value) {

	if (enabled_) {
		length_counter.load((value >> 3) & 0x1f);
	}

	envelope.start();
}


// -----------------------------------------------------------------------------
// Noise::enabled
//
// Reports whether the noise channel is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true if the noise channel is enabled.
// -----------------------------------------------------------------------------
bool Noise::enabled() const {
	return enabled_;
}


// -----------------------------------------------------------------------------
// Noise::tick
//
// Advances the noise channel timer by one APU clock.
//
// Each timer expiration clocks the linear-feedback shift register that generates
// the noise waveform.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Noise::tick() {

	timer_.tick([this]() {
		lfsr_.clock();
	});
}


// -----------------------------------------------------------------------------
// Noise::output
//
// Returns the current noise-channel output level.
//
// A muted channel, expired length counter, or inactive LFSR output bit produces
// zero.  Otherwise, the current envelope volume is returned.
//
// Parameters:
//   None.
//
// Returns:
//   Current noise-channel output level.
// -----------------------------------------------------------------------------
uint8_t Noise::output() const {
	
	if (channel_muted_) {
		return 0;
	}
	
	if (length_counter.value() == 0 || ((lfsr_.value() & 1) == 0)) {
		return 0;
	} else {
		return envelope.volume();
	}
}


}

