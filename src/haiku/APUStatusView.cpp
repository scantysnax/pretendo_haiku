#include "APUStatusView.h"

#include "Apu.h"

#include <Font.h>
#include <String.h>
#include <Window.h>


namespace {

constexpr float kLeftMargin = 12.0f;
constexpr float kTopMargin = 18.0f;

constexpr float kColumn1X = 12.0f;
constexpr float kColumn2X = 245.0f;
constexpr float kColumn3X = 478.0f;

constexpr float kLineHeight = 16.0f;
constexpr float kSectionGap = 8.0f;

constexpr bigtime_t kPulseRate = 100000;

}


// -----------------------------------------------------------------------------
// APUStatusView::APUStatusView
//
// Creates the APU status debugger view.
//
// Parameters:
//   frame - Initial view frame.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUStatusView::APUStatusView(BRect frame)
	: BView(
		frame,
		"apu_status_view",
		B_FOLLOW_ALL,
		B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(245, 245, 245);
	SetLowColor(ViewColor());
}


// -----------------------------------------------------------------------------
// APUStatusView::AttachedToWindow
//
// Configures the debugger view after it has been attached to its window.
//
// The window pulse rate is enabled so the displayed APU state is refreshed
// periodically while emulation is running.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Window()) {
		Window()->SetPulseRate(kPulseRate);
	}
}


// -----------------------------------------------------------------------------
// APUStatusView::Pulse
//
// Requests a repaint of the APU status view.
//
// The actual APU state is captured during Draw(), keeping the pulse handler
// lightweight and ensuring that each repaint uses one debugger snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::Pulse()
{
	Invalidate();
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawSectionHeader
//
// Draws a section heading using a bold font.
//
// Parameters:
//   text - Section title.
//   x    - Horizontal drawing position.
//   y    - Baseline drawing position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawSectionHeader(
	const char *text,
	float x,
	float y)
{
	BFont font;
	GetFont(&font);

	BFont boldFont(font);
	boldFont.SetFace(B_BOLD_FACE);

	SetFont(&boldFont);
	SetHighColor(20, 20, 20);

	DrawString(text, BPoint(x, y));

	SetFont(&font);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawTextLine
//
// Draws one label/value debugger line.
//
// Parameters:
//   label - Field name.
//   value - Formatted field value.
//   x     - Horizontal drawing position.
//   y     - Baseline drawing position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawTextLine(
	const char *label,
	const char *value,
	float x,
	float y)
{
	BString line;

	line.SetToFormat(
		"%-15s %s",
		label,
		value);

	SetHighColor(20, 20, 20);
	DrawString(line.String(), BPoint(x, y));
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawBoolLine
//
// Draws one boolean debugger field as YES or NO.
//
// Parameters:
//   label - Field name.
//   value - Boolean field value.
//   x     - Horizontal drawing position.
//   y     - Baseline drawing position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawBoolLine(
	const char *label,
	bool value,
	float x,
	float y)
{
	DrawTextLine(
		label,
		value ? "YES" : "NO",
		x,
		y);
}


