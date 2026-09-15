#ifndef _DMC_H_
#define _DMC_H_

#include "BitField.h"
#include "ShiftRegister.h"
#include "Timer.h"
#include <cstdint>

namespace nes::apu {

// $4010 DMC control register: rate index, looping, and IRQ enable.
union DMCControl {
	uint8_t value;
	BitField<uint8_t, 0, 4> frequency;
	BitField<uint8_t, 6> loop;
	BitField<uint8_t, 7> irq;
};


// NES APU delta modulation channel.


class DMC {
public:
	// Channel enable/disable control.
	void enable();
	void disable();
	void set_enabled(bool value);

	public:
	// DMC register writes ($4010-$4013).
	void write_reg0(uint8_t value);
	void write_reg1(uint8_t value);
	void write_reg2(uint8_t value);
	void write_reg3(uint8_t value);

	public:
	// Sample reader state used by the APU/CPU memory-fetch path.
	uint16_t bytes_remaining() const;
	void load_sample_buffer(uint8_t value);

	public:
	// Debugger-facing channel state accessors.
	bool debug_enabled() const {
		return enabled_;
	}
	
	bool debug_active() const {
		return bytes_remaining_ != 0;
	}

	bool debug_muted() const {
		return channel_muted_;
	}

	uint16_t debug_timer_period() const {
		return timer_.frequency;
	}

	bool debug_irq_enabled() const {
		return irq_enabled();
	}

	bool debug_loop() const {
		return loop();
	}

	uint16_t debug_sample_address() const {
		return sample_address_;
	}

	uint16_t debug_current_address() const {
		return sample_pointer_;
	}

	uint16_t debug_sample_length() const {
		return sample_length_;
	}

	uint8_t debug_bits_remaining() const {
		return bits_remaining_;
	}

	bool debug_sample_buffer_empty() const {
		return sample_buffer_empty_;
	}


	public:
	// Advance the DMC and return its current 7-bit DAC output level.
	void tick();
	uint8_t output() const;
	void mute() { 
		channel_muted_ = true;
	}
	
	void unmute() {
		channel_muted_ = false;
	}

	private:
	// Control-register helpers and sample/output-unit clocks.
	bool irq_enabled() const;
	bool loop() const;
	bool output_clock();
	void start_cycle();
	void refill_sample_buffer();

	private:
	// Channel, sample-reader, output-unit, and timer state.
	bool enabled_              = false;
	bool muted_                = false;
	bool sample_buffer_empty_  = true;
	uint16_t sample_pointer_   = 0xc000;
	uint16_t sample_address_   = 0xc000;
	uint16_t bytes_remaining_  = 0;
	uint16_t sample_length_    = 0;
	uint8_t bits_remaining_    = 0;
	uint8_t output_            = 0;
	uint8_t sample_buffer_     = 0;
	ShiftRegister<uint8_t> shift_register_{0};
	DMCControl control_{0};
	Timer timer_;

	// Debugger/user-controlled audio mute; does not stop DMC emulation.
	bool channel_muted_ = false;
};

}


#endif // _DMC_H_
