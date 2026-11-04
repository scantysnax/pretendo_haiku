
#include "VRC7.h"

#include "Cart.h"
#include "Cpu.h"
#include "Nes.h"

#include <algorithm>
#include <cmath>


namespace {


// -----------------------------------------------------------------------------
// kVRC7InstrumentTable
//
// Contains the 15 fixed eight-byte instrument patches stored internally by
// the VRC7.
//
// Instrument 0 is not included here because it is the programmable custom
// instrument supplied through FM registers $00-$07.  Entries 0-14 therefore
// correspond to VRC7 instruments 1-15.
//
// Each eight-byte patch encodes the modulator and carrier operator parameters,
// including multiplier, tremolo, vibrato, sustain behavior, key scaling,
// output level, waveform selection, feedback, attack, decay, sustain level,
// and release.
//
// -----------------------------------------------------------------------------
const uint8_t kVRC7InstrumentTable[15][8] = {
	{ 0x03, 0x21, 0x04, 0x06, 0x8d, 0xf2, 0x42, 0x17 },
	{ 0x13, 0x41, 0x16, 0x0e, 0xfd, 0xf4, 0x23, 0x23 },
	{ 0x31, 0x11, 0x10, 0x0a, 0xf0, 0x9c, 0x32, 0x02 },
	{ 0x61, 0x61, 0x1b, 0x07, 0xaf, 0x63, 0x20, 0x28 },
	{ 0x22, 0x21, 0x1e, 0x06, 0xf0, 0x76, 0x08, 0x28 },
	{ 0x66, 0x21, 0x15, 0x00, 0x93, 0x94, 0x20, 0xf8 },
	{ 0x21, 0x61, 0x1c, 0x07, 0x82, 0x81, 0x16, 0x07 },
	{ 0x23, 0x21, 0x1a, 0x17, 0xef, 0x82, 0x25, 0x15 },
	{ 0x25, 0x11, 0x1f, 0x00, 0x54, 0x41, 0x20, 0x11 },
	{ 0x85, 0x01, 0x1f, 0x0f, 0xe4, 0xa2, 0x11, 0x12 },
	{ 0x07, 0x81, 0x2b, 0x05, 0xc1, 0xf4, 0x14, 0xf4 },
	{ 0x61, 0x23, 0x11, 0x06, 0x98, 0x96, 0x13, 0x16 },
	{ 0x11, 0x02, 0xd4, 0x05, 0xf2, 0xf2, 0x41, 0x51 },
	{ 0x61, 0x22, 0x0a, 0x01, 0x94, 0xa0, 0x44, 0x15 },
	{ 0x21, 0x61, 0x0b, 0x00, 0xa1, 0xa3, 0x44, 0x47 }
};


// -----------------------------------------------------------------------------
// kVRC7MultiplierTable
//
// Contains the VRC7 operator frequency multipliers.
//
// Values are stored at twice their actual multiplier so the 0.5 multiplier
// can be represented without floating-point arithmetic.  The corresponding
// real multiplier is therefore table[value] / 2.
//
// -----------------------------------------------------------------------------
const uint8_t kVRC7MultiplierTable[16] = {
	1,	// 0 = 0.5
	2,	// 1 = 1
	4,	// 2 = 2
	6,	// 3 = 3
	8,	// 4 = 4
	10,	// 5 = 5
	12,	// 6 = 6
	14,	// 7 = 7
	16,	// 8 = 8
	18,	// 9 = 9
	20,	// A = 10
	20,	// B = 10
	24,	// C = 12
	24,	// D = 12
	30,	// E = 15
	30	// F = 15
};


// -----------------------------------------------------------------------------
// kVRC7EnvelopeStepTable
//
// Defines the four fractional envelope stepping patterns used by the VRC7
// envelope-rate generator.
//
// Each row corresponds to the low two bits of the effective envelope rate.
// -----------------------------------------------------------------------------
const uint8_t kVRC7EnvelopeStepTable[4][8] = {
	{0, 1, 0, 1, 0, 1, 0, 1},
	{0, 1, 0, 1, 1, 1, 0, 1},
	{0, 1, 1, 1, 0, 1, 1, 1},
	{0, 1, 1, 1, 1, 1, 1, 1}
};

}


// -----------------------------------------------------------------------------
// VRC7::VRC7
//
// Initializes the VRC7 mapper core.
//
// The initial PRG mapping places banks 0, 1, and 2 in the three switchable
// 8 KB CPU regions and fixes the final PRG bank at $E000-$FFFF.
//
// PRG RAM begins disabled, and the generic mapper debugger state reflects the
// unmapped $6000-$7FFF region until the mapper enables WRAM.
//
// The initial CHR mapping exposes the first eight consecutive 1 KB banks.
// Cartridges containing CHR ROM use the normal CHR-ROM mapping, while
// cartridges such as Lagrange Point use the mapper's 8 KB CHR RAM.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VRC7::VRC7()
{
	prg_[0] = 0;
	prg_[1] = 1;
	prg_[2] = 2;

	for (int i = 0; i < 8; ++i) {
		chr_[i] = static_cast<uint8_t>(i);
	}

	set_prg_89(prg_[0]);
	set_prg_ab(prg_[1]);
	set_prg_cd(prg_[2]);
	set_prg_ef(-1);

	if (nes::cart.has_chr_rom()) {
		set_chr_0000_1fff(0);
	} else {
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}
	
	debug_set_prg_ram(false, false, 0);
}


// -----------------------------------------------------------------------------
// VRC7::name
//
// Returns the generic hardware name for the VRC7 mapper core.
//
// Parameters:
//   None.
//
// Returns:
//   Mapper hardware name.
// -----------------------------------------------------------------------------
std::string
VRC7::name() const
{
	return "Konami VRC7";
}


// -----------------------------------------------------------------------------
// VRC7::read_6
//
// Reads the lower half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC7::read_6 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC7::read_7
//
// Reads the upper half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//
// Returns:
//   PRG-RAM byte when enabled, otherwise simulated open-bus data.
// -----------------------------------------------------------------------------
uint8_t
VRC7::read_7 (uint_least16_t address)
{
	if (!wram_enabled_) {
		return static_cast<uint8_t>((address >> 8) & 0xff);
	}

	return wram_[address & 0x1fff];
}


// -----------------------------------------------------------------------------
// VRC7::write_6
//
// Writes the lower half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $6000-$6FFF range.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_6 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC7::write_7
//
// Writes the upper half of the VRC7 PRG-RAM window.
//
// Parameters:
//   address - CPU address in the $7000-$7FFF range.
//   value   - Byte value to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_7 (uint_least16_t address, uint8_t value)
{
	if (!wram_enabled_) {
		return;
	}

	wram_[address & 0x1fff] = value;
}