// -----------------------------------------------------------------------------
// APUStatusView::Draw
//
// Draws a side-effect-free snapshot of the current NES APU state.
//
// Only nes::apu::debug_state() is used to inspect the APU.  The view does not
// access individual channel implementation objects directly.
//
// Parameters:
//   updateRect - Area requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(ViewColor());
	FillRect(Bounds());

	SetHighColor(20, 20, 20);

	BFont font(be_fixed_font);
	SetFont(&font);

	const nes::apu::apu_debug_state_t state =
		nes::apu::debug_state();

	BString value;

	float y = kTopMargin;


	// -------------------------------------------------------------------------
	// Overall APU state
	// -------------------------------------------------------------------------

	DrawSectionHeader(
		"APU STATUS",
		kLeftMargin,
		y);

	y += kLineHeight + kSectionGap;


	value.SetToFormat(
		"%llu",
		static_cast<unsigned long long>(state.cycle));

	DrawTextLine(
		"Cycle:",
		value.String(),
		kLeftMargin,
		y);

	y += kLineHeight;


	value.SetToFormat(
		"%llu",
		static_cast<unsigned long long>(state.next_frame_cycle));

	DrawTextLine(
		"Next frame:",
		value.String(),
		kLeftMargin,
		y);

	y += kLineHeight;


	DrawTextLine(
		"Frame mode:",
		state.five_step_mode ? "5-step" : "4-step",
		kLeftMargin,
		y);

	y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.frame_step));

	DrawTextLine(
		"Frame step:",
		value.String(),
		kLeftMargin,
		y);

	y += kLineHeight;


	DrawBoolLine(
		"IRQ inhibit:",
		state.frame_irq_inhibit,
		kLeftMargin,
		y);

	y += kLineHeight;


	DrawBoolLine(
		"Frame IRQ:",
		state.frame_irq,
		kLeftMargin,
		y);

	y += kLineHeight;


	DrawBoolLine(
		"DMC IRQ:",
		state.dmc_irq,
		kLeftMargin,
		y);

	y += kLineHeight;


	value.SetToFormat(
		"$%02X",
		static_cast<unsigned>(state.status));

	DrawTextLine(
		"Status:",
		value.String(),
		kLeftMargin,
		y);

	y += kLineHeight;


	DrawBoolLine(
		"Debug mute:",
		state.audio_muted,
		kLeftMargin,
		y);


	// -------------------------------------------------------------------------
	// Square 1
	// -------------------------------------------------------------------------

	float square1Y = kTopMargin;

	DrawSectionHeader(
		"SQUARE 1",
		kColumn2X,
		square1Y);

	square1Y += kLineHeight + kSectionGap;


	DrawBoolLine(
		"Enabled:",
		state.square1.enabled,
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	DrawBoolLine(
		"Muted:",
		state.square1.muted,
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"$%03X",
		static_cast<unsigned>(state.square1.timer_period));

	DrawTextLine(
		"Timer:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.timer_frequency));

	DrawTextLine(
		"Frequency:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.duty));

	DrawTextLine(
		"Duty:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.sequence_index));

	DrawTextLine(
		"Sequence:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.length_counter));

	DrawTextLine(
		"Length:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.envelope_volume));

	DrawTextLine(
		"Envelope:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	DrawBoolLine(
		"Sweep:",
		state.square1.sweep_enabled,
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"P:%u N:%u S:%u",
		static_cast<unsigned>(state.square1.sweep_period),
		state.square1.sweep_negate ? 1u : 0u,
		static_cast<unsigned>(state.square1.sweep_shift));

	DrawTextLine(
		"Sweep cfg:",
		value.String(),
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	DrawBoolLine(
		"Sweep mute:",
		state.square1.sweep_silenced,
		kColumn2X,
		square1Y);

	square1Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square1.output));

	DrawTextLine(
		"Output:",
		value.String(),
		kColumn2X,
		square1Y);


	// -------------------------------------------------------------------------
	// Square 2
	// -------------------------------------------------------------------------

	float square2Y = kTopMargin;

	DrawSectionHeader(
		"SQUARE 2",
		kColumn3X,
		square2Y);

	square2Y += kLineHeight + kSectionGap;


	DrawBoolLine(
		"Enabled:",
		state.square2.enabled,
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	DrawBoolLine(
		"Muted:",
		state.square2.muted,
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"$%03X",
		static_cast<unsigned>(state.square2.timer_period));

	DrawTextLine(
		"Timer:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.timer_frequency));

	DrawTextLine(
		"Frequency:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.duty));

	DrawTextLine(
		"Duty:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.sequence_index));

	DrawTextLine(
		"Sequence:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.length_counter));

	DrawTextLine(
		"Length:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.envelope_volume));

	DrawTextLine(
		"Envelope:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	DrawBoolLine(
		"Sweep:",
		state.square2.sweep_enabled,
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"P:%u N:%u S:%u",
		static_cast<unsigned>(state.square2.sweep_period),
		state.square2.sweep_negate ? 1u : 0u,
		static_cast<unsigned>(state.square2.sweep_shift));

	DrawTextLine(
		"Sweep cfg:",
		value.String(),
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	DrawBoolLine(
		"Sweep mute:",
		state.square2.sweep_silenced,
		kColumn3X,
		square2Y);

	square2Y += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.square2.output));

	DrawTextLine(
		"Output:",
		value.String(),
		kColumn3X,
		square2Y);


	// -------------------------------------------------------------------------
	// Triangle
	// -------------------------------------------------------------------------

	float lowerY = 245.0f;

	DrawSectionHeader(
		"TRIANGLE",
		kColumn1X,
		lowerY);

	lowerY += kLineHeight + kSectionGap;


	DrawBoolLine(
		"Enabled:",
		state.triangle.enabled,
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"Muted:",
		state.triangle.muted,
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"$%03X",
		static_cast<unsigned>(state.triangle.timer_period));

	DrawTextLine(
		"Timer:",
		value.String(),
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.triangle.timer_frequency));

	DrawTextLine(
		"Frequency:",
		value.String(),
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.triangle.length_counter));

	DrawTextLine(
		"Length:",
		value.String(),
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.triangle.linear_counter));

	DrawTextLine(
		"Linear:",
		value.String(),
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.triangle.sequence_index));

	DrawTextLine(
		"Sequence:",
		value.String(),
		kColumn1X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.triangle.output));

	DrawTextLine(
		"Output:",
		value.String(),
		kColumn1X,
		lowerY);


	// -------------------------------------------------------------------------
	// Noise
	// -------------------------------------------------------------------------

	lowerY = 245.0f;

	DrawSectionHeader(
		"NOISE",
		kColumn2X,
		lowerY);

	lowerY += kLineHeight + kSectionGap;


	DrawBoolLine(
		"Enabled:",
		state.noise.enabled,
		kColumn2X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"Muted:",
		state.noise.muted,
		kColumn2X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.noise.timer_period));

	DrawTextLine(
		"Timer:",
		value.String(),
		kColumn2X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.noise.length_counter));

	DrawTextLine(
		"Length:",
		value.String(),
		kColumn2X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.noise.envelope_volume));

	DrawTextLine(
		"Envelope:",
		value.String(),
		kColumn2X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.noise.output));

	DrawTextLine(
		"Output:",
		value.String(),
		kColumn2X,
		lowerY);


	// -------------------------------------------------------------------------
	// DMC
	// -------------------------------------------------------------------------

	lowerY = 245.0f;

	DrawSectionHeader(
		"DMC",
		kColumn3X,
		lowerY);

	lowerY += kLineHeight + kSectionGap;


	DrawBoolLine(
		"Active:",
		state.dmc.active,
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"Muted:",
		state.dmc.muted,
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.dmc.timer_period));

	DrawTextLine(
		"Timer:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"IRQ enable:",
		state.dmc.irq_enabled,
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"Loop:",
		state.dmc.loop,
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.dmc.output));

	DrawTextLine(
		"Output:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"$%04X",
		static_cast<unsigned>(state.dmc.sample_address));

	DrawTextLine(
		"Sample addr:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"$%04X",
		static_cast<unsigned>(state.dmc.current_address));

	DrawTextLine(
		"Current addr:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.dmc.sample_length));

	DrawTextLine(
		"Sample len:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.dmc.bytes_remaining));

	DrawTextLine(
		"Bytes left:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	value.SetToFormat(
		"%u",
		static_cast<unsigned>(state.dmc.bits_remaining));

	DrawTextLine(
		"Bits left:",
		value.String(),
		kColumn3X,
		lowerY);

	lowerY += kLineHeight;


	DrawBoolLine(
		"Buffer empty:",
		state.dmc.sample_buffer_empty,
		kColumn3X,
		lowerY);
}

