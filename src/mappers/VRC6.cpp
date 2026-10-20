#include "VRC6.h"
#include "Apu.h"
#include "Cpu.h"


namespace {

constexpr double kVRC6NTSCClock = 1789773.0;

}

// -----------------------------------------------------------------------------
// VRC6::PulseChannel::write_control
//
// Programs one VRC6 pulse channel's volume, duty cycle, and constant-output
// mode.
//
// Bits 0-3 select the 4-bit volume. Bits 4-6 select the duty threshold.
// Bit 7 selects constant-output mode, which bypasses the duty sequencer.
//
// Parameters:
//   value - Value written to the pulse control register.
// -----------------------------------------------------------------------------
void
VRC6::PulseChannel::write_control (uint8_t value)
{
	volume = value & 0x0f;
	duty = (value >> 4) & 0x07;
	mode = (value & 0x80) != 0;
}


// -----------------------------------------------------------------------------
// VRC6::PulseChannel::write_period_low
//
// Writes the low eight bits of the VRC6 pulse timer period.
//
// Parameters:
//   value - Low timer-period byte.
// -----------------------------------------------------------------------------
void
VRC6::PulseChannel::write_period_low (uint8_t value)
{
	period = (period & 0x0f00) | static_cast<uint16_t>(value);
}


// -----------------------------------------------------------------------------
// VRC6::PulseChannel::write_period_high
//
// Writes the high four bits of the VRC6 pulse timer period and controls the
// channel enable state.
//
// Bit 7 enables the pulse channel. Bits 0-3 supply the high timer-period bits.
// Disabling the channel resets the waveform sequencer to its initial phase.
//
// Parameters:
//   value - Pulse enable and high timer-period value.
// -----------------------------------------------------------------------------
void
VRC6::PulseChannel::write_period_high (uint8_t value)
{
	period = (period & 0x00ff) | (static_cast<uint16_t>(value & 0x0f) << 8);
	enabled = (value & 0x80) != 0;

	if (!enabled) {
		step = 15;
	}
}


// -----------------------------------------------------------------------------
// VRC6::PulseChannel::clock
//
// Advances one VRC6 pulse channel by one CPU clock.
//
// The global VRC6 frequency-control register can halt the oscillator or reduce
// the effective timer period to produce 16x or 256x frequency scaling. When the
// timer expires, it reloads from the effective period and advances the 16-step
// duty sequencer.
//
// Parameters:
//   frequency_control - Current value of the VRC6 $9003 frequency-control
//                       register.
// -----------------------------------------------------------------------------
void
VRC6::PulseChannel::clock (uint8_t frequency_control)
{
	if (!enabled) {
		return;
	}

	/*
	 * Bit 0 halts all VRC6 oscillators in their current state.
	 */
	if (frequency_control & 0x01) {
		return;
	}

	uint16_t effective_period = period;

	/*
	 * Bit 2 selects 256x frequency and overrides the 16x setting.
	 * Bit 1 selects 16x frequency.
	 */
	if (frequency_control & 0x04) {
		effective_period >>= 8;
	} else if (frequency_control & 0x02) {
		effective_period >>= 4;
	}

	if (timer == 0) {
		timer = effective_period;

		/*
		 * The VRC6 pulse sequencer counts downward from 15 to 0.
		 */
		step = (step - 1) & 0x0f;
	} else {
		--timer;
	}
}


// -----------------------------------------------------------------------------
// VRC6::PulseChannel::output
//
// Returns the current 4-bit output level of one VRC6 pulse channel.
//
// Constant-output mode always returns the programmed volume while the channel
// is enabled. Otherwise the 16-step sequencer and duty threshold determine
// whether the programmed volume or silence is produced.
//
// Returns:
//   Current channel output in the range 0-15.
// -----------------------------------------------------------------------------
uint8_t
VRC6::PulseChannel::output() const
{
	if (!enabled) {
		return 0;
	}

	if (mode) {
		return volume;
	}

	return step <= duty ? volume : 0;
}


// -----------------------------------------------------------------------------
// VRC6::SawChannel::write_rate
//
// Programs the VRC6 sawtooth accumulator rate.
//
// Only the low six bits are used. This value is added to the internal
// accumulator six times during each 14-step sawtooth sequence.
//
// Parameters:
//   value - Value written to $B000.
// -----------------------------------------------------------------------------
void
VRC6::SawChannel::write_rate (uint8_t value)
{
	rate = value & 0x3f;
}


