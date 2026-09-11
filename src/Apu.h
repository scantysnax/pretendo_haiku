#ifndef _APU_H_
#define _APU_H_

#include "BitField.h"
#include "Reset.h"

#include <cstddef>
#include <cstdint>


namespace nes::apu {
	
// Host audio output rate in samples per second.
constexpr int32_t kOutputFrequency = 48000;

// Nominal emulator video frame rate.
constexpr int32_t kFrameRate = 60;

// Audio sample buffer sized for roughly four video frames of output.
constexpr int32_t kBufferSize = (kOutputFrequency / kFrameRate) * 4;

// Unsigned 8-bit PCM silence level.
constexpr uint8_t kSilence = 0x80;


// Identifies the five NES APU sound channels.
typedef enum {
	SQUARE1 = 0,
	SQUARE2 = 1,
	TRIANGLE = 2,
	NOISE = 3,
	DPCM = 4
} sound_channel;


// Forward declarations for the APU channel implementations.
template <int Channel>
class Square;

class Triangle;
class Noise;
class DMC;


// -----------------------------------------------------------------------------
// apu_status_t
//
// Represents the APU status register and its individual status/interrupt bits.
//
// The channel-enable fields occupy bits 0-4.  Bit 5 is unused.  Bits 6 and 7
// report the frame-counter and DMC interrupt flags respectively.
//
// irq_firing is a debugger/internal convenience field spanning bits 6-7 so the
// code can quickly determine whether either APU interrupt source is active.
// -----------------------------------------------------------------------------
union apu_status_t {
	uint8_t raw;

	// Channel status bits.
	BitField<uint8_t, 0> square1_enabled;
	BitField<uint8_t, 1> square2_enabled;
	BitField<uint8_t, 2> triangle_enabled;
	BitField<uint8_t, 3> noise_enabled;
	BitField<uint8_t, 4> dmc_enabled;

	// Interrupt status bits.
	BitField<uint8_t, 6> frame_irq;
	BitField<uint8_t, 7> dmc_irq;

	// Meta-field covering both interrupt bits.
	BitField<uint8_t, 6, 2> irq_firing;
};


// -----------------------------------------------------------------------------
// square_debug_state_t
//
// Side-effect-free debugger snapshot of one NES square-wave channel.
//
// The structure captures the channel's effective enable/mute state, timer and
// duty configuration, sequencer position, length/envelope state, sweep state,
// and current output level.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct square_debug_state_t {
	// Channel state.
	bool enabled = false;
	bool muted = false;

	// Timer state.
	uint16_t timer_period = 0;
	uint16_t timer_frequency = 0;

	// Duty-cycle / waveform sequencer state.
	uint8_t duty = 0;
	uint8_t sequence_index = 0;

	// Length-counter state.
	uint8_t length_counter = 0;

	// Current envelope output volume.
	uint8_t envelope_volume = 0;

	// Sweep-unit state.
	bool sweep_enabled = false;
	uint8_t sweep_period = 0;
	bool sweep_negate = false;
	uint8_t sweep_shift = 0;
	bool sweep_silenced = false;

	// Current channel output level.
	uint8_t output = 0;
};


// -----------------------------------------------------------------------------
// triangle_debug_state_t
//
// Side-effect-free debugger snapshot of the NES triangle-wave channel.
//
// The structure captures the channel's effective enable/mute state, timer
// configuration, length and linear counters, waveform sequencer position,
// and current output level.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct triangle_debug_state_t {
	// Channel state.
	bool enabled = false;
	bool muted = false;

	// Timer state.
	uint16_t timer_period = 0;
	uint16_t timer_frequency = 0;

	// Length and linear-counter state.
	uint8_t length_counter = 0;
	uint8_t linear_counter = 0;

	// Waveform sequencer position and current output level.
	uint8_t sequence_index = 0;
	uint8_t output = 0;
};


// -----------------------------------------------------------------------------
// noise_debug_state_t
//
// Side-effect-free debugger snapshot of the NES noise channel.
//
// The structure captures the channel's effective enable/mute state, timer
// period, length/envelope state, and current output level.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct noise_debug_state_t {
	// Channel state.
	bool enabled = false;
	bool muted = false;

	// Noise timer period.
	uint16_t timer_period = 0;

	// Length-counter and envelope state.
	uint8_t length_counter = 0;
	uint8_t envelope_volume = 0;

	// Current channel output level.
	uint8_t output = 0;
};


