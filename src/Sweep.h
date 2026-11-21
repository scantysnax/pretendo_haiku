#ifndef _SWEEP_H_
#define _SWEEP_H_


#include <algorithm>
#include <cstdint>


namespace nes::apu {


template <int Channel>
class Square;


template <int Channel>
class Sweep
{
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
	void clock()
	{
		// ---------------------------------------------------------------------
		// Clock the sweep divider and conditionally update the pulse timer.
		//
		// The divider reloads with the programmed period P, producing an
		// interval of P + 1 half-frame clocks.
		//
		// A pending reload takes priority over decrementing the divider,
		// but does not suppress an adjustment if the divider has expired.
		//
		// Sweep adjustments are suppressed when the current pulse period
		// is below 8 or the target exceeds the 11-bit hardware limit.
		// ---------------------------------------------------------------------

		const bool divider_expired = (counter_ == 0);

		// Apply a sweep adjustment only on a divider output clock.
		if (divider_expired && enabled() && shift() != 0 && !silenced()) {
			const uint16_t target = target_period();

			square_->timer_reload_ = target;
			pulse_period_ = target;
			square_->timer_.frequency =
				(static_cast<uint32_t>(target) + 1) * 2;
		}

		// Reload has priority over decrementing the divider.
		if (divider_expired || reload_) {
			counter_ = period();
			reload_ = false;
		} else {
			--counter_;
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
	// Determines whether the pulse channel is silenced by its timer
	// period or the sweep target-period overflow condition.
	//
	// The NES silences pulse output when:
	//
	//   - The current timer period is less than 8.
	//   - The sweep target period exceeds the 11-bit limit ($7FF).
	//
	// These conditions apply even when sweep updating is disabled.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the pulse channel must produce silence.
	// -------------------------------------------------------------------------
	bool silenced() const {
		return pulse_period_ < 8 || target_period() > 0x07ff;
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
		return silenced();
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
	// Calculates the target pulse timer period from the current period,
	// sweep shift count, and negate setting.
	//
	// Pulse 1 uses one's-complement subtraction (subtracts an additional 1),
	// while Pulse 2 uses two's-complement subtraction.
	//
	// Negative results are clamped to zero, matching NES sweep behavior.
	// Positive overflow is preserved so silenced() can detect a target
	// exceeding the 11-bit timer range.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Calculated target pulse timer period.
	// -------------------------------------------------------------------------
	uint16_t target_period() const {
		const int32_t period = static_cast<int32_t>(pulse_period_);
		const int32_t delta = period >> shift();

		int32_t target;

		if (negate()) {
			if constexpr (Channel == 0) {
				target = period - delta - 1;
			} else {
				target = period - delta;
			}
		} else {
			target = period + delta;
		}

		return static_cast<uint16_t>(std::max<int32_t>(0, target));
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
};

}

#endif	// _SWEEP_H_