// -----------------------------------------------------------------------------
// VRC6::SawChannel::write_period_low
//
// Writes the low eight bits of the VRC6 sawtooth timer period.
//
// Parameters:
//   value - Low timer-period byte.
// -----------------------------------------------------------------------------
void
VRC6::SawChannel::write_period_low (uint8_t value)
{
	period = (period & 0x0f00) | static_cast<uint16_t>(value);
}


// -----------------------------------------------------------------------------
// VRC6::SawChannel::write_period_high
//
// Writes the high four bits of the VRC6 sawtooth timer period and controls
// channel enable state.
//
// Clearing the enable bit forces the accumulator to zero. The frequency
// divider itself is intentionally left untouched.
//
// Parameters:
//   value - Saw enable and high timer-period value.
// -----------------------------------------------------------------------------
void
VRC6::SawChannel::write_period_high(uint8_t value)
{
	period = (period & 0x00ff) | (static_cast<uint16_t>(value & 0x0f) << 8);
	enabled = (value & 0x80) != 0;

	if (!enabled) {
		accumulator = 0;
		step = 0;
	}
}


// -----------------------------------------------------------------------------
// VRC6::SawChannel::clock
//
// Advances the VRC6 sawtooth generator by one CPU clock.
//
// The global $9003 control can halt the oscillator or apply the same 16x/256x
// period scaling used by the pulse channels.
//
// The saw sequencer contains 14 divider steps. The accumulator is updated on
// every second step. Six additions occur before the accumulator is cleared at
// the end of the sequence.
//
// Parameters:
//   frequency_control - Current value of the VRC6 $9003 frequency-control
//                       register.
// -----------------------------------------------------------------------------
void
VRC6::SawChannel::clock(uint8_t frequency_control)
{
	if (!enabled) {
		return;
	}

	if (frequency_control & 0x01) {
		return;
	}

	uint16_t effective_period = period;

	if (frequency_control & 0x04) {
		effective_period >>= 8;
	} else if (frequency_control & 0x02) {
		effective_period >>= 4;
	}

	if (timer != 0) {
		--timer;
		return;
	}

	timer = effective_period;

	++step;

	if (step >= 14) {
		step = 0;
		accumulator = 0;
		return;
	}

	/*
	 * The accumulator reacts on every second divider clock.
	 *
	 * Steps 2, 4, 6, 8, 10, and 12 add the programmed rate.
	 */
	if ((step & 0x01) == 0) {
		accumulator = static_cast<uint8_t>(accumulator + rate);
	}
}


// -----------------------------------------------------------------------------
// VRC6::SawChannel::output
//
// Returns the current VRC6 sawtooth output level.
//
// The VRC6 exposes the upper five bits of its 8-bit accumulator.
//
// Returns:
//   Current sawtooth output in the range 0-31.
// -----------------------------------------------------------------------------
uint8_t
VRC6::SawChannel::output() const
{
	if (!enabled) {
		return 0;
	}

	return accumulator >> 3;
}


// -----------------------------------------------------------------------------
// VRC6::VRC6
//
// Initializes the VRC6 mapper core.
//
// The initial PRG mapping places the first 16 KB bank at $8000-$BFFF and the
// final 16 KB bank at $C000-$FFFF. CHR is initially mapped as one contiguous
// 8 KB region beginning at bank 0.
// -----------------------------------------------------------------------------
VRC6::VRC6()
{
	set_prg_89ab(0);
	set_prg_cdef(-1);

	set_chr_0000_1fff(0);
}


// -----------------------------------------------------------------------------
// VRC6::name
//
// Returns the generic mapper hardware name for the VRC6 core.
//
// Derived mapper implementations identify the specific VRC6 wiring variant,
// such as VRC6a or VRC6b.
//
// Returns:
//   Mapper hardware name.
// -----------------------------------------------------------------------------
std::string
VRC6::name() const
{
	return "VRC6";
}


// -----------------------------------------------------------------------------
// VRC6::reset
//
// Resets the VRC6 expansion-audio state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC6::reset()
{
	pulse1_ = {};
	pulse2_ = {};
	saw_ = {};

	frequency_control_ = 0;
}