// -----------------------------------------------------------------------------
// dmc_debug_state_t
//
// Side-effect-free debugger snapshot of the NES DMC channel.
//
// The structure captures enable/activity state, timer configuration, interrupt
// and loop control, current DAC output, sample-address/length state, remaining
// transfer state, and sample-buffer status.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct dmc_debug_state_t {
	// Channel state.
	bool enabled = false;
	bool active = false;
	bool muted = false;

	// DMC timer period.
	uint16_t timer_period = 0;

	// Playback control.
	bool irq_enabled = false;
	bool loop = false;

	// Current 7-bit DMC DAC output level.
	uint8_t output = 0;

	// Sample address and transfer state.
	uint16_t sample_address = 0;
	uint16_t current_address = 0;
	uint16_t sample_length = 0;
	uint16_t bytes_remaining = 0;

	// Remaining bits in the current output shift operation.
	uint8_t bits_remaining = 0;

	// true when no prefetched sample byte is currently buffered.
	bool sample_buffer_empty = true;
};


// -----------------------------------------------------------------------------
// apu_debug_state_t
//
// Side-effect-free debugger snapshot of the complete NES APU state.
//
// The structure captures global APU timing and frame-counter state, interrupt
// status, debugger audio-mute state, and the current effective state of all
// five audio channels.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct apu_debug_state_t {
	// Current global APU cycle count.
	uint64_t cycle = 0;

	// Frame-counter state.
	bool five_step_mode = false;
	bool frame_irq_inhibit = false;
	uint8_t frame_step = 0;

	// APU cycle at which the next frame-counter event is scheduled.
	uint64_t next_frame_cycle = 0;

	// Reconstructed $4015-style APU status value.
	uint8_t status = 0;

	// Current interrupt flags.
	bool frame_irq = false;
	bool dmc_irq = false;

	// Debugger-controlled host audio mute state.
	bool audio_muted = false;

	// Square-channel debugger state.
	square_debug_state_t square1;
	square_debug_state_t square2;

	// Remaining APU channel debugger state.
	triangle_debug_state_t triangle;
	noise_debug_state_t noise;
	dmc_debug_state_t dmc;
};

// Returns a side-effect-free snapshot of the current APU and channel state.
apu_debug_state_t debug_state();



// -----------------------------------------------------------------------------
// apu_frame_sequencer_debug_state_t
//
// Side-effect-free debugger snapshot of the NES APU frame sequencer.
//
// The structure captures the current APU cycle, the next scheduled frame-
// sequencer event, the number of cycles remaining until that event, the active
// 4-step/5-step mode, IRQ state, and the type of work performed by the upcoming
// sequencer event.
//
// next_step identifies the next internal sequencer step that will execute when
// next_event_cycle is reached.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct apu_frame_sequencer_debug_state_t {
	// Current global APU cycle count.
	uint64_t apu_cycle;

	// Cycle at which the next frame-sequencer event will occur.
	uint64_t next_event_cycle;

	// Number of APU cycles remaining until the next scheduled event.
	uint64_t cycles_until_next_event;

	// Next internal sequencer step and total number of internal steps.
	uint8_t next_step;
	uint8_t step_count;

	// true when the frame counter is operating in 5-step mode.
	bool five_step_mode;

	// Frame-IRQ control and current frame-IRQ state.
	bool irq_inhibit;
	bool frame_irq;

	// Operations performed by the next scheduled sequencer event.
	bool next_event_quarter_frame;
	bool next_event_half_frame;
	bool next_event_irq_point;

	// Most recently programmed $4017 frame-counter register value.
	uint8_t frame_counter_register;
};


// Returns a side-effect-free snapshot of the current APU frame sequencer.
apu_frame_sequencer_debug_state_t debug_frame_sequencer_state();


// -----------------------------------------------------------------------------
// APU frame-sequencer event history
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// APU frame-sequencer event history
//
// Defines the rolling debugger history used to record meaningful APU
// frame-sequencer events.
//
// The history records quarter-frame clocks, half-frame clocks, combined
// quarter+half-frame clocks, and frame-IRQ timing points together with the APU
// cycle and sequencer mode in which they occurred.
//
// This history is intended for debugger/UI inspection and does not affect APU
// timing or behavior.
// -----------------------------------------------------------------------------

