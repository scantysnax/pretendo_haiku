#ifndef _SWEEP_H_
#define _SWEEP_H_


#include <cstdint>


namespace nes::apu {


template <int Channel>
class Square;


template <int Channel>
class Sweep {
public:
	// -------------------------------------------------------------------------
	// Sweep::Sweep
	//
	// Constructs a sweep unit associated with the supplied square-wave channel.
	//
	// Parameters:
	//   square - Square channel controlled by this sweep unit.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	explicit Sweep(Square<Channel> *square)
		: square_(square) {
	}

	~Sweep()             = default;
	Sweep(const Sweep &) = delete;
	Sweep &operator=(const Sweep &) = delete;

public:
	// -------------------------------------------------------------------------
	// Sweep::clock
	//
	// Advances the sweep-unit divider and applies a frequency adjustment when
	// the divider expires under the active sweep configuration.
	//
	// Reload requests reset the divider immediately.  When an adjustment is
	// required, the target period is calculated and applied only if it remains
	// within the valid 11-bit pulse-period range.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void clock() {
		// If the reload flag flag is set, the divider's counter is set to the
		// period P. If the divider's counter was zero before the reload, the
		// pulse's period is also adjusted.
		if (reload_) {
			reload_                    = false;
			const uint8_t prev_counter = counter_;
			counter_                   = period() + 1;

			if (prev_counter == 0 && enabled() && shift()) {
				const uint16_t target = target_period();
				if (target < 0x800) {
					square_->timer_reload_    = target;
					pulse_period_             = target;
					square_->timer_.frequency = (target + 1) * 2;
				}
			}

		} else {
			if (counter_) {
				// If the reload flag is clear and the divider's counter is non-zero,
				// it is decremented.
				--counter_;
			} else {
				// If the reload flag is clear and the divider's counter is zero,
				// the counter is set to P and the pulse's period is adjusted.
				counter_ = period() + 1;

				if (enabled() && shift()) {
					const uint16_t target = target_period();
					if (target < 0x800) {
						square_->timer_reload_    = target;
						pulse_period_             = target;
						square_->timer_.frequency = (target + 1) * 2;
					}
				}
			}
		}
	}

	// -------------------------------------------------------------------------
	// Sweep::set_control
	//
	// Stores a new sweep control value and requests that the divider reload on
	// the next sweep clock.
	//
	// Parameters:
	//   value - Sweep enable, period, negate, and shift control bits.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void set_control(uint8_t value) {
		control_ = value;
		reload_  = true;
	}

	// -------------------------------------------------------------------------
	// Sweep::set_pulse_period
	//
	// Updates the raw pulse timer period used as the basis for sweep
	// calculations.
	//
	// Parameters:
	//   value - Current 11-bit pulse timer period.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void set_pulse_period(uint16_t value) {
		pulse_period_ = value;
	}

	// -------------------------------------------------------------------------
	// Sweep::silenced
	//
	// Reports whether the sweep unit currently silences the square channel.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the sweep unit is silencing the channel.
	// -------------------------------------------------------------------------
	bool silenced() const {
		return silenced_;
	}
	
	public:
	bool debug_enabled() const {
		return enabled();
	}

	bool debug_negate() const {
		return negate();
	}

	uint8_t debug_period() const {
		return period();
	}

	uint8_t debug_shift() const {
		return shift();
	}

	bool debug_silenced() const {
		return silenced_;
	}

	uint16_t debug_pulse_period() const {
		return pulse_period_;
	}

	uint8_t debug_counter() const {
		return counter_;
	}

	bool debug_reload_pending() const {
		return reload_;
	}
	

private:
	// -------------------------------------------------------------------------
	// Sweep::enabled
	//
	// Reports whether sweep processing is enabled by the control register.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the sweep unit is enabled.
	// -------------------------------------------------------------------------
	bool enabled() const {
		return control_ & 0x80;
	}

	// -------------------------------------------------------------------------
	// Sweep::negate
	//
	// Reports whether the sweep calculation subtracts the frequency delta
	// instead of adding it.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true when negate mode is selected.
	// -------------------------------------------------------------------------
	bool negate() const {
		return control_ & 0x08;
	}

	// -------------------------------------------------------------------------
	// Sweep::target_period
	//
	// Calculates the target timer period produced by the current sweep settings.
	//
	// The current period is shifted to form the sweep delta.  Positive sweeps add
	// that delta, while negative sweeps subtract it.  Pulse channel 1 applies the
	// NES one's-complement correction by subtracting one additional count.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Calculated target pulse timer period.
	// -------------------------------------------------------------------------
	uint16_t target_period() const {
		// The channel's 11-bit raw timer period is shifted right by the shift count
		// (using a barrel shifter), then either added to or subtracted from the
		// channel's raw period, yielding the target period.
		const uint16_t delta = pulse_period_ >> shift();

		if (negate()) {
			if constexpr (Channel == 0) {
				return pulse_period_ - delta - 1;
			} else {
				return pulse_period_ - delta;
			}
		} else {
			return pulse_period_ + delta;
		}
	}

	// -------------------------------------------------------------------------
	// Sweep::period
	//
	// Extracts the sweep divider period from the control register.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Three-bit sweep divider period.
	// -------------------------------------------------------------------------
	uint8_t period() const {
		return (control_ >> 4) & 0x07;
	}

	// -------------------------------------------------------------------------
	// Sweep::shift
	//
	// Extracts the sweep shift count from the control register.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Three-bit sweep shift count.
	// -------------------------------------------------------------------------
	uint8_t shift() const {
		return control_ & 0x07;
	}

private:
	Square<Channel> *square_;
	uint16_t pulse_period_ = 0;
	uint8_t counter_       = 0;
	uint8_t control_       = 0;
	bool reload_           = false;
	bool silenced_         = false;
};

}

#endif	// _SWEEP_H_
