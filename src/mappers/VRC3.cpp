
#include "VRC3.h"

#include "Cpu.h"


// -----------------------------------------------------------------------------
// VRC3::VRC3
//
// Initializes the VRC3 mapper core.
//
// The initial PRG mapping places the first 16 KB bank at $8000-$BFFF and the
// final 16 KB bank at $C000-$FFFF.  VRC3 provides a fixed 8 KB writable CHR RAM
// region mapped across the complete PPU pattern-table range.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VRC3::VRC3()
{
	set_prg_89ab(0);
	set_prg_cdef(-1);

	set_chr_0000_1fff_ram(chr_ram_, 0);
}


// -----------------------------------------------------------------------------
// VRC3::name
//
// Returns the mapper hardware name.
//
// Parameters:
//   None.
//
// Returns:
//   Generic mapper hardware name.
// -----------------------------------------------------------------------------
std::string
VRC3::name() const
{
	return "Konami VRC3";
}


// -----------------------------------------------------------------------------
// VRC3::read_6
//
// Reads VRC3 cartridge work RAM in the CPU $6000-$6FFF range.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Byte stored in VRC3 work RAM.
// -----------------------------------------------------------------------------
uint8_t
VRC3::read_6 (uint_least16_t address)
{
	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC3::read_7
//
// Reads VRC3 cartridge work RAM in the CPU $7000-$7FFF range.
//
// Parameters:
//   address - CPU address being read.
//
// Returns:
//   Byte stored in VRC3 work RAM.
// -----------------------------------------------------------------------------
uint8_t
VRC3::read_7 (uint_least16_t address)
{
	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC3::write_6
//
// Writes VRC3 cartridge work RAM in the CPU $6000-$6FFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to work RAM.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_6 (uint_least16_t address, uint8_t value)
{
	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC3::write_7
//
// Writes VRC3 cartridge work RAM in the CPU $7000-$7FFF range.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to work RAM.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_7 (uint_least16_t address, uint8_t value)
{
	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC3::write_8
//
// Handles writes to the first VRC3 IRQ latch register.
//
// The low four bits of the written value become bits 0-3 of the 16-bit IRQ
// latch.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value containing the IRQ latch nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_8 (uint_least16_t address, uint8_t value)
{
	(void)address;

	irq_latch_ = (irq_latch_ & 0xfff0) | static_cast<uint16_t>(value & 0x0f);
}


// -----------------------------------------------------------------------------
// VRC3::write_9
//
// Handles writes to the second VRC3 IRQ latch register.
//
// The low four bits of the written value become bits 4-7 of the 16-bit IRQ
// latch.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value containing the IRQ latch nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_9 (uint_least16_t address, uint8_t value)
{
	(void)address;

	irq_latch_ = (irq_latch_ & 0xff0f) | (static_cast<uint16_t>(value & 0x0f) << 4);
}


// -----------------------------------------------------------------------------
// VRC3::write_a
//
// Handles writes to the third VRC3 IRQ latch register.
//
// The low four bits of the written value become bits 8-11 of the 16-bit IRQ
// latch.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value containing the IRQ latch nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_a (uint_least16_t address, uint8_t value)
{
	(void)address;

	irq_latch_ = (irq_latch_ & 0xf0ff) | (static_cast<uint16_t>(value & 0x0f) << 8);
}


// -----------------------------------------------------------------------------
// VRC3::write_b
//
// Handles writes to the fourth VRC3 IRQ latch register.
//
// The low four bits of the written value become bits 12-15 of the 16-bit IRQ
// latch.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value containing the IRQ latch nibble.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_b (uint_least16_t address, uint8_t value)
{
	(void)address;

	irq_latch_ = (irq_latch_ & 0x0fff) | (static_cast<uint16_t>(value & 0x0f) << 12);
}


// -----------------------------------------------------------------------------
// VRC3::write_c
//
// Handles writes to the VRC3 IRQ control register.
//
// Any write acknowledges a pending mapper IRQ.  The control register contains
// the enable-after-acknowledge, enable, and counter-mode bits.  When IRQs are
// enabled by this write, the complete 16-bit counter is reloaded from the IRQ
// latch regardless of whether 8-bit or 16-bit counting mode is selected.
//
// Parameters:
//   address - CPU address being written.
//   value   - IRQ control value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_c (uint_least16_t address, uint8_t value)
{
	(void)address;

	nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

	irq_control_.raw = value;

	if (irq_control_.enabled) {
		irq_counter_ = irq_latch_;
	}
}


// -----------------------------------------------------------------------------
// VRC3::write_d
//
// Handles writes to the VRC3 IRQ acknowledge register.
//
// Any write clears the pending mapper IRQ and copies the control register's
// enable-after-acknowledge bit into the IRQ enable bit.  The current IRQ
// counter value is not modified.
//
// Parameters:
//   address - CPU address being written.
//   value   - Unused register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_d (uint_least16_t address, uint8_t value)
{
	(void)address;
	(void)value;

	nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

	irq_control_.enabled = irq_control_.a;
}


// -----------------------------------------------------------------------------
// VRC3::write_f
//
// Handles writes to the VRC3 PRG bank-select register.
//
// The low three bits select the 16 KB PRG ROM bank mapped at $8000-$BFFF.
// The final 16 KB PRG bank remains fixed at $C000-$FFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::write_f (uint_least16_t address, uint8_t value)
{
	(void)address;

	set_prg_89ab(value & 0x07);
}


// -----------------------------------------------------------------------------
// VRC3::cpu_sync
//
// Advances the VRC3 IRQ counter by one CPU cycle.
//
// Unlike VRC4/VRC6 IRQ hardware, VRC3 has no scanline prescaler.  When IRQs
// are enabled the counter advances directly once per CPU cycle.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::cpu_sync()
{
	if (irq_control_.enabled) {
		clock_irq();
	}
}


// -----------------------------------------------------------------------------
// VRC3::clock_irq
//
// Advances the VRC3 IRQ counter by one clock.
//
// In 16-bit mode, the complete counter increments until $FFFF overflows.  The
// counter is then reloaded from the complete 16-bit latch and a mapper IRQ is
// asserted.
//
// In 8-bit mode, only the low byte increments.  When it overflows from $FF,
// only the low byte is reloaded from the low byte of the IRQ latch; the high
// byte of the counter remains unchanged.
//
// Persistent debugger counters record IRQ-counter clocks and IRQ assertions so
// activity remains visible even when periodic debugger sampling misses the
// individual counter transitions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC3::clock_irq()
{
	++irq_clock_count_;

	if (irq_control_.mode) {
		const uint8_t counter =
			static_cast<uint8_t>(irq_counter_ & 0x00ff);

		if (counter == 0xff) {
			irq_counter_ =
				(irq_counter_ & 0xff00) |
				(irq_latch_ & 0x00ff);

			++irq_assert_count_;

			nes::cpu::irq(nes::cpu::MAPPER_IRQ);
		} else {
			irq_counter_ =
				(irq_counter_ & 0xff00) |
				static_cast<uint16_t>(counter + 1);
		}
	} else {
		if (irq_counter_ == 0xffff) {
			irq_counter_ = irq_latch_;

			++irq_assert_count_;

			nes::cpu::irq(nes::cpu::MAPPER_IRQ);
		} else {
			++irq_counter_;
		}
	}
}


// -----------------------------------------------------------------------------
// VRC3::debug_mapper_state
//
// Returns a debugger-facing snapshot of VRC3-specific mapper state.
//
// The snapshot exposes the IRQ latch, counter, control bits, counter mode, and
// persistent IRQ activity counters.  Resolved PRG and CHR mappings are handled
// separately by the generic mapper debugger state.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC3 mapper-specific debugger state.
// -----------------------------------------------------------------------------
vrc3_mapper_debug_state_t
VRC3::debug_mapper_state() const
{
	vrc3_mapper_debug_state_t state;

	state.irq_latch = irq_latch_;
	state.irq_counter = irq_counter_;

	state.irq_control = irq_control_.raw;
	state.irq_enable_after_ack = irq_control_.a;
	state.irq_enabled = irq_control_.enabled;
	state.irq_8bit_mode = irq_control_.mode;

	state.irq_clock_count = irq_clock_count_;
	state.irq_assert_count = irq_assert_count_;

	return state;
}