// -----------------------------------------------------------------------------
// VRC6::read_6
//
// Reads the lower half of the VRC6 PRG-RAM window.
//
// When PRG RAM is disabled, the address behaves as open bus.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC6::read_6 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC6::read_7
//
// Reads the upper half of the VRC6 PRG-RAM window.
//
// When PRG RAM is disabled, the address behaves as open bus.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC6::read_7 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC6::write_6
//
// Writes the lower half of the VRC6 PRG-RAM window.
//
// Writes are ignored while PRG RAM is disabled.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//   value   - Byte value to write.
// -----------------------------------------------------------------------------
void
VRC6::write_6 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC6::write_7
//
// Writes the upper half of the VRC6 PRG-RAM window.
//
// Writes are ignored while PRG RAM is disabled.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//   value   - Byte value to write.
// -----------------------------------------------------------------------------
void
VRC6::write_7 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC6::write_8
//
// Handles writes to the VRC6 $8000 PRG bank register.
//
// The low four bits select the 16 KB PRG-ROM bank mapped at $8000-$BFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
// -----------------------------------------------------------------------------
void
VRC6::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0x8000:
	case 0x8001:
	case 0x8002:
	case 0x8003:
		set_prg_89ab(value & 0x0f);
		break;
	}
}

// -----------------------------------------------------------------------------
// VRC6::write_9
//
// Handles writes to the first VRC6 pulse channel and the global audio-frequency
// control register.
//
// $9000 controls pulse 1 volume, duty, and constant-output mode. $9001 and
// $9002 program its timer period and enable state. $9003 controls the global
// VRC6 audio frequency divider behavior.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the selected VRC6 register.
// -----------------------------------------------------------------------------
void
VRC6::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0x9000:
		pulse1_.write_control(value);
		nes::apu::log_external_audio_write(0x9000, value, pulse1_.period, false, frequency_control_);
		break;

	case 0x9001:
		pulse1_.write_period_low(value);
		nes::apu::log_external_audio_write(0x9001, value, pulse1_.period, true, frequency_control_);
		break;

	case 0x9002:
		pulse1_.write_period_high(value);
		nes::apu::log_external_audio_write(0x9002, value, pulse1_.period, true, frequency_control_);
		break;

	case 0x9003:
		frequency_control_ = value;
		nes::apu::log_external_audio_write(0x9003, value, 0, false, frequency_control_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_a
//
// Handles writes to the second VRC6 pulse channel.
//
// $A000 controls pulse 2 volume, duty, and constant-output mode. $A001 and
// $A002 program its timer period and enable state.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the selected VRC6 register.
// -----------------------------------------------------------------------------
void
VRC6::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xa000:
		pulse2_.write_control(value);
		nes::apu::log_external_audio_write(0xa000, value, pulse2_.period, false, frequency_control_);
		break;

	case 0xa001:
		pulse2_.write_period_low(value);
		nes::apu::log_external_audio_write(0xa001, value, pulse2_.period, true, frequency_control_);
		break;

	case 0xa002:
		pulse2_.write_period_high(value);
		nes::apu::log_external_audio_write(0xa002, value, pulse2_.period, true, frequency_control_);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_b
//
// Handles VRC6 sawtooth-audio and mapper-control register writes.
//
// $B000-$B002 program the sawtooth channel. $B003 controls nametable
// mirroring and enables or disables PRG RAM at $6000-$7FFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the selected register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC6::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xb000:
		saw_.write_rate(value);
		nes::apu::log_external_audio_write(0xb000, value, saw_.period, false, frequency_control_);
		break;

	case 0xb001:
		saw_.write_period_low(value);
		nes::apu::log_external_audio_write(0xb001, value, saw_.period, true, frequency_control_);
		break;

	case 0xb002:
		saw_.write_period_high(value);
		nes::apu::log_external_audio_write(0xb002, value, saw_.period, true, frequency_control_);
		break;

	case 0xb003:
		wram_enabled_ = (value & 0x80) != 0;
		debug_set_prg_ram(wram_enabled_, wram_enabled_, 0);

		switch ((value >> 2) & 0x03) {
		case 0x00:
			set_mirroring(mirror_vertical);
			break;

		case 0x01:
			set_mirroring(mirror_horizontal);
			break;

		case 0x02:
			set_mirroring(mirror_single_low);
			break;

		case 0x03:
			set_mirroring(mirror_single_high);
			break;
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_c
//
// Handles writes to the VRC6 $C000 PRG bank register.
//
// The low five bits select the 8 KB PRG-ROM bank mapped at $C000-$DFFF.
//
// Parameters:
//   address - CPU address being written.
//   value   - Value written to the PRG bank register.
// -----------------------------------------------------------------------------
void
VRC6::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xc000:
	case 0xc001:
	case 0xc002:
	case 0xc003:
		set_prg_cd(value & 0x1f);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_d
//
// Handles writes to the first four VRC6 CHR bank registers.
//
// The selected value maps one 1 KB CHR bank into the corresponding PPU range
// from $0000-$0FFF.
//
// Parameters:
//   address - CPU address selecting the CHR bank register.
//   value   - CHR bank number.
// -----------------------------------------------------------------------------
void
VRC6::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xd000:
		set_chr_0000_03ff(value);
		break;

	case 0xd001:
		set_chr_0400_07ff(value);
		break;

	case 0xd002:
		set_chr_0800_0bff(value);
		break;

	case 0xd003:
		set_chr_0c00_0fff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_e
//
// Handles writes to the final four VRC6 CHR bank registers.
//
// The selected value maps one 1 KB CHR bank into the corresponding PPU range
// from $1000-$1FFF.
//
// Parameters:
//   address - CPU address selecting the CHR bank register.
//   value   - CHR bank number.
// -----------------------------------------------------------------------------
void
VRC6::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xe000:
		set_chr_1000_13ff(value);
		break;

	case 0xe001:
		set_chr_1400_17ff(value);
		break;

	case 0xe002:
		set_chr_1800_1bff(value);
		break;

	case 0xe003:
		set_chr_1c00_1fff(value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::write_f
//
// Handles writes to the VRC6 IRQ registers.
//
// $F000 programs the IRQ latch. $F001 acknowledges any pending mapper IRQ,
// configures IRQ mode and enable state, and reloads the counter when enabled.
// $F002 acknowledges the IRQ and restores the enable state from the control
// register's acknowledge-enable bit.
//
// Parameters:
//   address - CPU address selecting the IRQ register.
//   value   - Value written to the selected IRQ register.
// -----------------------------------------------------------------------------
void
VRC6::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf003) {
	case 0xf000:
		irq_latch_ = value;
		break;

	case 0xf001:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.raw = value;

		if (irq_control_.enabled) {
			irq_counter_ = irq_latch_;
			irq_prescaler_ = 341;
		}
		break;

	case 0xf002:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);
		irq_control_.enabled = irq_control_.a;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC6::cpu_sync
//
// Advances the VRC6 audio generators and IRQ timing by one CPU cycle.
//
// Both pulse channels and the sawtooth generator use the global $9003
// frequency-control state. The IRQ unit operates independently in either
// CPU-cycle or scanline-derived mode.
// -----------------------------------------------------------------------------
void
VRC6::cpu_sync()
{
	pulse1_.clock(frequency_control_);
	pulse2_.clock(frequency_control_);
	saw_.clock(frequency_control_);

	if (irq_control_.enabled) {
		if (irq_control_.mode) {
			clock_irq();
		} else {
			irq_prescaler_ -= 3;

			if (irq_prescaler_ <= 0) {
				clock_irq();
				irq_prescaler_ += 341;
			}
		}
	}
}


// -----------------------------------------------------------------------------
// VRC6::clock_irq
//
// Advances the VRC6 IRQ counter by one mapper IRQ clock.
//
// A persistent debugger clock counter is incremented on every IRQ-counter clock.
// When the IRQ counter reaches $FF, it reloads from the programmed latch and
// asserts the mapper IRQ line. A second persistent counter records those IRQ
// assertions so debugger tools can show activity even when periodic UI sampling
// repeatedly observes the mapper at the same point in its frame cycle.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC6::clock_irq()
{
	++irq_clock_count_;

	if (irq_counter_ == 0xff) {
		irq_counter_ = irq_latch_;

		++irq_assert_count_;

		nes::cpu::irq(nes::cpu::MAPPER_IRQ);
	} else {
		++irq_counter_;
	}
}


// -----------------------------------------------------------------------------
// VRC6::audio_output
//
// Returns the current combined VRC6 expansion-audio output level.
//
// The two pulse channels and sawtooth channel feed a common linear output.
// Each pulse contributes a level from 0-15 and the sawtooth contributes a
// level from 0-31, giving a combined raw range of 0-61.
//
// Returns:
//   Current combined VRC6 expansion-audio level in the range 0-61.
// -----------------------------------------------------------------------------
uint8_t
VRC6::audio_output() const
{
	uint8_t output = 0;

	if (!audio_channel_muted_[0]) {
		output += pulse1_.output();
	}

	if (!audio_channel_muted_[1]) {
		output += pulse2_.output();
	}

	if (!audio_channel_muted_[2]) {
		output += saw_.output();
	}

	return output;
}


size_t
VRC6::audio_channel_count() const
{
	return 3;
}


const char*
VRC6::audio_channel_name (size_t channel) const
{
	switch (channel) {
		case 0:
			return "Pulse 1";

		case 1:
			return "Pulse 2";

		case 2:
			return "Saw";

		default:
			return nullptr;
	}
}


bool
VRC6::audio_channel_muted (size_t channel) const
{
	if (channel >= audio_channel_count()) {
		return false;
	}

	return audio_channel_muted_[channel];
}


void
VRC6::set_audio_channel_muted (size_t channel, bool muted)
{
	if (channel >= audio_channel_count()) {
		return;
	}

	audio_channel_muted_[channel] = muted;
}


// -----------------------------------------------------------------------------
// VRC6::debug_audio_scope_sample
//
// Returns the instantaneous output level of each VRC6 expansion-audio channel
// for oscilloscope capture.
//
// Channel order is stable and matches the VRC6 debugger channel order:
//
//   0 - Pulse 1
//   1 - Pulse 2
//   2 - Saw
//
// Parameters:
//   None.
//
// Returns:
//   Current per-channel VRC6 oscilloscope sample.
// -----------------------------------------------------------------------------
mapper_audio_scope_sample_t
VRC6::debug_audio_scope_sample() const
{
	mapper_audio_scope_sample_t sample;

	sample.channel_count = 3;

	sample.channels[0] = pulse1_.output();
	sample.channels[1] = pulse2_.output();
	sample.channels[2] = saw_.output();

	return sample;
}


// -----------------------------------------------------------------------------
// VRC6::debug_mapper_state
//
// Returns a debugger-facing snapshot of VRC6-specific mapper-control state.
//
// Resolved PRG/CHR mappings and nametable mirroring are intentionally omitted
// because they are already available through Mapper::debug_state(). This
// snapshot exposes internal VRC6 state that is otherwise not visible from the
// resolved mappings alone.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC6 mapper-specific debugger state.
// -----------------------------------------------------------------------------
vrc6_debug_state_t
VRC6::debug_mapper_state() const
{
	vrc6_debug_state_t state;

	state.wram_enabled = wram_enabled_;

	state.irq_latch = irq_latch_;
	state.irq_counter = irq_counter_;
	state.irq_prescaler = irq_prescaler_;

	state.irq_control = irq_control_.raw;
	state.irq_enable_after_ack = irq_control_.a;
	state.irq_enabled = irq_control_.enabled;
	state.irq_cycle_mode = irq_control_.mode;

	state.frequency_control = frequency_control_;
	state.audio_halted = (frequency_control_ & 0x01) != 0;

	if (frequency_control_ & 0x04) {
		state.audio_clock_scale = 256;
	} else if (frequency_control_ & 0x02) {
		state.audio_clock_scale = 16;
	} else {
		state.audio_clock_scale = 1;
	}
	
	state.irq_clock_count = irq_clock_count_;
	state.irq_assert_count = irq_assert_count_;

	return state;
}


// -----------------------------------------------------------------------------
// VRC6::debug_audio_state
//
// Returns debugger-facing state for the VRC6 expansion-audio channels.
//
// The snapshot includes mapper-wide frequency-control state from $9003,
// generic combined-output information, normalized output percentages,
// mapper-calculated channel frequencies, and the live state of both pulse
// channels and the sawtooth channel.
//
// Returns:
//   Current VRC6 expansion-audio debug state.
// -----------------------------------------------------------------------------
mapper_audio_debug_state_t
VRC6::debug_audio_state() const
{
	mapper_audio_debug_state_t state;

	state.type = ExpansionAudioType::VRC6;
	state.name = "VRC6";

	state.output = audio_output();

	state.output_maximum = 61;


	// -------------------------------------------------------------------------
	// Global VRC6 audio state
	// -------------------------------------------------------------------------

	state.field_count = 6;

	state.fields[0].label = "Frequency Control";
	state.fields[0].value = frequency_control_;
	state.fields[0].type = mapper_audio_debug_field_type::Hex;

	state.fields[1].label = "Halt";
	state.fields[1].value = (frequency_control_ & 0x01) != 0;
	state.fields[1].type = mapper_audio_debug_field_type::Boolean;

	state.fields[2].label = "16x Frequency";
	state.fields[2].value = (frequency_control_ & 0x02) != 0;
	state.fields[2].type = mapper_audio_debug_field_type::Timing;

	state.fields[3].label = "256x Frequency";
	state.fields[3].value = (frequency_control_ & 0x04) != 0;
	state.fields[3].type = mapper_audio_debug_field_type::Timing;

	state.fields[4].label = "Combined Output";
	state.fields[4].value = state.output;
	state.fields[4].type = mapper_audio_debug_field_type::Output;

	state.fields[5].label = "Output Level";
	state.fields[5].value = state.output;
	state.fields[5].maximum = state.output_maximum;
	state.fields[5].type = mapper_audio_debug_field_type::Percent;


	// -------------------------------------------------------------------------
	// Shared frequency-control state
	// -------------------------------------------------------------------------

	const bool halted = (frequency_control_ & 0x01) != 0;

	auto effective_period =
		[this](uint16_t period) -> uint16_t {
			if (frequency_control_ & 0x04) {
				return period >> 8;
			}

			if (frequency_control_ & 0x02) {
				return period >> 4;
			}

			return period;
		};


	// -------------------------------------------------------------------------
	// Pulse 1
	// -------------------------------------------------------------------------

	state.channel_count = 3;

	state.channels[0].name = "VRC6 Pulse 1";
	state.channels[0].period = pulse1_.period;
	state.channels[0].output = pulse1_.output();
	state.channels[0].output_maximum = 15;
	state.channels[0].enabled = pulse1_.enabled;
	state.channels[0].muted = audio_channel_muted_[0];
	state.channels[0].halted = halted;

	if (!halted) {
		const uint16_t period = effective_period(pulse1_.period);

		state.channels[0].frequency_hz = kVRC6NTSCClock / 
										 (16.0 * (static_cast<double>(period) + 1.0));

		state.channels[0].frequency_valid = true;
	}

	state.channels[0].field_count = 7;

	state.channels[0].fields[0].label = "Output Level";
	state.channels[0].fields[0].value = pulse1_.output();
	state.channels[0].fields[0].maximum = 15;
	state.channels[0].fields[0].type = mapper_audio_debug_field_type::Percent;

	state.channels[0].fields[1].label = "Volume";
	state.channels[0].fields[1].value = pulse1_.volume;

	state.channels[0].fields[2].label = "Duty";
	state.channels[0].fields[2].value = pulse1_.duty;

	state.channels[0].fields[3].label = "Duty Cycle";

	switch (pulse1_.duty) {
		case 0:
			state.channels[0].fields[3].text = "6.25%";
			break;

		case 1:
			state.channels[0].fields[3].text = "12.5%";
			break;

		case 2:
			state.channels[0].fields[3].text = "18.75%";
			break;

		case 3:
			state.channels[0].fields[3].text = "25.0%";
			break;

		case 4:
			state.channels[0].fields[3].text = "31.25%";
			break;

		case 5:
			state.channels[0].fields[3].text = "37.5%";
			break;

		case 6:
			state.channels[0].fields[3].text = "43.75%";
			break;

		default:
			state.channels[0].fields[3].text = "50.0%";
			break;
	}

	state.channels[0].fields[3].type = mapper_audio_debug_field_type::Text;
	state.channels[0].fields[4].label = "Mode";
	state.channels[0].fields[4].text = pulse1_.mode ? "Constant" : "Duty";
	state.channels[0].fields[4].type = mapper_audio_debug_field_type::Text;

	state.channels[0].fields[5].label = "Timer";
	state.channels[0].fields[5].value = pulse1_.timer;

	state.channels[0].fields[6].label = "Sequence Step";
	state.channels[0].fields[6].value = pulse1_.step;


	// -------------------------------------------------------------------------
	// Pulse 2
	// -------------------------------------------------------------------------

	state.channels[1].name = "VRC6 Pulse 2";
	state.channels[1].period = pulse2_.period;
	state.channels[1].output = pulse2_.output();
	state.channels[1].output_maximum = 15;
	state.channels[1].enabled = pulse2_.enabled;
	state.channels[1].muted = audio_channel_muted_[1];
	state.channels[1].halted = halted;

	if (!halted) {
		const uint16_t period = effective_period(pulse2_.period);

		state.channels[1].frequency_hz = kVRC6NTSCClock / 
									     (16.0 *(static_cast<double>(period) + 1.0));
		state.channels[1].frequency_valid = true;
	}

	state.channels[1].field_count = 7;

	state.channels[1].fields[0].label = "Output Level";
	state.channels[1].fields[0].value = pulse2_.output();
	state.channels[1].fields[0].maximum = 15;
	state.channels[1].fields[0].type = mapper_audio_debug_field_type::Percent;

	state.channels[1].fields[1].label = "Volume";
	state.channels[1].fields[1].value = pulse2_.volume;

	state.channels[1].fields[2].label = "Duty";
	state.channels[1].fields[2].value = pulse2_.duty;

	state.channels[1].fields[3].label = "Duty Cycle";

	switch (pulse2_.duty) {
		case 0:
			state.channels[1].fields[3].text = "6.25%";
			break;

		case 1:
			state.channels[1].fields[3].text = "12.5%";
			break;

		case 2:
			state.channels[1].fields[3].text = "18.75%";
			break;

		case 3:
			state.channels[1].fields[3].text = "25.0%";
			break;

		case 4:
			state.channels[1].fields[3].text = "31.25%";
			break;

		case 5:
			state.channels[1].fields[3].text = "37.5%";
			break;

		case 6:
			state.channels[1].fields[3].text = "43.75%";
			break;

		default:
			state.channels[1].fields[3].text = "50.0%";
			break;
	}

	state.channels[1].fields[3].type = mapper_audio_debug_field_type::Text;
	state.channels[1].fields[4].label = "Mode";
	state.channels[1].fields[4].text = pulse2_.mode ? "Constant" : "Duty";
	state.channels[1].fields[4].type = mapper_audio_debug_field_type::Text;

	state.channels[1].fields[5].label = "Timer";
	state.channels[1].fields[5].value = pulse2_.timer;

	state.channels[1].fields[6].label = "Sequence Step";
	state.channels[1].fields[6].value = pulse2_.step;


	// -------------------------------------------------------------------------
	// Saw
	// -------------------------------------------------------------------------

	state.channels[2].name = "VRC6 Saw";
	state.channels[2].period = saw_.period;
	state.channels[2].output = saw_.output();
	state.channels[2].output_maximum = 31;
	state.channels[2].enabled = saw_.enabled;
	state.channels[2].muted = audio_channel_muted_[2];
	state.channels[2].halted = halted;

	if (!halted) {
		const uint16_t period = effective_period(saw_.period);

		state.channels[2].frequency_hz = kVRC6NTSCClock / 
										 (14.0 * (static_cast<double>(period) + 1.0));
		state.channels[2].frequency_valid = true;
	}

	state.channels[2].field_count = 7;
	state.channels[2].fields[0].label = "Output Level";
	state.channels[2].fields[0].value = saw_.output();
	state.channels[2].fields[0].maximum = 31;
	state.channels[2].fields[0].type = mapper_audio_debug_field_type::Percent;

	state.channels[2].fields[1].label = "Rate";
	state.channels[2].fields[1].value = saw_.rate;

	state.channels[2].fields[2].label = "Adds / Sequence";
	state.channels[2].fields[2].value = 6;

	state.channels[2].fields[3].label = "Rate x 6";
	state.channels[2].fields[3].value = static_cast<uint32_t>(saw_.rate) * 6;

	state.channels[2].fields[4].label = "Timer";
	state.channels[2].fields[4].value = saw_.timer;

	state.channels[2].fields[5].label = "Accumulator";
	state.channels[2].fields[5].value = saw_.accumulator;

	state.channels[2].fields[6].label = "Step";
	state.channels[2].fields[6].value = saw_.step;	

	return state;
}

