
#include "Dmc.h"
#include "Apu.h"
#include "Cpu.h"
#include "Mapper.h"

namespace nes::apu {
namespace {

// NTSC period table
const uint16_t frequency_table[16] = {
	0x1ac, 0x17c, 0x154, 0x140,
	0x11e, 0x0fe, 0x0e2, 0x0d6,
	0x0be, 0x0a0, 0x08e, 0x080,
	0x06a, 0x054, 0x048, 0x036};

};

/* NOTE:
 * The following is speculation, and thus not necessarily 100% accurate. It does accurately predict observed behavior.
 * The 6502 cannot be pulled off of the bus normally. The 2A03 DMC gets around this by pulling RDY low internally.
 * This causes the CPU to pause during the next read cycle, until RDY goes high again. The DMC unit holds RDY low for 4 cycles.
 * The first three cycles it idles, as the CPU could have just started an interrupt cycle, and thus be writing for 3 consecutive cycles (and thus ignoring RDY).
 * On the fourth cycle, the DMC unit drives the next sample address onto the address lines, and reads that byte from memory.
 * It then drives RDY high again, and the CPU picks up where it left off.
 * This matters because on NTSC NES and Famicom, it can interfere with the expected operation of any register where reads have a side effect:
 * the controller registers ($4016 and $4017), reads of the PPU status register ($2002), and reads of VRAM/VROM data ($2007) if they happen to occur in the same cycle that the DMC unit pulls RDY low.
 * For the controller registers, this can cause an extra rising clock edge to occur, and thus shift an extra bit out.
 * For the others, the PPU will see multiple reads, which will cause extra increments of the address latches, or clear the vblank flag.
 * This problem has been fixed on the 2A07 and PAL NES is exempt of this bug.
 */


// -----------------------------------------------------------------------------
// DMC::load_sample_buffer
//
// Loads a newly fetched sample byte into the DMC sample buffer and marks the
// buffer as containing valid data.
//
// Parameters:
//   value - Sample byte fetched from CPU memory.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::load_sample_buffer(uint8_t value) {
	sample_buffer_       = value;
	sample_buffer_empty_ = false;
}


// -----------------------------------------------------------------------------
// DMC::set_enabled
//
// Enables or disables the DMC channel.
//
// Parameters:
//   value - true to enable the channel, false to disable it.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::set_enabled(bool value) {
	if (value) {
		enable();
	} else {
		disable();
	}
}


// -----------------------------------------------------------------------------
// DMC::enable
//
// Enables DMC sample playback.
//
// If no sample bytes remain, the current sample length is reloaded.  If the
// output unit is ready for a new cycle, sample playback is started immediately.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::enable()
{
	enabled_ = true;

	if (bytes_remaining_ == 0) {
		bytes_remaining_ = sample_length_;
	}

	if (output_clock()) {
		start_cycle();
	}
}


// -----------------------------------------------------------------------------
// DMC::disable
//
// Disables DMC sample playback by clearing the remaining sample-byte count.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::disable()
{
	enabled_ = false;
	bytes_remaining_ = 0;
}


// -----------------------------------------------------------------------------
// DMC::write_reg0
//
// Writes the DMC control register.
//
// Updates the playback frequency, resets the timer, and clears any pending DMC
// IRQ state when DMC interrupt generation is disabled.
//
// Parameters:
//   value - Value written to DMC register $4010.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::write_reg0(uint8_t value) {

	control_.value = value;

	timer_.frequency = frequency_table[control_.frequency];
	timer_.reset();

	if (!irq_enabled()) {
		nes::apu::status.dmc_irq = false;
		if (!nes::apu::status.irq_firing) {
			nes::cpu::clear_irq(nes::cpu::APU_IRQ);
		}
	}
}


// -----------------------------------------------------------------------------
// DMC::write_reg1
//
// Writes the DMC direct-load output register.
//
// Only the low seven bits are used for the DMC output level.
//
// Parameters:
//   value - Value written to DMC register $4011.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::write_reg1(uint8_t value) {
	output_ = value & 0x7f;
}


// -----------------------------------------------------------------------------
// DMC::write_reg2
//
// Writes the DMC sample-address register.
//
// The register value is converted into the corresponding CPU sample address,
// and the current sample pointer is reset to that address.
//
// Parameters:
//   value - Value written to DMC register $4012.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::write_reg2(uint8_t value) {
	sample_address_ = 0xc000 | (value << 6);
	sample_pointer_ = sample_address_;
}


// -----------------------------------------------------------------------------
// DMC::write_reg3
//
// Writes the DMC sample-length register.
//
// The encoded register value is converted into a byte count.  If no sample is
// currently active, the remaining-byte counter is initialized immediately.
//
// Parameters:
//   value - Value written to DMC register $4013.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::write_reg3(uint8_t value) {
	sample_length_ = (value << 4) | 1;
	if (bytes_remaining_ == 0) {
		bytes_remaining_ = sample_length_;
	}
}


// -----------------------------------------------------------------------------
// DMC::bytes_remaining
//
// Returns the number of sample bytes still remaining in the current DMC sample.
//
// Parameters:
//   None.
//
// Returns:
//   Number of sample bytes remaining.
// -----------------------------------------------------------------------------
uint16_t DMC::bytes_remaining() const {
	return bytes_remaining_;
}


