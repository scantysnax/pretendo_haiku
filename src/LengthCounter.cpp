
#include "LengthCounter.h"
#include "Apu.h"

namespace nes::apu {

namespace {

const uint8_t length_table[32] = {
	0x0a, 0xfe,
	0x14, 0x02,
	0x28, 0x04,
	0x50, 0x06,
	0xa0, 0x08,
	0x3c, 0x0a,
	0x0e, 0x0c,
	0x1a, 0x0e,
	0x0c, 0x10,
	0x18, 0x12,
	0x30, 0x14,
	0x60, 0x16,
	0xc0, 0x18,
	0x48, 0x1a,
	0x10, 0x1c,
	0x20, 0x1e};
};

// -----------------------------------------------------------------------------
// LengthCounter::load
//
// Schedules a new length-counter value from the NES length lookup table.
//
// If a previous reload is still pending, that value is first committed.  The
// new reload value and the current APU cycle are then recorded so the hardware
// timing rules can be applied when the counter is next clocked.
//
// Parameters:
//   index - Length-table index from the channel register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
LengthCounter::load (uint8_t index)
{
	if (reload_) {
		value_ = reload_value_;
	}

	reload_value_ = length_table[index & 0x1f];
	reload_       = true;
	reload_cycle_ = nes::apu::cycle_count();
}



// -------------------------------------------------------------------------
// LengthCounter::clear
//
// Immediately clears the length counter when its channel is disabled
// through APU register $4015.
//
// NES hardware forces the length counter to zero as soon as the
// corresponding channel-enable bit is cleared. A pending length reload
// must also be discarded so that it cannot restore the previous value
// after the channel has been disabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------------------
void
LengthCounter::clear()
{
	value_ = 0;
	reload_value_ = 0;
	reload_ = false;
	reload_cycle_ = static_cast<uint64_t>(-1);
}


// -----------------------------------------------------------------------------
// LengthCounter::halt
//
// Requests that the length counter stop decrementing.
//
// The previous halt state and current APU cycle are recorded so the one-cycle
// delay in applying the halt state can be emulated correctly.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
LengthCounter::halt()
{
	prev_halt_  = halt_;
	halt_       = true;
	halt_cycle_ = nes::apu::cycle_count();
}


// -----------------------------------------------------------------------------
// LengthCounter::resume
//
// Requests that the length counter resume decrementing.
//
// The previous halt state and current APU cycle are recorded so the one-cycle
// delay in applying the new halt state can be emulated correctly.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
LengthCounter::resume()
{
	prev_halt_  = halt_;
	halt_       = false;
	halt_cycle_ = nes::apu::cycle_count();
}


// -----------------------------------------------------------------------------
// LengthCounter::value
//
// Returns the current length-counter value.
//
// If a reload is pending, the pending value is committed before the counter is
// returned.
//
// Parameters:
//   None.
//
// Returns:
//   Current length-counter value.
// -----------------------------------------------------------------------------
uint8_t
LengthCounter::value() const
{
	if (reload_) {
		value_  = reload_value_;
		reload_ = false;
	}

	return value_;
}


// -----------------------------------------------------------------------------
// LengthCounter::debug_value
//
// Returns the effective length-counter value for debugger inspection without
// modifying emulation state.
//
// If a reload is pending, the pending reload value is returned without committing
// it and without clearing the reload flag.  This allows debugger views to inspect
// the counter without affecting APU timing or length-counter behavior.
//
// Parameters:
//   None.
//
// Returns:
//   Effective current length-counter value.
// -----------------------------------------------------------------------------
uint8_t
LengthCounter::debug_value() const
{
	if (reload_) {
		return reload_value_;
	}

	return value_;
}


// -----------------------------------------------------------------------------
// LengthCounter::clock
//
// Advances the length counter by one half-frame clock.
//
// Pending reloads are applied according to their APU-cycle timing rules.  The
// counter is decremented only when the effective halt state is clear, with
// same-cycle halt changes delayed by one cycle to match hardware behavior.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
LengthCounter::clock()
{
	bool prevent_decrement       = false;
	const uint64_t current_cycle = nes::apu::cycle_count();

	if (reload_) {
		if (reload_cycle_ == current_cycle && value_ == 0) {
			value_            = reload_value_;
			prevent_decrement = true;
		} else if (reload_cycle_ == current_cycle && value_ != 0) {
			// no reload!
		} else {
			value_ = reload_value_;
		}
	}

	if (!prevent_decrement) {
		bool halted;

		// delay the halt 1 cycle
		if (halt_cycle_ == current_cycle) {
			halted = prev_halt_;
		} else {
			halted = halt_;
		}

		if (!halted && value_ > 0) {
			--value_;
		}
	}

	reload_ = false;
}


}

