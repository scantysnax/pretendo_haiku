
#include "Dmc.h"
#include "Apu.h"
#include "Cpu.h"
#include "Ppu.h"

#include <iostream>

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
// load_sample_buffer
//
// Completes a DMC DMA transfer by loading the fetched byte into the sample
// buffer and advancing the DMC memory reader state.
//
// A stale DMA completion may arrive after $4015 has disabled the DMC reader.
// In that case bytes_remaining_ is already zero, so the completion is ignored
// to prevent the byte counter from underflowing or the sample pointer from
// advancing incorrectly.
//
// When the final byte of a sample is consumed, looping restarts the sample
// reader; otherwise, if enabled, the DMC IRQ is asserted.
// -----------------------------------------------------------------------------
void
DMC::load_sample_buffer (uint8_t value)
{
	sample_dma_pending_ = false;

	/*
	 * A DMC DMA may already have begun when $4015 disables the
	 * memory reader.  In that case disable() has cleared
	 * bytes_remaining_ to zero, but the outstanding DMA may still
	 * complete.
	 *
	 * Do not let that stale completion underflow the remaining-byte
	 * counter or advance the sample reader.
	 */
	if (bytes_remaining_ == 0) {
		return;
	}

	sample_buffer_       = value;
	sample_buffer_empty_ = false;

	sample_pointer_ =
		((sample_pointer_ + 1) & 0xffff) | 0x8000;

	--bytes_remaining_;

	if (bytes_remaining_ == 0) {
		if (loop()) {
			bytes_remaining_ = sample_length_;
			sample_pointer_  = sample_address_;
		} else if (irq_enabled()) {
			nes::cpu::irq(nes::cpu::APU_IRQ);
			nes::apu::status.dmc_irq = true;
		}
	}
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
void
DMC::set_enabled (bool value)
{
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
// When the memory reader has no bytes remaining, playback restarts from the
// programmed sample address and length.  If the sample buffer is empty, the
// first sample byte is requested immediately.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
DMC::enable()
{
	enabled_ = true;

	if (bytes_remaining_ == 0) {
		sample_pointer_  = sample_address_;
		bytes_remaining_ = sample_length_;

		if (sample_buffer_empty_) {
			refill_sample_buffer(true);
		}
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
void
DMC::disable()
{
	enabled_ = false;
	bytes_remaining_ = 0;
}


// -----------------------------------------------------------------------------
// DMC::write_reg0
//
// Writes the DMC control register.
//
// Updates the playback timer period.  The current timer countdown is preserved.
// Disabling DMC IRQ generation also clears any pending DMC IRQ state.
//
// Parameters:
//   value - Value written to DMC register $4010.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
DMC::write_reg0 (uint8_t value)
{
	control_.value = value;

	timer_.frequency = frequency_table[control_.frequency];

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
void
DMC::write_reg1 (uint8_t value)
{
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
void
DMC::write_reg2 (uint8_t value)
{
	sample_address_ = 0xc000 | (value << 6);
	sample_pointer_ = sample_address_;
}


// -----------------------------------------------------------------------------
// DMC::write_reg3
//
// Writes the DMC sample-length register.
//
// The encoded register value is converted into the corresponding byte count.
// The active remaining-byte counter is not changed by this register write.
//
// Parameters:
//   value - Value written to DMC register $4013.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
DMC::write_reg3 (uint8_t value)
{
	sample_length_ = (value << 4) | 1;
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
uint16_t
DMC::bytes_remaining() const
{
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

bool
DMC::output_clock()
{
	if (bits_remaining_ != 0) {

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

		shift_register_.clock();

		--bits_remaining_;

		return bits_remaining_ == 0;
	}

	return true;
}


// -----------------------------------------------------------------------------
// DMC::start_cycle
//
// Begins a new eight-bit DMC output cycle.
//
// A buffered sample byte is transferred into the shift register even when the
// memory reader has already reached the end of the sample.  This allows the
// output unit to drain the final fetched byte independently of the memory
// reader's remaining-byte count.
//
// If the sample buffer becomes empty while additional sample bytes remain, the
// memory reader requests another byte.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
DMC::start_cycle()
{
	bits_remaining_ = 8;

	if (sample_buffer_empty_) {
		muted_ = true;
	} else {
		muted_ = false;
		shift_register_.load(sample_buffer_);
		sample_buffer_empty_ = true;
	}

	if (sample_buffer_empty_ && bytes_remaining_ != 0) {
		refill_sample_buffer(false);
	}
}


// -----------------------------------------------------------------------------
// refill_sample_buffer
//
// Requests a DMC DMA fetch when the sample buffer needs another byte.
//
// Only one DMC DMA request may be outstanding at a time.  If a fetch is
// already pending, no new request is scheduled even though the sample buffer
// may still be empty.
//
// The load_dma flag selects between the initial load-DMA timing and the normal
// reload-DMA timing used for subsequent sample bytes.
// -----------------------------------------------------------------------------
void
DMC::refill_sample_buffer (bool load_dma)
{
	/*
	 * The sample buffer may remain empty for several CPU cycles while
	 * its DMA fetch is pending.
	 *
	 * Do not schedule another fetch merely because the buffer is still
	 * empty.  Doing so would replace the CPU's existing DMC DMA request.
	 */
	if (sample_dma_pending_) {
		return;
	}

	if (bytes_remaining_ != 0) {
		sample_dma_pending_ = true;

		nes::cpu::schedule_dmc_dma(
			[](uint8_t value) {
				nes::apu::dmc.load_sample_buffer(value);
			},
			sample_pointer_,
			1,
			load_dma
				? nes::cpu::dmc_dma_type::load
				: nes::cpu::dmc_dma_type::reload
		);
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
void
DMC::tick()
{
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
uint8_t
DMC::output() const
{
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
bool
DMC::irq_enabled() const
{
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
bool
DMC::loop() const
{
	return control_.loop;
}


}
