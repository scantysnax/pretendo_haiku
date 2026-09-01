#ifndef _SQUARE_H_
#define _SQUARE_H_

#include "Envelope.h"
#include "LengthCounter.h"
#include "Sweep.h"
#include "Timer.h"
#include <cstdint>

namespace nes::apu {

template <int Channel>
class Square {
	friend class Sweep<Channel>;
	static_assert(Channel >= 0 && Channel < 2, "only channels 0 and 1 are valid");

public:
	// -------------------------------------------------------------------------
	// Square::enable
	//
	// Enables the square-wave channel.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void enable() {
		enabled_ = true;
	}

	// -------------------------------------------------------------------------
	// Square::disable
	//
	// Disables the square-wave channel and clears its length counter.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void disable() {
		enabled_ = false;
		length_counter.clear();
	}

	// -------------------------------------------------------------------------
	// Square::set_enabled
	//
	// Enables or disables the square-wave channel.
	//
	// Parameters:
	//   value - true to enable the channel, false to disable it.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void set_enabled(bool value) {
		if (value) {
			enable();
		} else {
			disable();
		}
	}

public:
	// -------------------------------------------------------------------------
	// Square::write_reg0
	//
	// Writes the square channel's duty-cycle, envelope, and length-counter
	// control register.
	//
	// The high two bits select the duty sequence.  Bit 5 controls the length
	// counter halt state, and the complete value is passed to the envelope.
	//
	// Parameters:
	//   value - Value written to the channel's first control register.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void write_reg0(uint8_t value) {
		duty_ = (value >> 6) & 0x03;

		if (value & 0x20) {
			length_counter.halt();
		} else {
			length_counter.resume();
		}

		envelope.set_control(value);
	}

	// -------------------------------------------------------------------------
	// Square::write_reg1
	//
	// Writes the square channel's sweep-unit control register.
	//
	// Parameters:
	//   value - Sweep control value.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void write_reg1(uint8_t value) {
		sweep.set_control(value);
	}

	// -------------------------------------------------------------------------
	// Square::write_reg2
	//
	// Writes the low eight bits of the square channel timer period.
	//
	// The resulting timer frequency and sweep-unit pulse period are updated
	// immediately.
	//
	// Parameters:
	//   value - Low eight bits of the timer reload value.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void write_reg2(uint8_t value) {
		timer_reload_    = (timer_reload_ & 0xff00) | value;
		timer_.frequency = (timer_reload_ + 1) * 2;
		sweep.set_pulse_period(timer_reload_);
	}

	// -------------------------------------------------------------------------
	// Square::write_reg3
	//
	// Writes the high timer-period bits and length-counter load value.
	//
	// The timer and sweep period are updated, the length counter is loaded when
	// the channel is enabled, and the duty sequencer and envelope are restarted.
	//
	// Parameters:
	//   value - High timer-period bits and length-table index.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void write_reg3(uint8_t value) {
		timer_reload_    = (timer_reload_ & 0x00ff) | ((value & 0x07) << 8);
		timer_.frequency = (timer_reload_ + 1) * 2;
		sweep.set_pulse_period(timer_reload_);

		if (enabled_) {
			length_counter.load((value >> 3) & 0x1f);
		}

		sequence_index_ = 0;
		envelope.start();
	}

public:
	// -------------------------------------------------------------------------
	// Square::enabled
	//
	// Reports whether the square-wave channel is enabled.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the channel is enabled.
	// -------------------------------------------------------------------------
	bool enabled() const {
		return enabled_;
	}
	
	// -------------------------------------------------------------------------
	// Square::mute
	//
	// Mutes this square-wave channel without changing its emulation state.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void mute() {
		channel_muted_ = true;
	}
	
	// -------------------------------------------------------------------------
	// Square::unmute
	//
	// Restores audible output from this square-wave channel.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void unmute() {
		channel_muted_ = false;
	}

public:
	// -------------------------------------------------------------------------
	// Square::tick
	//
	// Advances the square channel timer.
	//
	// Each timer expiration advances the eight-step duty sequencer by one
	// position.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void tick() {
		timer_.tick([this]() {
			sequence_index_ = (sequence_index_ + 1) % 8;
		});
	}

	// -------------------------------------------------------------------------
	// Square::output
	//
	// Returns the current square-channel output level.
	//
	// Output is suppressed when the channel is muted, the timer period is too
	// small, the current duty-sequence position is low, the sweep unit silences
	// the channel, or the length counter has expired.  Otherwise the envelope
	// volume is returned.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Current square-channel output level.
	// -------------------------------------------------------------------------
	uint8_t output() const {
		static const uint8_t sequence[4][8] = {
			{0, 1, 0, 0, 0, 0, 0, 0},
			{0, 1, 1, 0, 0, 0, 0, 0},
			{0, 1, 1, 1, 1, 0, 0, 0},
			{1, 0, 0, 1, 1, 1, 1, 1},
		};
		
		if (channel_muted_) {
			return 0;
		}

		if ((timer_.frequency - 1) < 8) {
			return 0;
		} else if (sequence[duty_][sequence_index_] == 0) {
			return 0;
		} else if (sweep.silenced()) {
			return 0;
		} else if (length_counter.value() == 0) {
			return 0;
		} else {
			return envelope.volume();
		}
	}

// debug things...	
public:
	uint16_t debug_timer_period() const {
		return timer_reload_;
	}

	uint16_t debug_timer_frequency() const {
		return timer_.frequency;
	}

	uint8_t debug_duty() const {
		return duty_;
	}

	uint8_t debug_sequence_index() const {
		return sequence_index_;
	}

	bool debug_muted() const {
		return channel_muted_;
	}

	uint8_t debug_output() const {
		static const uint8_t sequence[4][8] = {
			{0, 1, 0, 0, 0, 0, 0, 0},
			{0, 1, 1, 0, 0, 0, 0, 0},
			{0, 1, 1, 1, 1, 0, 0, 0},
			{1, 0, 0, 1, 1, 1, 1, 1},
		};

		if (channel_muted_) {
			return 0;
		}

		if ((timer_.frequency - 1) < 8) {
			return 0;
		}

		if (sequence[duty_][sequence_index_] == 0) {
			return 0;
		}

		if (sweep.silenced()) {
			return 0;
		}

		if (length_counter.debug_value() == 0) {
			return 0;
		}

		return envelope.volume();
	}

public:
	LengthCounter length_counter;
	Envelope envelope;
	Sweep<Channel> sweep{this};

private:
	Timer timer_;
	uint16_t timer_reload_  = 0;
	uint8_t duty_           = 0;
	uint8_t sequence_index_ = 0;
	bool enabled_           = false;
	bool channel_muted_		= false;
};

}

#endif // _SQUARE_H_
