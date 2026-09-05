
#include "APUStatusView.h"


// -----------------------------------------------------------------------------
// APUStatusView::APUStatusView
//
// Creates the APU status debugger view.
//
// Parameters:
//   frame - Initial bounds of the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUStatusView::APUStatusView (BRect frame)
	: BView (frame, "apu_status_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fSquare1CheckBox = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), "square1_enable",
									 "Enabled", new BMessage(PretendoWindow::messages::ENABLE_SQ1));
	fSquare2CheckBox = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), "square2_enable",
									 "Enabled", new BMessage(PretendoWindow::messages::ENABLE_SQ2));
	fTriangleCheckBox = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), "triangle_enable",
									  "Enabled", new BMessage(PretendoWindow::messages::ENABLE_TRI));
	fNoiseCheckBox = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), "noise_enable",
								   "Enabled", new BMessage(PretendoWindow::messages::ENABLE_NOISE));
	fDMCCheckBox = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), "dmc_enable",
								 "Enabled", new BMessage(PretendoWindow::messages::ENABLE_DMC));
								 
	fSquare1CheckBox->ResizeToPreferred();
	fSquare2CheckBox->ResizeToPreferred();
	fTriangleCheckBox->ResizeToPreferred();
	fNoiseCheckBox->ResizeToPreferred();
	fDMCCheckBox->ResizeToPreferred();
	
	fSquare1CheckBox->SetViewColor(kHeaderColor);
	fSquare2CheckBox->SetViewColor(kHeaderColor);
	fTriangleCheckBox->SetViewColor(kHeaderColor);
	fNoiseCheckBox->SetViewColor(kHeaderColor);
	fDMCCheckBox->SetViewColor(kHeaderColor);
		
	AddChild(fSquare1CheckBox);
	AddChild(fSquare2CheckBox);
	AddChild(fTriangleCheckBox);
	AddChild(fNoiseCheckBox);
	AddChild(fDMCCheckBox);
	
	
	if (!HasROMLoaded()) {
		fSquare1CheckBox->Hide();
		fSquare2CheckBox->Hide();
		fTriangleCheckBox->Hide();
		fNoiseCheckBox->Hide();
		fDMCCheckBox->Hide();

		fChannelControlsVisible = false;
	} else {
		fChannelControlsVisible = true;
	}
}


// -----------------------------------------------------------------------------
// APUStatusView::~APUStatusView
//
// Destroys the APU Status debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUStatusView::~APUStatusView()
{
}