// Maximum number of frame-sequencer events retained in the rolling history.
constexpr uint32_t APU_FRAME_EVENT_CAPACITY = 64;


// Type of frame-sequencer event recorded in the history.
enum apu_frame_event_type {
	APU_FRAME_EVENT_QUARTER = 0,
	APU_FRAME_EVENT_HALF,
	APU_FRAME_EVENT_QUARTER_HALF,
	APU_FRAME_EVENT_IRQ
};


// -----------------------------------------------------------------------------
// apu_frame_event_t
//
// Describes one recorded APU frame-sequencer event.
//
// cycle identifies the APU cycle on which the event occurred.  step records the
// internal sequencer step responsible for the event, while five_step_mode and
// irq_inhibit capture the relevant frame-counter state at that moment.
// -----------------------------------------------------------------------------
struct apu_frame_event_t {
	// APU cycle on which the event occurred.
	uint64_t cycle;

	// Type of frame-sequencer event.
	apu_frame_event_type type;

	// Internal frame-sequencer step that generated the event.
	uint8_t step;

	// true when the event occurred while using the 5-step sequence.
	bool five_step_mode;

	// Frame-IRQ inhibit state when the event occurred.
	bool irq_inhibit;
};


// Copies the rolling frame-sequencer event history in chronological order.
uint32_t debug_frame_event_snapshot (apu_frame_event_t *events, uint32_t capacity);

// Clears the debugger frame-sequencer event history.
void debug_clear_frame_events();


// Debugger-controlled host audio mute state.
extern bool debug_audio_muted;

// Enables or disables debugger-controlled host audio mute.
void debug_set_audio_muted (bool muted);

// Returns true when debugger-controlled host audio mute is active.
bool debug_audio_is_muted();



// -----------------------------------------------------------------------------
// APU register-write history
//
// Defines the rolling debugger history used to record CPU writes to APU
// registers.
//
// Each entry stores the APU cycle, register address, written value, and a
// monotonically increasing write index.  Selected writes may also carry extra
// decoded state used by debugger views, such as a reconstructed timer period
// or the previous $4015 channel-enable mask.
//
// This history is intended for debugger/UI inspection and does not affect APU
// behavior.
// -----------------------------------------------------------------------------

// Maximum number of APU register writes retained in the rolling history.
constexpr uint32_t APU_WRITE_LOG_CAPACITY = 256;


// -----------------------------------------------------------------------------
// apu_write_log_entry_t
//
// Describes one recorded CPU write to an APU register.
//
// timer_period is valid only when has_timer_period is true and is used for
// writes that affect a complete channel timer value.
//
// previous_enable_mask is valid only when has_previous_enable_mask is true and
// records the channel-enable state that existed before a $4015 write.
// -----------------------------------------------------------------------------
struct apu_write_log_entry_t {
	// APU cycle on which the write occurred.
	uint64_t cycle;

	// CPU-visible APU register address.
	uint16_t address;

	// Value written to the register.
	uint8_t value;

	// Monotonically increasing debugger write sequence number.
	uint32_t write_index;

	// Optional reconstructed timer period after the write.
	uint16_t timer_period;
	bool has_timer_period;

	// Optional channel-enable mask captured before a $4015 write.
	uint8_t previous_enable_mask;
	bool has_previous_enable_mask;
};


// Returns the number of valid entries currently stored in the write log.
uint32_t apu_write_log_count();

// Copies the rolling APU write log into caller-provided storage.
uint32_t apu_write_log_snapshot(apu_write_log_entry_t *entries, uint32_t capacity);

// Clears the debugger APU register-write history.
void clear_apu_write_log();


// -----------------------------------------------------------------------------
// apu_explorer_state_t
//
// Side-effect-free debugger snapshot of the most recently programmed raw APU
// register values.
//
// Unlike apu_debug_state_t, which reports effective channel state, this
// structure preserves the raw values most recently written by the CPU so the
// APU Explorer can show exactly how each channel was programmed.
//
// This is intended for debugger/UI inspection and does not itself modify APU
// behavior.
// -----------------------------------------------------------------------------
struct apu_explorer_state_t {
	// Square 1 registers: $4000-$4003.
	uint8_t square1[4] = {};