// -----------------------------------------------------------------------------
// VRC7::write_8
//
// Handles the first two VRC7 PRG bank registers.
//
// $8000 selects the 8 KB PRG bank mapped at $8000-$9FFF.
// $8010 selects the 8 KB PRG bank mapped at $A000-$BFFF.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - PRG bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_8 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0x8000:
		prg_[0] = value & 0x3f;
		set_prg_89(prg_[0]);
		break;

	case 0x8010:
		prg_[1] = value & 0x3f;
		set_prg_ab(prg_[1]);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_9
//
// Handles the third VRC7 PRG register and expansion-audio ports.
//
// $9000 selects the 8 KB PRG bank mapped at $C000-$DFFF.
// $9010 selects an internal VRC7 FM register.
// $9030 writes data to the selected FM register.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_9 (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0x9000:
		prg_[2] = value & 0x3f;
		set_prg_cd(prg_[2]);
		break;

	case 0x9010:
		audio_register_ = value;
		break;

	case 0x9030:
		write_audio_register(audio_register_, value);
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_a
//
// Handles the first two VRC7 1 KB CHR bank registers.
//
// Raw register values are retained for debugging.  CHR-RAM cartridges wrap
// the effective bank number to the available 8 KB RAM region.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_a (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xa000:
		chr_[0] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_0000_03ff(chr_[0]);
		} else {
			set_chr_0000_03ff_ram(chr_ram_, chr_[0] & 0x07);
		}
		break;

	case 0xa010:
		chr_[1] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_0400_07ff(chr_[1]);
		} else {
			set_chr_0400_07ff_ram(chr_ram_, chr_[1] & 0x07);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_b
//
// Handles the third and fourth VRC7 1 KB CHR bank registers.
//
// Raw register values are retained for debugging.  CHR-RAM cartridges wrap
// the effective bank number to the available 8 KB RAM region.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_b (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xb000:
		chr_[2] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_0800_0bff(chr_[2]);
		} else {
			set_chr_0800_0bff_ram(chr_ram_, chr_[2] & 0x07);
		}
		break;

	case 0xb010:
		chr_[3] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_0c00_0fff(chr_[3]);
		} else {
			set_chr_0c00_0fff_ram(chr_ram_, chr_[3] & 0x07);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_c
//
// Handles the fifth and sixth VRC7 1 KB CHR bank registers.
//
// Raw register values are retained for debugging.  CHR-RAM cartridges wrap
// the effective bank number to the available 8 KB RAM region.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_c (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xc000:
		chr_[4] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_1000_13ff(chr_[4]);
		} else {
			set_chr_1000_13ff_ram(chr_ram_, chr_[4] & 0x07);
		}
		break;

	case 0xc010:
		chr_[5] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_1400_17ff(chr_[5]);
		} else {
			set_chr_1400_17ff_ram(chr_ram_, chr_[5] & 0x07);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_d
//
// Handles the seventh and eighth VRC7 1 KB CHR bank registers.
//
// Raw register values are retained for debugging.  CHR-RAM cartridges wrap
// the effective bank number to the available 8 KB RAM region.
//
// Parameters:
//   address - CPU address selecting the CHR register.
//   value   - CHR bank number.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_d (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xd000:
		chr_[6] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_1800_1bff(chr_[6]);
		} else {
			set_chr_1800_1bff_ram(chr_ram_, chr_[6] & 0x07);
		}
		break;

	case 0xd010:
		chr_[7] = value;

		if (nes::cart.has_chr_rom()) {
			set_chr_1c00_1fff(chr_[7]);
		} else {
			set_chr_1c00_1fff_ram(chr_ram_, chr_[7] & 0x07);
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_e
//
// Handles the VRC7 mapper-control and IRQ-latch registers.
//
// $E000 controls nametable mirroring, expansion-audio silence state, and
// PRG-RAM access.  $E010 programs the IRQ latch.
//
// The generic mapper debugger state is updated whenever PRG-RAM access changes
// so the resolved $6000-$7FFF mapping remains accurate in Mapper Explorer.
//
// Parameters:
//   address - CPU address selecting the register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_e (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xe000:
		switch (value & 0x03) {
		case 0:
			set_mirroring(mirror_vertical);
			break;

		case 1:
			set_mirroring(mirror_horizontal);
			break;

		case 2:
			set_mirroring(mirror_single_low);
			break;

		case 3:
			set_mirroring(mirror_single_high);
			break;
		}

		audio_silenced_ = (value & 0x40) != 0;
		wram_enabled_ = (value & 0x80) != 0;

		debug_set_prg_ram(wram_enabled_, wram_enabled_,0);
		break;

	case 0xe010:
		irq_latch_ = value;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::write_f
//
// Handles the VRC7 IRQ control and acknowledge registers.
//
// Parameters:
//   address - CPU address selecting the IRQ register.
//   value   - Register value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_f (uint_least16_t address, uint8_t value)
{
	switch (address & 0xf030) {
	case 0xf000:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.raw = value;

		if (irq_control_.enabled) {
			irq_counter_ = irq_latch_;
			irq_prescaler_ = 341;
		}
		break;

	case 0xf010:
		nes::cpu::clear_irq(nes::cpu::MAPPER_IRQ);

		irq_control_.enabled = irq_control_.a;
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::cpu_sync
//
// Advances the VRC7 mapper state by one CPU cycle.
//
// The IRQ unit is clocked according to its configured mode.  The FM phase
// generator is advanced at approximately one update per 36 NTSC CPU cycles,
// corresponding to the VRC7/OPLL channel update rate.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::cpu_sync()
{
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

	--fm_clock_divider_;

	if (fm_clock_divider_ <= 0) {
		clock_fm_phase();
		fm_clock_divider_ += 36;
	}
}


// -----------------------------------------------------------------------------
// VRC7::clock_irq
//
// Advances the VRC7 IRQ counter by one clock.
//
// The persistent clock counter is incremented for every mapper IRQ clock.
// When the IRQ counter reaches $FF, the next clock reloads it from the
// programmed latch, records an IRQ assertion, and asserts the mapper IRQ line.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::clock_irq()
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
// VRC7::debug_mapper_state
//
// Returns a snapshot of the current VRC7 mapper-specific debugger state.
//
// The snapshot includes raw PRG and CHR registers, mapper-control state,
// selected FM register, decoded FM channel and instrument state, operator
// phase increments, phase accumulators, raw operator waveform outputs,
// envelope state and attenuation levels, IRQ registers, decoded IRQ control
// flags, prescaler, and persistent IRQ activity counters.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC7 debugger-state snapshot.
// -----------------------------------------------------------------------------
vrc7_debug_state_t
VRC7::debug_mapper_state() const
{
	vrc7_debug_state_t state;

	for (int i = 0; i < 3; ++i) {
		state.prg[i] = prg_[i];
	}

	for (int i = 0; i < 8; ++i) {
		state.chr[i] = chr_[i];
	}

	state.wram_enabled = wram_enabled_;
	state.audio_silenced = audio_silenced_;
	state.audio_register = audio_register_;

	for (int i = 0; i < 6; ++i) {
		state.fm_channels[i].f_number = fm_channels_[i].f_number;
		state.fm_channels[i].block = fm_channels_[i].block;
		state.fm_channels[i].instrument = fm_channels_[i].instrument;
		state.fm_channels[i].volume = fm_channels_[i].volume;
		state.fm_channels[i].key_on = fm_channels_[i].key_on;
		state.fm_channels[i].sustain = fm_channels_[i].sustain;
		state.fm_channels[i].modulator_multiplier = fm_channels_[i].patch.modulator_multiplier;
		state.fm_channels[i].carrier_multiplier = fm_channels_[i].patch.carrier_multiplier;
		state.fm_channels[i].feedback = fm_channels_[i].patch.feedback;
		state.fm_channels[i].modulator_output_level = fm_channels_[i].patch.modulator_output_level;
		state.fm_channels[i].modulator_attack = fm_channels_[i].patch.modulator_attack;
		state.fm_channels[i].modulator_decay = fm_channels_[i].patch.modulator_decay;
		state.fm_channels[i].carrier_attack = fm_channels_[i].patch.carrier_attack;
		state.fm_channels[i].carrier_decay = fm_channels_[i].patch.carrier_decay;
		state.fm_channels[i].modulator_phase_increment = fm_channels_[i].modulator.phase_increment;
		state.fm_channels[i].carrier_phase_increment = fm_channels_[i].carrier.phase_increment;
		state.fm_channels[i].modulator_phase = fm_channels_[i].modulator.phase;
		state.fm_channels[i].carrier_phase = fm_channels_[i].carrier.phase;
		state.fm_channels[i].modulator_output = fm_channels_[i].modulator.output;
		state.fm_channels[i].carrier_output = fm_channels_[i].carrier.output;
		state.fm_channels[i].modulator_envelope_state = static_cast<uint8_t>(fm_channels_[i].modulator.envelope_state);
		state.fm_channels[i].carrier_envelope_state = static_cast<uint8_t>(fm_channels_[i].carrier.envelope_state);
		state.fm_channels[i].modulator_envelope_level = fm_channels_[i].modulator.envelope_level;
		state.fm_channels[i].carrier_envelope_level = fm_channels_[i].carrier.envelope_level;
	}

	state.irq_latch = irq_latch_;
	state.irq_counter = irq_counter_;
	state.irq_control = irq_control_.raw;

	state.irq_enable_after_ack = irq_control_.a;
	state.irq_enabled = irq_control_.enabled;
	state.irq_cycle_mode = irq_control_.mode;

	state.irq_prescaler = irq_prescaler_;

	state.irq_clock_count = irq_clock_count_;
	state.irq_assert_count = irq_assert_count_;
	
	state.fm_output = fm_output_;

	return state;
}


// -----------------------------------------------------------------------------
// VRC7::write_audio_register
//
// Writes a value to an internal VRC7 FM register.
//
// Raw register data is retained for debugging and later synthesis.  Registers
// $00-$07 define the programmable custom instrument patch.  Registers $10-$15
// contain the low eight bits of the F-number for FM channels 0-5.  Registers
// $20-$25 contain the F-number high bit, octave block, key-on state, and
// sustain flag.  Registers $30-$35 select the instrument and output volume.
//
// Whenever pitch or instrument parameters change, the corresponding operator
// phase increments are recalculated.  Key-on/key-off changes are handled as
// envelope-state transitions rather than by directly assigning the key flag.
//
// Parameters:
//   reg   - Internal VRC7 FM register number.
//   value - Value written to the selected register.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::write_audio_register (uint8_t reg, uint8_t value)
{
	reg &= 0x3f;

	fm_registers_[reg] = value;

	/*
	 * Custom instrument registers.
	 */
	if (reg <= 0x07) {
		custom_instrument_[reg] = value;

		for (int i = 0; i < 6; ++i) {
			if (fm_channels_[i].instrument == 0) {
				decode_instrument(fm_channels_[i]);
				update_phase_increments(fm_channels_[i]);
			}
		}

		return;
	}

	/*
	 * F-number low byte.
	 */
	if (reg >= 0x10 && reg <= 0x15) {
		const int channel = reg - 0x10;

		fm_channels_[channel].f_number = (fm_channels_[channel].f_number & 0x0100) | value;
		update_phase_increments(fm_channels_[channel]);

		return;
	}

	/*
	 * F-number high bit, octave block, key-on, and sustain.
	 */
	if (reg >= 0x20 && reg <= 0x25) {
		const int channel = reg - 0x20;

		fm_channels_[channel].f_number = (fm_channels_[channel].f_number & 0x00ff) | 
										 ((static_cast<uint16_t>(value) & 0x01) << 8);
		fm_channels_[channel].block = (value >> 1) & 0x07;
		const bool keyOn = (value & 0x10) != 0;

		update_key_state(fm_channels_[channel], keyOn);

		fm_channels_[channel].sustain = (value & 0x20) != 0;

		update_phase_increments(fm_channels_[channel]);

		return;
	}

	/*
	 * Instrument and output volume.
	 */
	if (reg >= 0x30 && reg <= 0x35) {
		const int channel = reg - 0x30;

		fm_channels_[channel].instrument = (value >> 4) & 0x0f;
		fm_channels_[channel].volume = value & 0x0f;

		decode_instrument(fm_channels_[channel]);
		update_phase_increments(fm_channels_[channel]);

		return;
	}
}


// -----------------------------------------------------------------------------
// VRC7::decode_instrument
//
// Decodes the currently selected VRC7 instrument into per-channel operator
// parameters.
//
// Instrument 0 uses the eight-byte programmable custom patch.  Instruments
// 1-15 use the fixed instrument data stored internally by the VRC7.
//
// Parameters:
//   channel - FM channel whose instrument state is to be decoded.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::decode_instrument (fm_channel_t &channel)
{
	const uint8_t *patch;

	if (channel.instrument == 0) {
		patch = custom_instrument_;
	} else {
		patch = kVRC7InstrumentTable[channel.instrument - 1];
	}

	// Modulator control.
	channel.patch.modulator_tremolo = (patch[0] & 0x80) != 0;
	channel.patch.modulator_vibrato = (patch[0] & 0x40) != 0;
	channel.patch.modulator_sustain = (patch[0] & 0x20) != 0;
	channel.patch.modulator_key_scale_rate = (patch[0] & 0x10) != 0;
	channel.patch.modulator_multiplier = patch[0] & 0x0f;

	// Carrier control.
	channel.patch.carrier_tremolo = (patch[1] & 0x80) != 0;
	channel.patch.carrier_vibrato = (patch[1] & 0x40) != 0;
	channel.patch.carrier_sustain = (patch[1] & 0x20) != 0;
	channel.patch.carrier_key_scale_rate = (patch[1] & 0x10) != 0;
	channel.patch.carrier_multiplier = patch[1] & 0x0f;

	// Modulator level control.
	channel.patch.modulator_key_scale_level = (patch[2] >> 6) & 0x03;
	channel.patch.modulator_output_level = patch[2] & 0x3f;

	// Carrier level, waveform, and feedback control.
	channel.patch.carrier_key_scale_level = (patch[3] >> 6) & 0x03;
	channel.patch.carrier_waveform = (patch[3] & 0x10) != 0;
	channel.patch.modulator_waveform = (patch[3] & 0x08) != 0;
	channel.patch.feedback = patch[3] & 0x07;

	// Modulator attack and decay.
	channel.patch.modulator_attack = (patch[4] >> 4) & 0x0f;
	channel.patch.modulator_decay = patch[4] & 0x0f;

	// Carrier attack and decay.
	channel.patch.carrier_attack = (patch[5] >> 4) & 0x0f;
	channel.patch.carrier_decay = patch[5] & 0x0f;

	// Modulator sustain level and release.
	channel.patch.modulator_sustain_level = (patch[6] >> 4) & 0x0f;
	channel.patch.modulator_release = patch[6] & 0x0f;

	// Carrier sustain level and release.
	channel.patch.carrier_sustain_level = (patch[7] >> 4) & 0x0f;
	channel.patch.carrier_release = patch[7] & 0x0f;
}


// -----------------------------------------------------------------------------
// VRC7::update_phase_increments
//
// Recalculates the modulator and carrier phase increments for an FM channel.
//
// The increment is derived from the channel F-number, octave block, and the
// operator multiplier selected by the decoded instrument patch.
//
// The result is the internal fixed-point phase step used by the FM oscillator
// during each VRC7 synthesis update.
//
// Parameters:
//   channel - FM channel whose operator phase increments are to be updated.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::update_phase_increments (fm_channel_t &channel)
{
	const uint32_t base = static_cast<uint32_t>(channel.f_number) << channel.block;
	const uint32_t modMultiplier = kVRC7MultiplierTable[channel.patch.modulator_multiplier & 0x0f];
	const uint32_t carrierMultiplier = kVRC7MultiplierTable[channel.patch.carrier_multiplier & 0x0f];

	channel.modulator.phase_increment = (base * modMultiplier) >> 1;
	channel.carrier.phase_increment = (base * carrierMultiplier) >> 1;
}


// -----------------------------------------------------------------------------
// VRC7::clock_fm_phase
//
// Advances the VRC7 FM synthesis state for all six channels.
//
// Each FM update advances the shared envelope timing counter, clocks the
// modulator and carrier envelope generators, advances both 19-bit operator
// phase accumulators, applies modulator feedback, generates and attenuates the
// modulator, uses the modulator output to phase-modulate the carrier, then
// generates and attenuates the carrier.
//
// After all six channels have been processed, their carrier outputs are summed
// into the current composite VRC7 FM output.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::clock_fm_phase()
{
	static constexpr uint32_t kPhaseMask =
		(1u << 19) - 1u;

	/*
	 * Advance the shared envelope timing counter.
	 */
	++envelope_counter_;

	for (int i = 0; i < 6; ++i) {
		fm_channel_t &channel = fm_channels_[i];

		/*
		 * Advance the operator envelope generators.
		 */
		clock_envelope(channel, channel.modulator, false);
		clock_envelope(channel, channel.carrier, true);

		/*
		 * Calculate modulator self-feedback from the previous two
		 * modulator output samples.
		 */
		const int32_t feedbackOffset = feedback_phase_offset(channel);

		/*
		 * Advance the base operator phase accumulators.
		 */
		channel.modulator.phase = (channel.modulator.phase + channel.modulator.phase_increment) & kPhaseMask;
		channel.carrier.phase = (channel.carrier.phase + channel.carrier.phase_increment) & kPhaseMask;

		/*
		 * Apply feedback to the modulator phase.
		 */
		const int64_t modulatorPhaseWithFeedback = static_cast<int64_t>( channel.modulator.phase) + 
												   static_cast<int64_t>(feedbackOffset) * 256;
		const uint32_t modulatorPhase = static_cast<uint32_t>(modulatorPhaseWithFeedback) & kPhaseMask;

		/*
		 * Generate the modulator waveform.
		 */
		const int16_t modulatorWaveform = waveform_sample(modulatorPhase, channel.patch.modulator_waveform);

		/*
		 * Apply the modulator envelope and instrument total level.
		 */
		channel.modulator.output = apply_envelope(modulatorWaveform, combined_attenuation(channel,
												  channel.modulator, false));
		/*
		 * Rotate the modulator feedback history after calculating
		 * the current output.
		 */
		channel.modulator.feedback_output_1 = channel.modulator.feedback_output_0;
		channel.modulator.feedback_output_0 = channel.modulator.output;

		/*
		 * Use the current modulator output to phase-modulate the
		 * carrier.
		 */
		const int32_t carrierOffset = carrier_phase_offset(channel);
		const int64_t carrierPhaseWithModulation = static_cast<int64_t>(channel.carrier.phase) + 
												   static_cast<int64_t>(carrierOffset) * 256;
		const uint32_t carrierPhase = static_cast<uint32_t>(carrierPhaseWithModulation) & kPhaseMask;

		/*
		 * Generate the carrier waveform.
		 */
		const int16_t carrierWaveform = waveform_sample(carrierPhase,channel.patch.carrier_waveform);

		/*
		 * Apply the carrier envelope and channel volume.
		 */
		channel.carrier.output = apply_envelope(carrierWaveform, combined_attenuation(channel,
												channel.carrier, true));
	}

	/*
	 * All six FM channels have now completed this synthesis update.
	 * Sum their carrier outputs into the composite VRC7 output.
	 */
	update_fm_output();
}


// -----------------------------------------------------------------------------
// VRC7::waveform_sample
//
// Converts a VRC7 operator phase accumulator into a raw waveform sample.
//
// The 19-bit dynamic phase accumulator is reduced to the 10-bit waveform
// position used by the VRC7/OPLL sine generator.  Waveform 0 is a full sine
// wave.  Waveform 1 retains the first half of the sine wave and silences the
// second half.
//
// This is currently a linear sine approximation used to establish the
// oscillator path.  Envelope attenuation, logarithmic amplitude conversion,
// feedback, and FM modulation are applied separately in later stages.
//
// Parameters:
//   phase     - Current 19-bit operator phase accumulator.
//   half_sine - true for the alternate half-sine waveform.
//
// Returns:
//   Raw signed waveform sample in the approximate range -4095 to 4095.
// -----------------------------------------------------------------------------
int16_t
VRC7::waveform_sample (uint32_t phase, bool half_sine) const
{
	static constexpr uint32_t kPhaseBits = 19;
	static constexpr uint32_t kWaveBits = 10;

	static constexpr uint32_t kWaveSize = 1u << kWaveBits;
	static constexpr uint32_t kWaveMask = kWaveSize - 1u;
	static constexpr uint32_t kPhaseShift = kPhaseBits - kWaveBits;

	static constexpr double kTwoPi = 6.28318530717958647692;

	const uint32_t index = (phase >> kPhaseShift) & kWaveMask;

	if (half_sine && index >= (kWaveSize / 2)) {
		return 0;
	}

	const double angle = kTwoPi * static_cast<double>(index) / static_cast<double>(kWaveSize);

	return static_cast<int16_t>(std::sin(angle) * 4095.0);
}


// -----------------------------------------------------------------------------
// VRC7::update_key_state
//
// Updates the key state for a VRC7 FM channel and starts the appropriate
// envelope transition for both operators.
//
// A rising key transition enters the damping stage.  The damping stage will
// quickly silence the previous note before restarting the operator phases and
// beginning attack.  The modulator feedback history is also cleared so the new
// note does not inherit feedback from the previous one.
//
// A falling key transition starts release for the carrier.  The modulator also
// enters release unless its instrument sustain flag is set, in which case it
// continues sustaining while the carrier releases.
//
// Parameters:
//   channel - FM channel whose key state is being updated.
//   keyOn   - New key-on state.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::update_key_state (fm_channel_t &channel, bool keyOn)
{
	if (channel.key_on == keyOn) {
		return;
	}

	channel.key_on = keyOn;

	if (keyOn) {
		channel.modulator.envelope_counter = 0;
		channel.carrier.envelope_counter = 0;

		channel.modulator.feedback_output_0 = 0;
		channel.modulator.feedback_output_1 = 0;

		channel.modulator.envelope_state = FMEnvelopeState::Damp;
		channel.carrier.envelope_state = FMEnvelopeState::Damp;

		channel.modulator.key_on = true;
		channel.carrier.key_on = true;

		return;
	}

	channel.modulator.envelope_counter = 0;
	channel.carrier.envelope_counter = 0;

	if (!channel.patch.modulator_sustain && channel.modulator.envelope_state != FMEnvelopeState::Idle) {
		channel.modulator.envelope_state = FMEnvelopeState::Release;
	}

	if (channel.carrier.envelope_state != FMEnvelopeState::Idle) {
		channel.carrier.envelope_state = FMEnvelopeState::Release;
	}

	channel.modulator.key_on = false;
	channel.carrier.key_on = false;
}


// -----------------------------------------------------------------------------
// VRC7::clock_envelope
//
// Advances one VRC7 operator envelope by one FM update.
//
// The operator's effective envelope rate is derived from the current envelope
// stage, instrument parameters, key-rate scaling, and the shared envelope
// timing counter.
//
// Attack uses the OPLL exponential-style attenuation reduction.  Damp, decay,
// sustain, and release use the hardware decay-step patterns.
//
// Envelope level is attenuation:
//   0   = maximum output
//   123 = maximum audible attenuation
//   127 = fully muted
//
// Parameters:
//   channel - FM channel containing the operator.
//   op      - Operator whose envelope is being advanced.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::clock_envelope (fm_channel_t &channel, fm_operator_t &op, bool carrier)
{
	static constexpr uint8_t kEnvelopeMaximum = 123;
	static constexpr uint8_t kEnvelopeMute = 127;

	const fm_instrument_t &patch = channel.patch;

	const uint8_t sustainLevel = static_cast<uint8_t>((carrier ? patch.carrier_sustain_level
															   : patch.modulator_sustain_level) << 3);
	const uint8_t rateHigh = envelope_rate_high(channel, op, carrier);
	const uint8_t rateLow = envelope_rate_low(channel, op, carrier);
	uint8_t shift = 0;

	if (op.envelope_state == FMEnvelopeState::Attack) {
		if (rateHigh > 0 && rateHigh < 12) {
			shift = static_cast<uint8_t>(13 - rateHigh);
		}
	} else {
		if (rateHigh < 13) {
			shift = static_cast<uint8_t>(13 - rateHigh);
		}
	}

	const uint32_t mask = (1u << shift) - 1u;

	/*
	 * Attack uses a nonlinear reduction in attenuation.
	 */
	if (op.envelope_state == FMEnvelopeState::Attack) {
		if (op.envelope_level > 0 && rateHigh > 0 && (envelope_counter_ & mask & ~3u) == 0) {
			uint8_t step = 0;

			switch (rateHigh) {
			case 12:
				{
					const uint32_t index = (envelope_counter_ & 0x0c) >> 1;
					step = static_cast<uint8_t>(4 - kVRC7EnvelopeStepTable[rateLow][index]);
				}
				break;

			case 13:
				{
					const uint32_t index = (envelope_counter_ & 0x0c) >> 1;
					step = static_cast<uint8_t>(3 - kVRC7EnvelopeStepTable[rateLow][index]);
				}
				break;

			case 14:
				{
					const uint32_t index = (envelope_counter_ & 0x0c) >> 1;
					step = static_cast<uint8_t>(2 - kVRC7EnvelopeStepTable[rateLow][index]);
				}
				break;

			case 0:
			case 15:
				step = 0;
				break;

			default:
				{
					const uint32_t index = (envelope_counter_ >> shift) & 0x07;
					step = kVRC7EnvelopeStepTable[rateLow][index] ? 4 : 0;
				}
				break;
			}

			if (step > 0) {
				int level = static_cast<int>(op.envelope_level);
				level -= (level >> step) + 1;

				if (level < 0) {
					level = 0;
				}

				op.envelope_level = static_cast<uint8_t>(level);
			}
		}
	} else if (rateHigh > 0 && (envelope_counter_ & mask) == 0) {
		uint8_t step = 0;

		switch (rateHigh) {
		case 13:
			{
				const uint32_t index = ((envelope_counter_ & 0x0c) >> 1) | (envelope_counter_ & 0x01);
				step = kVRC7EnvelopeStepTable[rateLow][index];
			}
			break;

		case 14:
			{
				const uint32_t index = (envelope_counter_ & 0x0c) >> 1;
				step = static_cast<uint8_t>(kVRC7EnvelopeStepTable[rateLow][index]+ 1);
			}
			break;

		case 15:
			step = 2;
			break;

		default:
			{
				const uint32_t index = (envelope_counter_ >> shift) & 0x07;
				step = kVRC7EnvelopeStepTable[rateLow][index];
			}
			break;
		}

		const unsigned int level = static_cast<unsigned int>(op.envelope_level) + step;
		op.envelope_level = static_cast<uint8_t>(std::min<unsigned int>(kEnvelopeMute, level));
	}


	/*
	 * Handle envelope-stage transitions.
	 */
	switch (op.envelope_state) {
	case FMEnvelopeState::Idle:
		op.envelope_level = kEnvelopeMute;
		break;

	case FMEnvelopeState::Damp:
		if (op.envelope_level >= kEnvelopeMaximum && (envelope_counter_ & mask) == 0) {
			const uint8_t attackRate = carrier ? patch.carrier_attack : patch.modulator_attack;
			const uint8_t keyScale = envelope_key_scale(channel, carrier);
			const uint8_t effectiveAttack = std::min<uint8_t>(15, static_cast<uint8_t>(attackRate+ (keyScale >> 2)));
			
			op.phase = 0;
			op.envelope_counter = 0;

			if (effectiveAttack == 15) {
				op.envelope_level = 0;
				op.envelope_state = FMEnvelopeState::Decay;
			} else {
				op.envelope_state = FMEnvelopeState::Attack;
			}
		}
		break;

	case FMEnvelopeState::Attack:
		if (op.envelope_level == 0) {
			op.envelope_counter = 0;
			op.envelope_state = FMEnvelopeState::Decay;
		}
		break;

	case FMEnvelopeState::Decay:
		/*
		 * The hardware checks this transition continuously rather
		 * than only when the attenuation value advances.
		 */
		if ((op.envelope_level >> 3) == (sustainLevel >> 3)) {

			op.envelope_counter = 0;
			op.envelope_state = FMEnvelopeState::Sustain;
		}
		break;

	case FMEnvelopeState::Sustain:
		break;

	case FMEnvelopeState::Release:
		if (op.envelope_level >= kEnvelopeMute) {
			op.envelope_level = kEnvelopeMute;
			op.envelope_counter = 0;
			op.envelope_state = FMEnvelopeState::Idle;
		}
		break;
	}
}


// -----------------------------------------------------------------------------
// VRC7::envelope_key_scale
//
// Calculates the envelope key-rate-scaling value for one VRC7 operator.
//
// When key-rate scaling is enabled, the effective value is formed from the
// channel block and the high F-number bit.  When disabled, only a reduced
// block contribution is used.
//
// Parameters:
//   channel - FM channel containing the operator.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Envelope key-rate-scaling value.
// -----------------------------------------------------------------------------
uint8_t
VRC7::envelope_key_scale (const fm_channel_t &channel, bool carrier) const
{
	const bool keyScaleRate = carrier ? channel.patch.carrier_key_scale_rate : channel.patch.modulator_key_scale_rate;
	const uint8_t fNumberHigh = static_cast<uint8_t>((channel.f_number >> 8) & 0x01);

	if (keyScaleRate) {
		return static_cast<uint8_t>((channel.block << 1) | fNumberHigh);
	}

	return static_cast<uint8_t>(channel.block >> 1);
}


// -----------------------------------------------------------------------------
// VRC7::envelope_parameter_rate
//
// Returns the base envelope rate for the current operator state.
//
// Attack and decay use the corresponding instrument parameters.  Sustain
// either stops envelope movement or continues at the release rate depending
// on the patch sustain flag.  Release uses the VRC7 channel sustain override
// when active.  Damping uses the fixed hardware damping rate.
//
// Parameters:
//   channel - FM channel containing the operator.
//   op      - Operator whose envelope rate is being queried.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Base four-bit envelope rate.
// -----------------------------------------------------------------------------
uint8_t
VRC7::envelope_parameter_rate (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const
{
	const fm_instrument_t &patch = channel.patch;

	const bool patchSustain = carrier ? patch.carrier_sustain : patch.modulator_sustain;

	switch (op.envelope_state) {
	case FMEnvelopeState::Idle:
		return 0;

	case FMEnvelopeState::Damp:
		return 12;

	case FMEnvelopeState::Attack:
		return carrier ? patch.carrier_attack : patch.modulator_attack;

	case FMEnvelopeState::Decay:
		return carrier ? patch.carrier_decay : patch.modulator_decay;

	case FMEnvelopeState::Sustain:
		return patchSustain ? 0 : (carrier ? patch.carrier_release : patch.modulator_release);

	case FMEnvelopeState::Release:
		if (channel.sustain) {
			return 5;
		}

		if (patchSustain) {
			return carrier ? patch.carrier_release : patch.modulator_release;
		}

		return 7;
	}

	return 0;
}


// -----------------------------------------------------------------------------
// VRC7::envelope_rate_high
//
// Calculates the high portion of the effective VRC7 envelope rate.
//
// The four-bit patch rate is combined with the upper key-rate-scaling bits.
// The result is saturated to the hardware maximum value of 15.
//
// Parameters:
//   channel - FM channel containing the operator.
//   op      - Operator whose envelope rate is being calculated.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Effective envelope rate-high value in the range 0-15.
// -----------------------------------------------------------------------------
uint8_t
VRC7::envelope_rate_high (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const
{
	const uint8_t parameterRate = envelope_parameter_rate(channel, op, carrier);

	if (parameterRate == 0) {
		return 0;
	}

	const uint8_t keyScale = envelope_key_scale(channel, carrier);
	const uint8_t rate = static_cast<uint8_t>(parameterRate + (keyScale >> 2));

	return std::min<uint8_t>(15, rate);
}


// -----------------------------------------------------------------------------
// VRC7::envelope_rate_low
//
// Calculates the low two bits of the effective VRC7 envelope rate.
//
// These bits select one of four envelope stepping patterns used to provide
// the fractional rates between the main envelope timing divisions.
//
// Parameters:
//   channel - FM channel containing the operator.
//   op      - Operator whose envelope rate is being calculated.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Effective envelope rate-low value in the range 0-3.
// -----------------------------------------------------------------------------
uint8_t
VRC7::envelope_rate_low (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const
{
	const uint8_t parameterRate = envelope_parameter_rate(channel, op, carrier);

	if (parameterRate == 0) {
		return 0;
	}

	return envelope_key_scale(channel, carrier) & 0x03;
}


// -----------------------------------------------------------------------------
// VRC7::apply_envelope
//
// Applies VRC7 envelope attenuation to a raw linear waveform sample.
//
// The VRC7/OPLL envelope generator uses a 7-bit attenuation value with a
// resolution of approximately 0.375 dB per step.  Level 0 therefore leaves
// the waveform at full amplitude, while level 127 is effectively silent.
//
// This implementation converts the VRC7 attenuation value into a linear gain
// because the current oscillator path uses linear sine samples.
//
// Parameters:
//   sample        - Raw signed operator waveform sample.
//   envelopeLevel - Current envelope attenuation level, 0-127.
//
// Returns:
//   Envelope-attenuated signed waveform sample.
// -----------------------------------------------------------------------------
int16_t
VRC7::apply_envelope (int16_t sample, uint8_t envelopeLevel) const
{
	if (envelopeLevel >= 127) {
		return 0;
	}

	if (envelopeLevel == 0) {
		return sample;
	}

	const double attenuationDb = static_cast<double>(envelopeLevel) * 0.375;
	const double gain = std::pow(10.0, -attenuationDb / 20.0);

	return static_cast<int16_t>(static_cast<double>(sample) * gain);
}


// -----------------------------------------------------------------------------
// VRC7::combined_attenuation
//
// Combines the current envelope attenuation with the operator's static level
// control.
//
// The modulator uses the instrument's six-bit total-level field, where each
// unit corresponds to two envelope attenuation steps.
//
// The carrier uses the channel's four-bit volume field, where each unit
// corresponds to eight envelope attenuation steps.
//
// The result is saturated to the seven-bit envelope attenuation range.
//
// Parameters:
//   channel - FM channel containing the operator.
//   op      - Operator whose attenuation is being calculated.
//   carrier - true for the carrier, false for the modulator.
//
// Returns:
//   Combined attenuation level in the range 0-127.
// -----------------------------------------------------------------------------
uint8_t
VRC7::combined_attenuation (const fm_channel_t &channel, const fm_operator_t &op, bool carrier) const
{
	unsigned int attenuation = static_cast<unsigned int>(op.envelope_level);

	if (carrier) {
		attenuation += static_cast<unsigned int>(channel.volume) << 3;
	} else {
		attenuation += static_cast<unsigned int>(channel.patch.modulator_output_level) << 1;
	}

	return static_cast<uint8_t>(std::min<unsigned int>(127, attenuation));
}


// -----------------------------------------------------------------------------
// VRC7::feedback_phase_offset
//
// Calculates the phase offset produced by the VRC7 modulator feedback path.
//
// The feedback generator uses the two most recent modulator output samples.
// Their sum is scaled according to the instrument's three-bit feedback value.
// A feedback value of zero disables the feedback path entirely.
//
// Parameters:
//   channel - FM channel whose modulator feedback is being calculated.
//
// Returns:
//   Signed phase offset to apply to the modulator waveform phase.
// -----------------------------------------------------------------------------
int32_t
VRC7::feedback_phase_offset (const fm_channel_t &channel) const
{
	const uint8_t feedback = channel.patch.feedback & 0x07;

	if (feedback == 0) {
		return 0;
	}

	const int32_t history = static_cast<int32_t>(channel.modulator.feedback_output_0) +
							static_cast<int32_t>(channel.modulator.feedback_output_1);

	return history >> (8 - feedback);
}


// -----------------------------------------------------------------------------
// VRC7::carrier_phase_offset
//
// Calculates the carrier phase offset produced by the current modulator
// output.
//
// The modulator output is interpreted as a signed phase-displacement source.
// The resulting offset is expressed in the same 19-bit phase-accumulator units
// used by the operator oscillators.
//
// Parameters:
//   channel - FM channel whose carrier modulation is being calculated.
//
// Returns:
//   Signed phase offset to apply to the carrier waveform phase.
// -----------------------------------------------------------------------------
int32_t
VRC7::carrier_phase_offset(const fm_channel_t &channel) const
{
	return static_cast<int32_t>(channel.modulator.output);
}


// -----------------------------------------------------------------------------
// VRC7::update_fm_output
//
// Calculates the current composite output of the six VRC7 FM channels.
//
// Only the carrier output of each two-operator FM channel contributes directly
// to the final mix.  Muted channels are omitted from the audible mix, but
// their synthesis state continues to run normally so phase, envelopes, key
// state, and debugger information remain accurate.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::update_fm_output()
{
	int32_t output = 0;

	for (int i = 0; i < 6; ++i) {
		if (fm_channels_[i].muted) {
			continue;
		}

		output += static_cast<int32_t>(fm_channels_[i].carrier.output);
	}

	fm_output_ = output;
}


// -----------------------------------------------------------------------------
// VRC7::audio_output
//
// Returns the current composite VRC7 FM output through the generic mapper
// expansion-audio interface.
//
// The internal VRC7 FM mix is signed.  It is normalized by the established
// VRC7 output scaling and biased around 128 for the generic unsigned
// mapper-audio interface.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC7 expansion-audio sample.
// -----------------------------------------------------------------------------
uint8_t
VRC7::audio_output() const
{
	if (audio_silenced_) {
		return 128;
	}

	int32_t output = fm_output_ / 192;
	output += 128;

	if (output < 0) {
		output = 0;
	} else if (output > 255) {
		output = 255;
	}

	return static_cast<uint8_t>(output);
}


// -----------------------------------------------------------------------------
// VRC7::audio_channel_count
//
// Returns the number of independently mutable VRC7 FM channels.
//
// Parameters:
//   None.
//
// Returns:
//   Number of VRC7 FM channels.
// -----------------------------------------------------------------------------
size_t
VRC7::audio_channel_count() const
{
	return 6;
}


// -----------------------------------------------------------------------------
// VRC7::audio_channel_name
//
// Returns the display name for a VRC7 FM channel.
//
// Parameters:
//   index - Zero-based FM channel index.
//
// Returns:
//   Human-readable channel name, or nullptr for an invalid index.
// -----------------------------------------------------------------------------
const char*
VRC7::audio_channel_name (size_t index) const
{
	static const char *kChannelNames[6] = {
		"FM 1",
		"FM 2",
		"FM 3",
		"FM 4",
		"FM 5",
		"FM 6"
	};

	if (index >= 6) {
		return nullptr;
	}

	return kChannelNames[index];
}


// -----------------------------------------------------------------------------
// VRC7::audio_channel_muted
//
// Returns whether a VRC7 FM channel is currently muted.
//
// Parameters:
//   index - Zero-based FM channel index.
//
// Returns:
//   true if the channel is muted; false otherwise.
// -----------------------------------------------------------------------------
bool
VRC7::audio_channel_muted (size_t index) const
{
	if (index >= 6) {
		return false;
	}

	return fm_channels_[index].muted;
}


// -----------------------------------------------------------------------------
// VRC7::set_audio_channel_muted
//
// Changes the mute state of one VRC7 FM channel.
//
// Muting affects only the audible expansion-audio mix. The FM synthesizer
// continues running normally so phase, envelopes, key state, and debugger
// information remain accurate.
//
// Parameters:
//   index - Zero-based FM channel index.
//   muted - New mute state.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VRC7::set_audio_channel_muted (size_t index, bool muted)
{
	if (index >= 6) {
		return;
	}

	fm_channels_[index].muted = muted;
}


// -----------------------------------------------------------------------------
// VRC7::debug_audio_state
//
// Returns debugger-facing state for the VRC7 expansion-audio channels.
//
// The snapshot exposes mapper-wide VRC7 audio state together with all six FM
// channels.  Each channel reports its current frequency parameters, instrument,
// volume, key/sustain state, mute state, and a normalized carrier-output level.
//
// The carrier output is used for the channel level meter because it represents
// the audible output of the two-operator FM voice.  The modulator affects the
// carrier through phase modulation but is not mixed directly.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRC7 expansion-audio debug state.
// -----------------------------------------------------------------------------
mapper_audio_debug_state_t
VRC7::debug_audio_state() const
{
	mapper_audio_debug_state_t state;

	state.type = ExpansionAudioType::VRC7;
	state.name = "VRC7";

	state.output = audio_output();
	state.output_maximum = 255;


	// -------------------------------------------------------------------------
	// Global VRC7 audio state
	// -------------------------------------------------------------------------

	state.field_count = 4;

	state.fields[0].label = "Audio Silenced";
	state.fields[0].value = audio_silenced_;
	state.fields[0].type = MapperAudioDebugFieldType::Boolean;

	state.fields[1].label = "FM Register";
	state.fields[1].value = audio_register_;
	state.fields[1].type = MapperAudioDebugFieldType::Hex;

	state.fields[2].label = "Combined Output";
	state.fields[2].value = state.output;
	state.fields[2].type = MapperAudioDebugFieldType::Output;

	state.fields[3].label = "Output Level";
	state.fields[3].value = state.output;
	state.fields[3].maximum = state.output_maximum;
	state.fields[3].type = MapperAudioDebugFieldType::Percent;


	// -------------------------------------------------------------------------
	// FM channels
	// -------------------------------------------------------------------------

	static constexpr double kVRC7ChannelClock = 49716.0;

	state.channel_count = 6;

	for (size_t i = 0; i < 6; ++i) {
		const fm_channel_t &channel = fm_channels_[i];
		mapper_audio_channel_debug_t &debugChannel = state.channels[i];

		debugChannel.name = audio_channel_name(i);
		debugChannel.period = channel.f_number;

		/*
		 * Convert the signed carrier sample into a positive level
		 * suitable for the generic expansion-audio level meter.
		 *
		 * waveform_sample() currently produces approximately
		 * -4095 through +4095.  Dividing its absolute magnitude
		 * by 16 maps that into approximately 0-255.
		 */
		int32_t carrierLevel = static_cast<int32_t>(channel.carrier.output);

		if (carrierLevel < 0) {
			carrierLevel = -carrierLevel;
		}

		carrierLevel /= 16;

		if (carrierLevel > 255) {
			carrierLevel = 255;
		}

		debugChannel.output = static_cast<uint8_t>(carrierLevel);
		debugChannel.output_maximum = 255;
		debugChannel.enabled = channel.key_on;
		debugChannel.muted = channel.muted;
		debugChannel.halted = audio_silenced_;

		/*
		 * VRC7 channel frequency:
		 *
		 *              49716 * FNUM
		 *     Hz = ----------------------
		 *           2 ^ (19 - block)
		 */
		if (!audio_silenced_ && channel.f_number != 0) {
			const uint32_t divisor = 1u << (19 - channel.block);

			debugChannel.frequency_hz = kVRC7ChannelClock * static_cast<double>(channel.f_number) / 
										static_cast<double>(divisor);
			debugChannel.frequency_valid = true;
		}


		// -----------------------------------------------------------------
		// Channel-specific VRC7 state
		// -----------------------------------------------------------------

		debugChannel.field_count = 6;
		debugChannel.fields[0].label = "F-Number";
		debugChannel.fields[0].value = channel.f_number;
		debugChannel.fields[0].type = MapperAudioDebugFieldType::Hex;


		debugChannel.fields[1].label = "Block";
		debugChannel.fields[1].value = channel.block;
		debugChannel.fields[1].type = MapperAudioDebugFieldType::Value;

		debugChannel.fields[2].label = "Instrument";
		debugChannel.fields[2].value = channel.instrument;
		debugChannel.fields[2].type = MapperAudioDebugFieldType::Value;

		debugChannel.fields[3].label = "Volume";
		debugChannel.fields[3].value = channel.volume;
		debugChannel.fields[3].maximum = 15;
		debugChannel.fields[3].type = MapperAudioDebugFieldType::Value;

		debugChannel.fields[4].label = "Key On";
		debugChannel.fields[4].value = channel.key_on;
		debugChannel.fields[4].type = MapperAudioDebugFieldType::Boolean;

		debugChannel.fields[5].label = "Sustain";
		debugChannel.fields[5].value = channel.sustain;
		debugChannel.fields[5].type = MapperAudioDebugFieldType::Boolean;
	}

	return state;
}


// -----------------------------------------------------------------------------
// VRC7::audio_output_bias
//
// Returns the unsigned midpoint used by the VRC7 expansion-audio output.
//
// The internal FM waveform is signed, but audio_output() maps it into an
// unsigned eight-bit representation centered on 128.  The generic APU mixer
// subtracts this bias before mixing so VRC7 silence corresponds to zero signal
// and does not introduce a DC step.
//
// Parameters:
//   None.
//
// Returns:
//   Unsigned VRC7 audio-output midpoint.
// -----------------------------------------------------------------------------
int32_t
VRC7::audio_output_bias() const
{
	return 128;
}


// -----------------------------------------------------------------------------
// VRC7::debug_audio_scope_sample
//
// Returns the instantaneous output level of each VRC7 FM channel for
// oscilloscope capture.
//
// The VRC7 carrier output is signed and currently ranges approximately from
// -4095 through +4095.  The generic mapper oscilloscope interface stores
// unsigned eight-bit samples, so each carrier waveform is scaled and biased
// around 128.  This preserves both the positive and negative halves of the FM
// waveform in the debugger.
//
// Muting is intentionally ignored here.  A muted channel continues synthesizing
// normally, so the oscilloscope continues to show its underlying waveform even
// when it is omitted from the audible mix.
//
// Channel order matches the VRC7 debugger and mute-control order:
//
//   0 - FM 1
//   1 - FM 2
//   2 - FM 3
//   3 - FM 4
//   4 - FM 5
//   5 - FM 6
//
// Parameters:
//   None.
//
// Returns:
//   Current per-channel VRC7 oscilloscope sample.
// -----------------------------------------------------------------------------
mapper_audio_scope_sample_t
VRC7::debug_audio_scope_sample() const
{
	mapper_audio_scope_sample_t sample;

	sample.channel_count = 6;

	for (size_t i = 0; i < 6; ++i) {
		int32_t output =
			static_cast<int32_t>(fm_channels_[i].carrier.output);
		/*
		 * Convert approximately -4095..+4095 into 0..255,
		 * centered around unsigned scope silence at 128.
		 */
		output /= 32;
		output += 128;

		if (output < 0) {
			output = 0;
		} else if (output > 255) {
			output = 255;
		}

		sample.channels[i] = static_cast<uint8_t>(output);
	}

	return sample;
}