// -----------------------------------------------------------------------------
// APUStatusView::AttachedToWindow
//
// Completes initialization after the view is attached to its window.  Enables
// periodic pulse updates, assigns the APU channel checkboxes to the window as
// their message target, positions the controls within their panel headers, and
// initializes their checked states from the current APU mute state.
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
	
	BWindow *window = Window();

	
	// -------------------------------------------------------------------------
	// Enable periodic updates.
	// -------------------------------------------------------------------------
	
	window->SetPulseRate(kPulseRate);

	
	// -------------------------------------------------------------------------
	// Set checkbox targets.
	// -------------------------------------------------------------------------

	if (fSquare1CheckBox) {
		fSquare1CheckBox->SetTarget(window);
	}

	if (fSquare2CheckBox) {
		fSquare2CheckBox->SetTarget(window);
	}

	if (fTriangleCheckBox) {
		fTriangleCheckBox->SetTarget(window);
	}

	if (fNoiseCheckBox) {
		fNoiseCheckBox->SetTarget(window);
	}

	if (fDMCCheckBox) {
		fDMCCheckBox->SetTarget(window);
	}


	// -------------------------------------------------------------------------
	// Compute the panel geometry used for checkbox placement.
	//
	// Child views are positioned here, never from Draw().
	// -------------------------------------------------------------------------

	const float topY = kTopMargin;
	const float overallHeight = PanelHeightForLines(9);
	const float squareHeight = PanelHeightForLines(15);
	const float topPanelHeight = overallHeight > squareHeight ? overallHeight : squareHeight;
	const float bottomY = topY + topPanelHeight + kSectionGap;


	// -------------------------------------------------------------------------
	// Position channel controls at the right side of their panel headers.
	// -------------------------------------------------------------------------

	if (fSquare1CheckBox) {
		fSquare1CheckBox->MoveTo(kColumn2X + 158.0f, topY + 1.0f); 
	}

	if (fSquare2CheckBox) {
		fSquare2CheckBox->MoveTo(kColumn3X + 158.0f, topY + 1.0f);
	}

	if (fTriangleCheckBox) {
		fTriangleCheckBox->MoveTo(kColumn1X + 178.0f, bottomY + 1.0f);
	}

	if (fNoiseCheckBox) {
		fNoiseCheckBox->MoveTo(kColumn2X + 158.0f, bottomY + 1.0f);
	}

	if (fDMCCheckBox) {
		fDMCCheckBox->MoveTo(kColumn3X + 158.0f, bottomY + 1.0f);
	}


	// -------------------------------------------------------------------------
	// Initialize checkbox values immediately.
	// -------------------------------------------------------------------------

	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();

	if (fSquare1CheckBox) {
		fSquare1CheckBox->SetValue(state.square1.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fSquare2CheckBox) {
		fSquare2CheckBox->SetValue(state.square2.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fTriangleCheckBox) {
		fTriangleCheckBox->SetValue(state.triangle.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fNoiseCheckBox) {
		fNoiseCheckBox->SetValue(state.noise.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fDMCCheckBox) {
		fDMCCheckBox->SetValue(state.dmc.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}
}


// -----------------------------------------------------------------------------
// APUStatusView::Pulse
//
// Periodically refreshes the APU Status view.  Channel controls are shown or
// hidden according to whether a ROM is loaded, and their checked states are
// synchronized with the current APU channel mute states.
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
	const bool romLoaded = HasROMLoaded();

	SetChannelControlsVisible(romLoaded);

	if (!romLoaded) {
		Invalidate();
		return;
	}

	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();

	if (fSquare1CheckBox) {
		fSquare1CheckBox->SetValue(state.square1.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fSquare2CheckBox) {
		fSquare2CheckBox->SetValue(state.square2.muted ? B_CONTROL_OFF : B_CONTROL_ON); 
	}

	if (fTriangleCheckBox) {
		fTriangleCheckBox->SetValue(state.triangle.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fNoiseCheckBox) {
		fNoiseCheckBox->SetValue(state.noise.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (fDMCCheckBox) {
		fDMCCheckBox->SetValue(state.dmc.muted ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// APUStatusView::PanelHeightForLines
//
// Calculates the total panel height required for the requested number of
// debugger text lines.
//
// Parameters:
//   lines - Number of text rows displayed inside the panel.
//
// Returns:
//   Required panel height in pixels.
// -----------------------------------------------------------------------------
float
APUStatusView::PanelHeightForLines (int32 lines)
{
	return kHeaderHeight + kPanelPadding + (lines * kLineHeight) + kPanelPadding;
}


// -----------------------------------------------------------------------------
// APUStatusView::SquareFrequencyHz
//
// Converts a NES pulse-channel timer period into its approximate output
// frequency in Hertz using the NTSC NES CPU clock.
//
// Pulse-channel frequency:
//
//   CPU clock / (16 * (timer period + 1))
//
// Parameters:
//   timerPeriod - Current 11-bit pulse-channel timer period.
//
// Returns:
//   Approximate pulse-channel output frequency in Hertz.
// -----------------------------------------------------------------------------
double
APUStatusView::SquareFrequencyHz (uint16 timerPeriod)
{
	return kNESClockRate / (16.0 * (static_cast<double>(timerPeriod) + 1.0));
}


// -----------------------------------------------------------------------------
// APUStatusView::TriangleFrequencyHz
//
// Converts a NES triangle-channel timer period into its approximate output
// frequency in Hertz using the NTSC NES CPU clock.
//
// Triangle-channel frequency:
//
//   CPU clock / (32 * (timer period + 1))
//
// The triangle channel uses a 32-step waveform sequence, which produces a
// divider twice as large as the pulse channels.
//
// Parameters:
//   timerPeriod - Current 11-bit triangle-channel timer period.
//
// Returns:
//   Approximate triangle-channel output frequency in Hertz.
// -----------------------------------------------------------------------------
double
APUStatusView::TriangleFrequencyHz (uint16_t timerPeriod)
{
	return kNESClockRate / (32.0 * (static_cast<double>(timerPeriod) + 1.0));
}


// -----------------------------------------------------------------------------
// APUStatusView::NoiseClockRateHz
//
// Converts the current NES noise-channel timer period into the approximate LFSR
// clock rate in Hertz using the NTSC NES CPU clock.
//
// Unlike the pulse and triangle channels, the noise channel does not produce a
// conventional pitched waveform.  The timer clocks its linear-feedback shift
// register, so this value represents the LFSR update rate rather than a musical
// output frequency.
//
// Parameters:
//   timerPeriod - Current noise timer period.
//
// Returns:
//   Approximate noise-channel LFSR clock rate in Hertz.
// -----------------------------------------------------------------------------
double
APUStatusView::NoiseClockRateHz (uint16 timerPeriod)
{
	if (timerPeriod == 0) {
		return 0.0;
	}

	return kNESClockRate / static_cast<double>(timerPeriod);
}


// -----------------------------------------------------------------------------
// APUStatusView::DMCBitRateHz
//
// Converts the current DMC timer period into the approximate DMC output bit rate
// using the NTSC NES CPU clock.
//
// The DMC output unit processes one sample bit whenever its timer expires, so
// this value represents the playback bit rate rather than an audio waveform
// frequency.
//
// Parameters:
//   timerPeriod - Current DMC timer period.
//
// Returns:
//   Approximate DMC playback bit rate in bits per second.
// -----------------------------------------------------------------------------
double
APUStatusView::DMCBitRateHz (uint16 timerPeriod)
{
	if (timerPeriod == 0) {
		return 0.0;
	}

	return kNESClockRate / static_cast<double>(timerPeriod);
}


// -----------------------------------------------------------------------------
// APUStatusView::SquareStateText
//
// Returns a short debugger description explaining the current output state of a
// pulse channel.
//
// The checks follow the same broad gating conditions used by the pulse-channel
// output path so the debugger can explain why a channel is silent without
// modifying emulation state.
//
// Parameters:
//   enabled        - true if the channel is enabled through APU status.
//   muted          - true if debugger/channel muting is active.
//   lengthCounter  - Current side-effect-free length-counter value.
//   sweepSilenced  - true if the sweep unit is currently silencing the channel.
//   timerFrequency - Current timer frequency/reload value used by the channel.
//   output         - Current side-effect-free channel output value.
//
// Returns:
//   Short textual description of the current channel state.
// -----------------------------------------------------------------------------
const char*
APUStatusView::SquareStateText (bool enabled, bool muted, uint8_t lengthCounter, 
								bool sweepSilenced, uint16_t timerFrequency, uint8_t output)
{
	if (!enabled) {
		return "DISABLED";
	}

	if (muted) {
		return "MUTED";
	}

	if (lengthCounter == 0) {
		return "LENGTH = 0";
	}

	if (timerFrequency < 9) {
		return "TIMER < 8";
	}

	if (sweepSilenced) {
		return "SWEEP";
	}

	if (output == 0) {
		return "DUTY = 0";
	}

	return "ACTIVE";
}


// -----------------------------------------------------------------------------
// TriangleStateText
//
// Returns a short debugger description explaining the current output state of
// the triangle channel.
//
// The triangle channel requires both its length counter and linear counter to be
// non-zero before it can actively produce its waveform.  This helper reports the
// first condition currently preventing output.
//
// Parameters:
//   enabled       - true if the triangle channel is enabled through APU status.
//   muted         - true if debugger/channel muting is active.
//   lengthCounter - Current side-effect-free length-counter value.
//   linearCounter - Current triangle linear-counter value.
//
// Returns:
//   Short textual description of the current triangle-channel state.
// -----------------------------------------------------------------------------
const char*
APUStatusView::TriangleStateText (bool enabled, bool muted, uint8 lengthCounter, uint8 linearCounter)
{
	if (!enabled) {
		return "DISABLED";
	}

	if (muted) {
		return "MUTED";
	}

	if (lengthCounter == 0) {
		return "LENGTH = 0";
	}

	if (linearCounter == 0) {
		return "LINEAR = 0";
	}

	return "ACTIVE";
}


// -----------------------------------------------------------------------------
// APUStatusView::NoiseStateText
//
// Returns a short debugger description explaining the current output state of
// the noise channel.
//
// The noise channel requires a non-zero length counter and envelope volume.  Its
// LFSR also gates the instantaneous output, so a zero output after the other
// conditions have been checked indicates that the current LFSR state is
// suppressing the channel output.
//
// Parameters:
//   enabled        - true if the noise channel is enabled through APU status.
//   muted          - true if debugger/channel muting is active.
//   lengthCounter  - Current side-effect-free length-counter value.
//   envelopeVolume - Current envelope output volume.
//   output         - Current side-effect-free noise-channel output value.
//
// Returns:
//   Short textual description of the current noise-channel state.
// -----------------------------------------------------------------------------
const char*
APUStatusView::NoiseStateText (bool enabled, bool muted, uint8 lengthCounter, uint8 envelopeVolume, uint8 output)
{
	if (!enabled) {
		return "DISABLED";
	}

	if (muted) {
		return "MUTED";
	}

	if (lengthCounter == 0) {
		return "LENGTH = 0";
	}

	if (envelopeVolume == 0) {
		return "VOLUME = 0";
	}

	if (output == 0) {
		return "LFSR GATE";
	}

	return "ACTIVE";
}


// -----------------------------------------------------------------------------
// APUStatusView::DMCStateText
//
// Returns a short debugger description explaining the current state of the DMC
// channel.
//
// The DMC channel is considered active while sample bytes remain to be fetched.
// A debugger mute takes priority over all other states.  If the channel is no
// longer active, the sample has completed or has not been started.  A temporarily
// empty sample buffer can also be reported while playback is still active.
//
// Parameters:
//   active            - true if DMC sample bytes remain to be processed.
//   muted             - true if debugger/channel muting is active.
//   sampleBufferEmpty - true if the DMC sample buffer is currently empty.
//   bytesRemaining    - Number of sample bytes still remaining.
//
// Returns:
//   Short textual description of the current DMC-channel state.
// -----------------------------------------------------------------------------
const char*
APUStatusView::DMCStateText (bool active, bool muted, bool sampleBufferEmpty, uint16 bytesRemaining)
{
	if (muted) {
		return "MUTED";
	}

	if (!active) {
		return "INACTIVE";
	}

	if (sampleBufferEmpty && bytesRemaining != 0) {
		return "BUFFER EMPTY";
	}

	return "ACTIVE";
}


// -----------------------------------------------------------------------------
// APUStatusView::PanelContentY
//
// Returns the baseline position of the first text row inside a debugger panel.
//
// Parameters:
//   panel - Panel rectangle.
//
// Returns:
//   Y coordinate for the first text baseline.
// -----------------------------------------------------------------------------
float
APUStatusView::PanelContentY (BRect panel)
{
	return panel.top + kHeaderHeight + kPanelPadding + kLineHeight;
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawPanelBackground
//
// Draws the background, border, header bar, and title for one APU debugger
// information panel.
//
// Parameters:
//   rect  - Panel rectangle.
//   title - Panel title.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawPanelBackground (BRect rect, const char *title)
{
	SetHighColor(kPanelColor);
	FillRect(rect);

	SetHighColor(kPanelBorderColor);
	StrokeRect(rect);

	BRect headerRect = rect;
	headerRect.bottom = headerRect.top + kHeaderHeight;

	SetHighColor(kHeaderColor);
	FillRect(headerRect);

	SetHighColor(kPanelBorderColor);
	StrokeLine(BPoint(headerRect.left, headerRect.bottom), BPoint(headerRect.right, headerRect.bottom));

	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetFace(B_BOLD_FACE);
	SetFont(&font);

	font_height fh;
	font.GetHeight(&fh);

	const float baseline = headerRect.top + ((kHeaderHeight - (fh.ascent + fh.descent)) * 0.5f) + fh.ascent;
	SetHighColor(kHeaderTextColor);
	DrawString(title, BPoint(headerRect.left + kPanelPadding, baseline));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawSectionHeader
//
// Draws a bold section heading.
//
// This helper is retained for future debugger layout work even though the
// current panel layout draws its primary section titles through
// DrawPanelBackground().
//
// Parameters:
//   text - Header text.
//   x    - Horizontal drawing position.
//   y    - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawSectionHeader (const char *text, float x, float y)
{
	if (!text) {
		return;
	}

	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetFace(B_BOLD_FACE);

	SetFont(&font);
	SetHighColor(kHeaderTextColor);
	DrawString(text, BPoint(x, y));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawTextLine
//
// Draws one label/value pair using the standard APU debugger column layout.
//
// Parameters:
//   label - Description shown on the left.
//   value - Value shown on the right.
//   x     - Horizontal starting position.
//   y     - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawTextLine (const char *label, const char *value, float x, float y)
{
	if (!label || !value) {
		return;
	}

	const float valueX = x + 108.0f;

	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	SetHighColor(kValueColor);
	DrawString(value, BPoint(valueX, y));
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawBoolLine
//
// Draws one boolean label/value pair.  Active values are highlighted while
// inactive values use the debugger's subdued state color.
//
// Parameters:
//   label - Description shown on the left.
//   value - Boolean state to display.
//   x     - Horizontal starting position.
//   y     - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawBoolLine (const char *label, bool value, float x, float y)
{
	if (!label) {
		return;
	}

	const float valueX = x + 108.0f;

	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	SetHighColor(value ? kEnabledColor : kDisabledColor);
	DrawString(value ? "YES" : "NO", BPoint(valueX, y));
}


// -----------------------------------------------------------------------------
// DrawFixedTextLine
//
// Draws a label/value row using the normal UI font for the label and the
// system fixed-width font for the value.  This is used for hexadecimal and
// other values that benefit from consistent character spacing.
//
// Parameters:
//   view  - View on which to draw the text.
//   label - Label text shown in the normal UI font.
//   value - Value text shown in the fixed-width font.
//   x     - Left position of the label.
//   y     - Baseline position of the row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawFixedTextLine (const char *label, const char *value, float x, float y)
{
	if (!label || !value) {
		return;
	}
	
	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	BFont prevFont;
	GetFont(&prevFont);

	SetFont(be_fixed_font);
	SetHighColor(kValueColor);
	DrawString(value, BPoint(x + 108.0f, y));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUStatusView::SetChannelControlsVisible
//
// Shows or hides the five per-channel APU controls.  The function tracks the
// current visibility state so repeated requests for the same state do not
// issue additional Show() or Hide() calls.
//
// Parameters:
//   visible - true to show the channel controls, false to hide them.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::SetChannelControlsVisible (bool visible)
{
	if (visible == fChannelControlsVisible) {
		return;
	}

	if (visible) {
		if (fSquare1CheckBox) {
			fSquare1CheckBox->Show();
		}

		if (fSquare2CheckBox) {
			fSquare2CheckBox->Show();
		}

		if (fTriangleCheckBox) {
			fTriangleCheckBox->Show();
		}

		if (fNoiseCheckBox) {
			fNoiseCheckBox->Show();
		}

		if (fDMCCheckBox) {
			fDMCCheckBox->Show();
		}
	} else {
		if (fSquare1CheckBox) {
			fSquare1CheckBox->Hide();
		}

		if (fSquare2CheckBox) {
			fSquare2CheckBox->Hide();
		}

		if (fTriangleCheckBox) {
			fTriangleCheckBox->Hide();
		}

		if (fNoiseCheckBox) {
			fNoiseCheckBox->Hide();
		}

		if (fDMCCheckBox) {
			fDMCCheckBox->Hide();
		}
	}

	fChannelControlsVisible = visible;
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawAPUPanel
//
// Draws the overall APU status panel.  Displays the current APU cycle and frame
// sequencer state, reconstructed $4015 status value, frame and DMC interrupt
// flags, and the global audio mute state.
//
// Parameters:
//   panel - Rectangle defining the bounds of the APU status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawAPUPanel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);
	
	value.SetToFormat("%llu", static_cast<unsigned long long>(state.cycle >= 2 ? state.cycle - 2 : 0));
	DrawTextLine("Cycle", value.String(), x, y);
	y += kLineHeight;
	
	DrawBoolLine("5-Step Mode", state.five_step_mode, x, y);
	y += kLineHeight;

	DrawBoolLine("IRQ Inhibit", state.frame_irq_inhibit, x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.frame_step));
	DrawTextLine("Frame Step", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%llu", static_cast<unsigned long long>(state.next_frame_cycle));
	DrawTextLine("Next Frame", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("$%02X", static_cast<unsigned int>(state.status));
	DrawFixedTextLine("Status", value.String(), x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Frame IRQ", BPoint(x, y));

	SetHighColor(state.frame_irq ? kIRQColor : kDisabledColor);
	DrawString(state.frame_irq ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("DMC IRQ", BPoint(x, y));

	SetHighColor(state.dmc_irq ? kIRQColor : kDisabledColor);
	DrawString(state.dmc_irq ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	DrawBoolLine("Audio Mute", state.audio_muted, x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawSquare1Panel
//
// Draws the Square 1 channel status panel.  Displays channel enable and mute
// state, timer and frequency information, duty and sequence position, length
// and envelope state, sweep configuration, derived channel state, and current
// output level.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Square 1 status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawSquare1Panel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);

	DrawBoolLine("Enabled", state.square1.enabled, x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Muted", BPoint(x, y));

	SetHighColor(state.square1.muted ? kMutedColor : kDisabledColor);
	DrawString(state.square1.muted ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.timer_period));
	DrawTextLine("Timer", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%.2f Hz", state.square1.enabled ? SquareFrequencyHz(state.square1.timer_period) : 0.0);
	DrawTextLine("Frequency", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.duty));
	DrawTextLine("Duty", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.sequence_index));
	DrawTextLine("Sequence", value.String(), x, y);
	y += kLineHeight;
	
	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.length_counter));
	DrawTextLine("Length", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.envelope_volume));
	DrawTextLine("Envelope", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Enable", state.square1.sweep_enabled, x, y);
	y += kLineHeight;
 
	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.sweep_period));
	DrawTextLine("Sweep Period", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Negate", state.square1.sweep_negate, x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.sweep_shift));
	DrawTextLine("Sweep Shift", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Silent", state.square1.sweep_silenced, x, y);
	y += kLineHeight;

	DrawTextLine("State", SquareStateText(state.square1.enabled, state.square1.muted, state.square1.length_counter,
			state.square1.sweep_silenced, state.square1.timer_frequency, state.square1.output), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square1.output));
	DrawTextLine("Output", value.String(), x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawSquare2Panel
//
// Draws the Square 2 channel status panel.  Displays channel enable and mute
// state, timer and frequency information, duty and sequence position, length
// and envelope state, sweep configuration, derived channel state, and current
// output level.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Square 2 status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawSquare2Panel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);

	DrawBoolLine("Enabled", state.square2.enabled, x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Muted", BPoint(x, y));

	SetHighColor(state.square2.muted ? kMutedColor : kDisabledColor);
	DrawString(state.square2.muted ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.timer_period));
	DrawTextLine("Timer", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%.2f Hz", state.square2.enabled ? SquareFrequencyHz(state.square2.timer_period) : 0.0);
	DrawTextLine("Frequency", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.duty));
	DrawTextLine("Duty", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.sequence_index));
	DrawTextLine("Sequence", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.length_counter));
	DrawTextLine("Length", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.envelope_volume));
	DrawTextLine("Envelope", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Enable", state.square2.sweep_enabled, x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.sweep_period));
	DrawTextLine( "Sweep Period", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Negate", state.square2.sweep_negate, x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.sweep_shift));
	DrawTextLine("Sweep Shift", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Sweep Silent", state.square2.sweep_silenced, x, y);
	y += kLineHeight;

	DrawTextLine("State", SquareStateText(state.square2.enabled, state.square2.muted, state.square2.length_counter,
						  state.square2.sweep_silenced, state.square2.timer_frequency, state.square2.output), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.square2.output));
	DrawTextLine("Output", value.String(), x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawTrianglePanel
//
// Draws the Triangle channel status panel.  Displays channel enable and mute
// state, timer and frequency information, length and linear-counter state,
// sequence position, derived channel state, and current output level.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Triangle status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawTrianglePanel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);

	DrawBoolLine("Enabled", state.triangle.enabled, x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Muted", BPoint(x, y));
	
	SetHighColor(state.triangle.muted ? kMutedColor : kDisabledColor);
	DrawString(state.triangle.muted ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;


	value.SetToFormat("%u", static_cast<unsigned int>(state.triangle.timer_period));
	DrawTextLine("Timer", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%.2f Hz", state.triangle.enabled ? TriangleFrequencyHz(state.triangle.timer_period) : 0.0);
	DrawTextLine("Frequency", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.triangle.length_counter));
	DrawTextLine("Length", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.triangle.linear_counter));
	DrawTextLine("Linear", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.triangle.sequence_index));
	DrawTextLine("Sequence", value.String(), x, y);
	y += kLineHeight;

	DrawTextLine("State", TriangleStateText(state.triangle.enabled, state.triangle.muted,
											state.triangle.length_counter, state.triangle.linear_counter), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.triangle.output));
	DrawTextLine("Output", value.String(), x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawNoisePanel
//
// Draws the Noise channel status panel.  Displays channel enable and mute
// state, timer and clock information, length and envelope state, derived
// channel state, and current output level.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Noise status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawNoisePanel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);

	DrawBoolLine("Enabled", state.noise.enabled, x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Muted", BPoint(x, y));

	SetHighColor(state.noise.muted ? kMutedColor : kDisabledColor);
	DrawString(state.noise.muted ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.noise.enabled ? state.noise.timer_period : 0));
	DrawTextLine("Timer", value.String(), x, y);
	y += kLineHeight;
	
	value.SetToFormat("%.2f Hz", state.noise.enabled ? NoiseClockRateHz(state.noise.timer_period) : 0.0);
	DrawTextLine("Clock", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.noise.length_counter));
	DrawTextLine("Length", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.noise.envelope_volume));
	DrawTextLine("Envelope", value.String(), x, y);
	y += kLineHeight;
	
	DrawTextLine("State", NoiseStateText(state.noise.enabled, state.noise.muted, state.noise.length_counter,
										 state.noise.envelope_volume, state.noise.output), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.noise.output));
	DrawTextLine("Output", value.String(), x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawDMCPanel
//
// Draws the DMC channel status panel.  Displays channel activity and mute state,
// timer and bitrate information, IRQ and loop configuration, sample addressing
// and progress, sample-buffer state, derived channel state, and current output
// level.
//
// Parameters:
//   panel - Rectangle defining the bounds of the DMC status panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawDMCPanel (BRect panel)
{
	const nes::apu::apu_debug_state_t state = nes::apu::debug_state();
	BString value;
	
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);
	
	DrawBoolLine("Enabled", state.dmc.enabled, x, y);
	y += kLineHeight;

	DrawBoolLine("Active", state.dmc.active, x, y);
	y += kLineHeight;

	SetHighColor(kLabelColor);
	DrawString("Muted", BPoint(x, y));

	SetHighColor(state.dmc.muted ? kMutedColor : kDisabledColor);
	DrawString(state.dmc.muted ? "YES" : "NO", BPoint(x + 108.0f, y));
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.dmc.active ? state.dmc.timer_period : 0));
	DrawTextLine("Timer", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%.2f Hz", state.dmc.active ? DMCBitRateHz(state.dmc.timer_period) : 0.0);
	DrawTextLine("Bitrate", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("IRQ enable", state.dmc.irq_enabled, x, y);
	y += kLineHeight;

	DrawBoolLine("Loop", state.dmc.loop, x, y);
	y += kLineHeight;

	value.SetToFormat("$%04X", static_cast<unsigned int>(state.dmc.active ? state.dmc.sample_address : 0));
	DrawFixedTextLine("Sample Address", value.String(), x, y);
	y += kLineHeight;
	
	value.SetToFormat("$%04X", static_cast<unsigned int>(state.dmc.active ? state.dmc.current_address : 0));
	DrawFixedTextLine("Current Address", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.dmc.sample_length));
	DrawTextLine("Sample Length", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.dmc.bytes_remaining));
	DrawTextLine("Bytes Left", value.String(), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.dmc.bits_remaining));
	DrawTextLine("Bits Left", value.String(), x, y);
	y += kLineHeight;

	DrawBoolLine("Buffer Empty", state.dmc.sample_buffer_empty, x, y);
	y += kLineHeight;
	
	DrawTextLine("State", DMCStateText(state.dmc.active, state.dmc.muted, state.dmc.sample_buffer_empty,
									   state.dmc.bytes_remaining), x, y);
	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(state.dmc.output));
	DrawTextLine("Output", value.String(), x, y);
}


// -----------------------------------------------------------------------------
// APUStatusView::Draw
//
// Draws the complete APU debugger view.
//
// The view is arranged as two rows of panels:
//
//   Top:
//     APU        Square 1        Square 2
//
//   Bottom:
//     Triangle   Noise           DMC
//
// Channel-enable checkboxes are positioned in the panel headers after the panel
// geometry has been calculated.  The actual checked/unchecked state is kept
// synchronized with the APU channel mute state by Pulse().
//
// Parameters:
//   updateRect - Region requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::Draw(BRect updateRect)
{
	(void)updateRect;

	// -------------------------------------------------------------------------
	// No cartridge loaded.
	//
	// Do not display stale or meaningless APU state.  The channel checkboxes
	// are hidden by Pulse() while no ROM is present.
	// -------------------------------------------------------------------------

	if (!HasROMLoaded()) {
		DrawNoROMMessage(Bounds());
		return;
	}

	// =========================================================================
	// Panel geometry
	// =========================================================================

	const float topY = kTopMargin;
	const float overallHeight = PanelHeightForLines(9);
	const float squareHeight = PanelHeightForLines(15);
	const float topPanelHeight = overallHeight > squareHeight ? overallHeight : squareHeight;
	const float bottomY = topY + topPanelHeight + kSectionGap;
	const float lowerPanelHeight = PanelHeightForLines(15);
	
	const BRect apuPanel(kColumn1X, topY, kColumn1X + kAPUPanelWidth, topY + topPanelHeight);
	const BRect square1Panel(kColumn2X, topY, kColumn2X + kPanelWidth, topY + topPanelHeight);
	const BRect square2Panel(kColumn3X, topY, kColumn3X + kPanelWidth, topY + topPanelHeight);
	const BRect trianglePanel(kColumn1X, bottomY, kColumn1X + kAPUPanelWidth, bottomY + lowerPanelHeight);
	const BRect noisePanel(kColumn2X, bottomY, kColumn2X + kPanelWidth, bottomY + lowerPanelHeight);
	const BRect dmcPanel(kColumn3X, bottomY, kColumn3X + kPanelWidth, bottomY + lowerPanelHeight);

	// =========================================================================
	// Draw panel backgrounds
	// =========================================================================

	DrawPanelBackground(apuPanel, "APU");
	DrawPanelBackground(square1Panel, "Square 1"); 
	DrawPanelBackground(square2Panel, "Square 2");
	DrawPanelBackground(trianglePanel, "Triangle");
	DrawPanelBackground(noisePanel, "Noise");
	DrawPanelBackground(dmcPanel, "DMC");
	
	// =========================================================================
	// Draw channel panels
	// =========================================================================
	
	DrawAPUPanel(apuPanel);
	DrawSquare1Panel(square1Panel);
	DrawSquare2Panel(square2Panel);
	DrawTrianglePanel(trianglePanel);
	DrawNoisePanel(noisePanel);
	DrawDMCPanel(dmcPanel);
}


// -----------------------------------------------------------------------------
// APUStatusView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded.
// -----------------------------------------------------------------------------
bool
APUStatusView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// APUStatusView::DrawNoROMMessage
//
// Draws the APU debugger's empty state when no cartridge is loaded.
//
// Parameters:
//   panel - Bounds in which to center the empty-state message.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font(prevFont);
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect APU state.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(centerX - (StringWidth(title) * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - (StringWidth(detail) * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&prevFont);
}