	// Square 2 registers: $4004-$4007.
	uint8_t square2[4] = {};

	// Triangle registers.
	uint8_t triangle0 = 0;		// $4008
	uint8_t triangle2 = 0;		// $400A
	uint8_t triangle3 = 0;		// $400B

	// Noise registers.
	uint8_t noise0 = 0;			// $400C
	uint8_t noise2 = 0;			// $400E
	uint8_t noise3 = 0;			// $400F

	// DMC registers: $4010-$4013.
	uint8_t dmc[4] = {};

	// Most recently programmed global APU control values.
	uint8_t status = 0;			// Last value written to $4015.
	uint8_t frame_counter = 0;	// Last value written to $4017.
};

// Returns a side-effect-free snapshot of the raw APU programming state.
apu_explorer_state_t explorer_state();


// -----------------------------------------------------------------------------
// APU oscilloscope sample history
//
// Defines the rolling debugger sample history used by the APU Scope.
//
// Each sample captures the instantaneous output level of the five individual
// APU channels together with the already-generated final mixed PCM sample.
//
// This history is intended for debugger/UI inspection and does not affect APU
// timing, mixing, or host audio output.
// -----------------------------------------------------------------------------

// Maximum number of oscilloscope samples retained in the rolling history.
constexpr uint32_t APU_SCOPE_SAMPLE_CAPACITY = 1024;


// -----------------------------------------------------------------------------
// apu_scope_sample_t
//
// Describes one captured APU oscilloscope sample.
//
// The individual channel fields contain their instantaneous output levels.
// mixed contains the final mixed unsigned PCM sample produced by the normal
// audio path.
// -----------------------------------------------------------------------------
struct apu_scope_sample_t {
	// Instantaneous channel output levels.
	uint8_t square1;
	uint8_t square2;
	uint8_t triangle;
	uint8_t noise;
	uint8_t dmc;

	// Final mixed PCM sample.
	float mixed;
};


// Copies the rolling APU scope history into caller-provided storage.
uint32_t debug_scope_snapshot(apu_scope_sample_t *samples, uint32_t capacity);

// Clears the debugger oscilloscope sample history.
void debug_clear_scope();


// Resets APU timing, channel state, and debugger-visible APU state.
void reset(Reset reset_type);

// Square 1 register writes ($4000-$4003).
void write4000(uint8_t value);
void write4001(uint8_t value);
void write4002(uint8_t value);
void write4003(uint8_t value);

// Square 2 register writes ($4004-$4007).
void write4004(uint8_t value);
void write4005(uint8_t value);
void write4006(uint8_t value);
void write4007(uint8_t value);

// Triangle register writes ($4008, $400A, $400B).
void write4008(uint8_t value);
void write400A(uint8_t value);
void write400B(uint8_t value);

// Noise register writes ($400C, $400E, $400F).
void write400C(uint8_t value);
void write400E(uint8_t value);
void write400F(uint8_t value);

// DMC register writes ($4010-$4013).
void write4010(uint8_t value);
void write4011(uint8_t value);
void write4012(uint8_t value);
void write4013(uint8_t value);

// Global APU control-register writes.
void write4015(uint8_t value);
void write4017(uint8_t value);

// Reads the APU status register ($4015).
uint8_t read4015();


// Returns the current number of elapsed APU cycles.
uint64_t cycle_count();

// Advances the APU by one emulated cycle.
void tick();

// Marks the beginning of a new emulated video frame.
void start_frame();


// Mutes one APU channel without changing its internal timing or enable state.
void mute_channel(int const channel);

// Restores audible output for a previously muted APU channel.
void unmute_channel(int const channel);


// Copies mixed APU samples into the host buffer, repeating the last sample if needed.
size_t read_samples(uint8_t *buffer, size_t size);


// Advances the APU by the specified compile-time number of cycles.
template <int Cycles>
void exec()
{
	for (int i = 0; i < Cycles; ++i) {
		tick();
	}
}

// Shared APU channel objects and global status register state.
extern Square<0> square_0;
extern Square<1> square_1;

extern Triangle triangle;
extern Noise noise;
extern DMC dmc;

extern apu_status_t status;


}	// end namespace nes::apu


#endif // _APU_H_
