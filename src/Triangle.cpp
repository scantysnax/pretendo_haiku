#include "Triangle.h"

namespace nes::apu {
namespace {


// 32-step waveform sequence used by the NES APU triangle channel.
const uint8_t sequence[32] = {
	0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};


}


// -----------------------------------------------------------------------------
// Triangle::Triangle
//
// Initializes the triangle timer so its effective frequency matches the
// channel's initial timer-period state.
//
// The channel starts with timer_load_ equal to zero.  Since the timer uses
// timer_load_ + 1 as its effective frequency, initialize the Timer object to
// one rather than leaving its generic default frequency at 0xffff.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
Triangle::Triangle()
{
	timer_.frequency = timer_load_ + 1;
	timer_.reset();
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
void
Triangle::set_enabled(bool value)
{
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
void
Triangle::enable()
{
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
void
Triangle::disable()
{
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
void
Triangle::write_reg0 (uint8_t value)
{
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
void
Triangle::write_reg2 (uint8_t value)
{
	timer_load_      = (timer_load_ & 0xff00) | value;
	timer_.frequency = timer_load_ + 1;
}


// -----------------------------------------------------------------------------
// Triangle::write_reg3
//
// Writes the high timer-period bits and length-counter load value.
//
// When enabled, the length counter is loaded from the encoded table index. The
// timer period is updated and the linear counter is marked for reload.
//
// Parameters:
//   value - High timer-period bits and length-table index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Triangle::write_reg3 (uint8_t value)
{
	if (enabled_) {
		length_counter.load((value >> 3) & 0x1f);
	}

	timer_load_ =
		(timer_load_ & 0x00ff)
		| ((value & 0x07) << 8);

	timer_.frequency = timer_load_ + 1;

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
bool
Triangle::enabled() const
{
	return enabled_;
}

// -----------------------------------------------------------------------------
// Triangle::reset
//
// Resets the triangle channel's internal timer and waveform state.
//
// This clears state retained from the previously running program before APU
// reset register writes are applied.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
Triangle::reset()
{
	enabled_ = false;

	timer_load_ = 0;
	sequence_index_ = 0;

	timer_.frequency = 1;
	timer_.reset();

	length_counter.clear();
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
void
Triangle::tick()
{
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
// The triangle DAC retains the current sequencer value even while the waveform
// sequencer is not advancing. Timer period, length-counter state, and
// linear-counter state control sequencing rather than directly forcing the DAC
// output to zero.
//
// Debugger muting is handled separately and does not affect channel emulation.
//
// Parameters:
//   None.
//
// Returns:
//   Current triangle-channel DAC output level.
// -----------------------------------------------------------------------------
uint8_t
Triangle::output() const
{
	if (channel_muted_) {
		return 0x00;
	}

	return sequence[sequence_index_];
}

}