// -----------------------------------------------------------------------------
// DMC::output_clock
//
// Advances the DMC output unit by one output bit.
//
// When active, the low bit of the shift register adjusts the output level up or
// down by two while remaining within the valid 0-127 range.  The shift register
// and bit counter are then advanced.
//
// Parameters:
//   None.
//
// Returns:
//   true when a new output cycle should be started.
// -----------------------------------------------------------------------------
bool DMC::output_clock() {
	if (bits_remaining_ != 0) {

		// 1. If the silence flag is clear, the output level changes based on bit 0 of the shift register.
		// If the bit is 1, add 2; otherwise, subtract 2.
		// But if adding or subtracting 2 would cause the output level to leave the 0-127 range, leave the output level unchanged.
		// This means subtract 2 only if the current level is at least 2, or add 2 only if the current level is at most 125.
		if (!muted_) {
			if (shift_register_.value()) {
				if (output_ <= 0x7d) {
					output_ += 2;
				}
			} else {
				if (output_ >= 0x02) {
					output_ -= 2;
				}
			}
		}

		// 2. The right shift register is clocked.
		shift_register_.clock();

		// 3. the bits-remaining counter is decremented. If it becomes zero, a new output cycle is started.
		--bits_remaining_;
		return bits_remaining_ == 0;
	}

	return true;
}


// -----------------------------------------------------------------------------
// DMC::start_cycle
//
// Begins a new DMC output cycle.
//
// Eight output bits are scheduled.  If a sample byte is available it is loaded
// into the shift register; otherwise the output unit enters its muted state.
// The sample buffer is then refilled from CPU memory when more data remains.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::start_cycle() {

	// immediate load the first byte if the shift counter is empty
	if (bytes_remaining_ != 0) {

		// 1. The bits-remaining counter is loaded with 8.
		bits_remaining_ = 8;

		// 2. If the sample buffer is empty, then the silence flag is set;
		// otherwise, the silence flag is cleared and the sample buffer is emptied into the shift register.
		if (sample_buffer_empty_) {
			muted_ = true;
		} else {
			muted_ = false;
			shift_register_.load(sample_buffer_);
			sample_buffer_empty_ = true;
		}

		// When the sample buffer is emptied, the memory reader fills the sample buffer with the next byte from the currently playing sample.
		// It has an address counter and a bytes remaining counter.
		refill_sample_buffer();
	}
}


// -----------------------------------------------------------------------------
// DMC::refill_sample_buffer
//
// Requests the next DMC sample byte from CPU memory.
//
// A DMC DMA transfer is scheduled for the current sample address.  The sample
// pointer and remaining-byte count are then advanced.  At the end of a sample,
// playback either loops or raises a DMC IRQ according to the control flags.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::refill_sample_buffer() {

	if (bytes_remaining_ != 0) {
		// Any time the sample buffer is in an empty state and bytes remaining is not zero
		// (including just after a write to $4015 that enables the channel, regardless of where that write occurs relative to the bit counter mentioned below), the following occur:

		// 1. The CPU is stalled for up to 4 CPU cycles to allow the longest possible write (the return address and write after an IRQ) to finish.
		// If OAM DMA is in progress, it is paused for two cycles.

		// 2. The sample buffer is filled with the next sample byte read from the current address,
		// subject to whatever mapping hardware is present.
		nes::cpu::schedule_dmc_dma([](uint8_t value) {
			nes::apu::dmc.load_sample_buffer(value);
		},
								   sample_pointer_, 1);

		// 3. The address is incremented; if it exceeds $FFFF, it is wrapped around to $8000.
		sample_pointer_ = ((sample_pointer_ + 1) & 0xffff) | 0x8000;

		// 4. The bytes remaining counter is decremented;
		// if it becomes zero and the loop flag is set, the sample is restarted (see above);
		// otherwise, if the bytes remaining counter becomes zero and the IRQ enabled flag is set, the interrupt flag is set.
		if (--bytes_remaining_ == 0) {
			if (loop()) {
				// When a sample is (re)started, the current address is set to the sample address, and bytes remaining is set to the sample length.
				bytes_remaining_ = sample_length_;
				sample_pointer_  = sample_address_;
			} else if (irq_enabled()) {
				nes::cpu::irq(nes::cpu::APU_IRQ);
				nes::apu::status.dmc_irq = true;
			}
		}
	}
}


// -----------------------------------------------------------------------------
// DMC::tick
//
// Advances the DMC timer by one APU clock.
//
// Whenever the timer expires, the output unit is clocked and a new output cycle
// is started when the current byte has been fully consumed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void DMC::tick() {

	timer_.tick([this]() {
		if (output_clock()) {
			start_cycle();
		}
	});
}


// -----------------------------------------------------------------------------
// DMC::output
//
// Returns the current DMC output level.
//
// A channel-level mute overrides the current DMC DAC value and produces zero
// output without altering the underlying playback state.
//
// Parameters:
//   None.
//
// Returns:
//   Current 7-bit DMC output level.
// -----------------------------------------------------------------------------
uint8_t DMC::output() const {
	
	if (channel_muted_) {
		return 0;
	}
	
	return output_ & 0x7f;
}


// -----------------------------------------------------------------------------
// DMC::irq_enabled
//
// Reports whether DMC sample-completion IRQ generation is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true when the DMC IRQ-enable flag is set.
// -----------------------------------------------------------------------------
bool DMC::irq_enabled() const {
	return control_.irq;
}


// -----------------------------------------------------------------------------
// DMC::loop
//
// Reports whether DMC sample looping is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true when the DMC loop flag is set.
// -----------------------------------------------------------------------------
bool DMC::loop() const {
	return control_.loop;
}

}

