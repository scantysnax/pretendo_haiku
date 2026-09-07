
#include "APUExplorerView.h"
#include "PretendoWindow.h"


namespace {
	const rgb_color kTextColor = { 25, 25, 25, 255 };
	const rgb_color kAddressColor = { 0, 70, 150, 255 };
	const rgb_color kValueColor = { 105, 35, 135, 255 };
	const rgb_color kTimingColor = { 0, 80, 145, 255 };
	const rgb_color kModeColor = { 0, 80, 145, 255 };
	const rgb_color kOutputColor = { 150, 75, 0, 255 };
	const rgb_color kGoodColor = { 0, 120, 40, 255 };
	const rgb_color kBadColor = { 170, 35, 35, 255 };
	const rgb_color kInhibitColor = { 155, 95, 0, 255 };
}


// -----------------------------------------------------------------------------
// APUExplorerView::APUExplorerView
//
// Creates the APU Explorer debugger view.
//
// Parameters:
//   frame  - Initial view bounds.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerView::APUExplorerView (BRect frame, PretendoWindow *parent)
	: BView(frame, "apu_explorer_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// APUExplorerView::~APUExplorerView
//
// Destroys the APU Explorer debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerView::~APUExplorerView()
{
}


// -----------------------------------------------------------------------------
// APUExplorerView::AttachedToWindow
//
// Initializes the APU Explorer after it has been attached to its window and
// captures the initial debugger state when a cartridge is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	if (HasROMLoaded()) {
		CaptureState();
	}
}


// -----------------------------------------------------------------------------
// APUExplorerView::CaptureState
//
// Captures the current raw programmed APU register state together with the
// current effective/internal APU debugger state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::CaptureState()
{
	fExplorerState = nes::apu::explorer_state();
	fDebugState = nes::apu::debug_state();
}


// -----------------------------------------------------------------------------
// APUExplorerView::Pulse
//
// Periodically refreshes the APU Explorer while a cartridge is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::Pulse()
{
	if (!HasROMLoaded()) {
		Invalidate();
		return;
	}

	CaptureState();
	Invalidate();
}


// -----------------------------------------------------------------------------
// APUExplorerView::Draw
//
// Draws the APU Explorer header and six debugger panels.
// 
// Parameters:
//   updateRect - Area of the view requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(4.0f, 70.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
		DrawNoROMMessage(panel);
		return;
	}

	const float margin = 4.0f;
	const float gap = 6.0f;
	const float top = 70.0f;
	
	const float availableWidth = Bounds().Width() - (margin * 2.0f) - (gap * 2.0f);
	const float columnWidth = availableWidth / 3.0f;
	
	const float col1X = margin;
	const float col2X = col1X + columnWidth + gap;
	const float col3X = col2X + columnWidth + gap;
	
	const float topRowHeight = 300.0f;
	const float bottomRowHeight = 350.0f;
	
	const float row1Top = top;
	const float row1Bottom = row1Top + topRowHeight;
	const float row2Top = row1Bottom + gap;
	const float row2Bottom = row2Top + bottomRowHeight;

	BRect globalPanel(col1X, row1Top, col1X + columnWidth, row1Bottom);
	BRect square1Panel(col2X, row1Top, col2X + columnWidth, row1Bottom);
	BRect square2Panel(col3X, row1Top, Bounds().right - margin, row1Bottom);
	BRect trianglePanel(col1X, row2Top, col1X + columnWidth, row2Bottom);
	BRect noisePanel(col2X, row2Top, col2X + columnWidth, row2Bottom);
	BRect dmcPanel(col3X, row2Top, Bounds().right - margin, row2Bottom);

	DrawGlobalPanel(globalPanel);
	
	DrawSquare1Panel(square1Panel);
	DrawSquare2Panel(square2Panel);

	DrawTrianglePanel(trianglePanel);
	DrawNoisePanel(noisePanel);
	DrawDMCPanel(dmcPanel);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawHeaderPanel
//
// Draws the title panel for the APU Explorer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 58.0f);
	::DrawDebugPanel(this, panel, "APU Explorer");

	SetFontSize(11.0f);
	SetHighColor(35, 35, 35);
	DrawString("Raw register programming and effective channel state",
			   BPoint(panel.left + 8.0f, panel.top + 42.0f));
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawSquare1Panel
//
// Draws the Square 1 register and effective channel-state panel.
//
// The upper portion shows the most recently programmed raw values for $4000
// through $4003.  The lower portion shows the current effective state derived
// from the live APU channel.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Square 1 panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawSquare1Panel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Square 1");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$4000", "CTRL", fExplorerState.square1[0], x, y);
	y += lineH;

	DrawRegisterLine("$4001", "SWEEP", fExplorerState.square1[1], x, y);
	y += lineH;

	DrawRegisterLine("$4002", "TIMER L", fExplorerState.square1[2], x, y);
	y += lineH;

	DrawRegisterLine("$4003", "TIMER H", fExplorerState.square1[3], x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	DrawStateLine("Enabled", fDebugState.square1.enabled, true, x, y);
	y += lineH;

	DrawStateLine("Muted", fDebugState.square1.muted, false, x, y);
	y += lineH;

	BString value;
	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.timer_period));
	DrawFixedLine("Timer Period", value.String(), x, y);
	y += lineH;

	const double frequency = kNTSCCPUClock / 
							 (16.0 * (static_cast<double>(fDebugState.square1.timer_period) + 1.0));
	value.SetToFormat("%.1f Hz", frequency);	
	DrawColoredFixedLine("Frequency", value.String(), kTimingColor, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.duty));
	DrawFixedLine("Duty", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.sequence_index));
	DrawFixedLine("Sequence", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.length_counter));
	DrawFixedLine("Length", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.envelope_volume));
	DrawFixedLine("Envelope", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%s Period:%u %s Shift:%u", fDebugState.square1.sweep_enabled ? "On" : "Off",
					  static_cast<unsigned>(fDebugState.square1.sweep_period),
					  fDebugState.square1.sweep_negate ? "Down" : "Up",
					  static_cast<unsigned>(fDebugState.square1.sweep_shift));
	DrawFixedLine("Sweep", value.String(), x, y);
	y += lineH;

	DrawStateLine("Sweep Silent", fDebugState.square1.sweep_silenced, false, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square1.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawSquare2Panel
//
// Draws the Square 2 register and effective channel-state panel.
//
// The upper portion shows the most recently programmed raw values for $4004
// through $4007.  The lower portion shows the current effective state derived
// from the live APU channel.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Square 2 panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawSquare2Panel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Square 2");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$4004", "CTRL", fExplorerState.square2[0], x, y);
	y += lineH;

	DrawRegisterLine("$4005", "SWEEP", fExplorerState.square2[1], x, y);
	y += lineH;

	DrawRegisterLine("$4006", "TIMER L", fExplorerState.square2[2], x, y);
	y += lineH;

	DrawRegisterLine("$4007", "TIMER H", fExplorerState.square2[3], x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	DrawStateLine("Enabled", fDebugState.square2.enabled, true, x, y);
	y += lineH;

	DrawStateLine("Muted", fDebugState.square2.muted, false, x, y);
	y += lineH;

	BString value;
	
	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.timer_period));
	DrawFixedLine("Timer Period", value.String(), x, y);
	y += lineH;

	const double frequency = kNTSCCPUClock /
							 (16.0 * (static_cast<double>(fDebugState.square2.timer_period) + 1.0));
	value.SetToFormat("%.1f Hz", frequency);
	DrawColoredFixedLine("Frequency", value.String(), kTimingColor, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.duty));
	DrawFixedLine("Duty", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.sequence_index));
	DrawFixedLine("Sequence", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.length_counter));
	DrawFixedLine("Length", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.envelope_volume));
	DrawFixedLine("Envelope", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%s Period:%u %s Shift:%u", fDebugState.square2.sweep_enabled ? "On" : "Off",
												  static_cast<unsigned>(fDebugState.square2.sweep_period),
												  fDebugState.square2.sweep_negate ? "Down" : "Up",
												  static_cast<unsigned>(fDebugState.square2.sweep_shift));
	DrawFixedLine("Sweep", value.String(), x, y);
	y += lineH;

	DrawStateLine("Sweep Silent", fDebugState.square2.sweep_silenced, false, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.square2.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawTrianglePanel
//
// Draws the Triangle register and effective channel-state panel.
//
// The upper portion shows the most recently programmed raw values for $4008,
// $400A, and $400B.  The lower portion shows the current effective timer,
// length-counter, linear-counter, sequencer, and output state.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Triangle panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawTrianglePanel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Triangle");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$4008", "CTRL", fExplorerState.triangle0, x, y);
	y += lineH;

	DrawRegisterLine("$400A", "TIMER L", fExplorerState.triangle2, x, y);
	y += lineH;

	DrawRegisterLine("$400B", "TIMER H", fExplorerState.triangle3, x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	DrawStateLine("Enabled", fDebugState.triangle.enabled, true, x, y);
	y += lineH;

	DrawStateLine("Muted", fDebugState.triangle.muted, false, x, y);
	y += lineH;

	BString value;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.triangle.timer_period));
	DrawFixedLine("Timer Period", value.String(), x, y);
	y += lineH;

	const double frequency = kNTSCCPUClock /
							 (32.0 * (static_cast<double>(fDebugState.triangle.timer_period) + 1.0));
	value.SetToFormat("%.1f Hz", frequency);
	DrawColoredFixedLine("Frequency", value.String(), kTimingColor, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.triangle.length_counter));
	DrawFixedLine("Length", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.triangle.linear_counter));
	DrawFixedLine("Linear Counter", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.triangle.sequence_index));
	DrawFixedLine("Sequence", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.triangle.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawNoisePanel
//
// Draws the Noise register and effective channel-state panel.
//
// The upper portion shows the most recently programmed raw values for $400C,
// $400E, and $400F.  The lower portion shows the current effective timer,
// length-counter, envelope, and output state.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Noise panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawNoisePanel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Noise");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$400C", "CTRL", fExplorerState.noise0, x, y);
	y += lineH;

	DrawRegisterLine("$400E", "PERIOD", fExplorerState.noise2, x, y);
	y += lineH;

	DrawRegisterLine("$400F", "LENGTH", fExplorerState.noise3, x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	DrawStateLine("Enabled", fDebugState.noise.enabled, true, x, y);
	y += lineH;

	DrawStateLine("Muted", fDebugState.noise.muted, false, x, y);
	y += lineH;

	BString value;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.noise.timer_period));
	DrawFixedLine("Timer Period", value.String(), x, y);
	y += lineH;

	if (fDebugState.noise.timer_period != 0) {
		const double clockRate = kNTSCCPUClock / static_cast<double>(fDebugState.noise.timer_period);
		value.SetToFormat("%.1f Hz", clockRate);
	} else {
		value.SetTo("-");
	}
	
	DrawColoredFixedLine("Clock Rate", value.String(), kTimingColor, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.noise.length_counter));
	DrawFixedLine("Length", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.noise.envelope_volume));
	DrawFixedLine("Envelope", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.noise.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawDMCPanel
//
// Draws the DMC register and effective channel-state panel.
//
// The upper portion shows the most recently programmed raw values for $4010
// through $4013.  The lower portion shows the current effective DMC playback,
// timing, sample-address, progress, IRQ, loop, and output state.
//
// Parameters:
//   panel - Rectangle defining the bounds of the DMC panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawDMCPanel (BRect panel)
{
	::DrawDebugPanel(this, panel, "DMC");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$4010", "CTRL", fExplorerState.dmc[0], x, y);
	y += lineH;

	DrawRegisterLine("$4011", "LOAD", fExplorerState.dmc[1], x, y);
	y += lineH;

	DrawRegisterLine("$4012", "ADDR", fExplorerState.dmc[2], x, y);
	y += lineH;

	DrawRegisterLine("$4013", "LENGTH", fExplorerState.dmc[3], x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	DrawStateLine("Enabled", fDebugState.dmc.enabled, true, x, y);
	y += lineH;

	DrawStateLine("Active", fDebugState.dmc.active, true, x, y);
	y += lineH;

	DrawStateLine("Muted", fDebugState.dmc.muted, false, x, y);
	y += lineH;

	
	BString value;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.dmc.timer_period));
	DrawFixedLine("Timer Period", value.String(), x, y);
	y += lineH;

	if (fDebugState.dmc.timer_period != 0) {
		const double bitRate = kNTSCCPUClock / static_cast<double>(fDebugState.dmc.timer_period);

		value.SetToFormat("%.1f Hz", bitRate);
	} else {
		value.SetTo("-");
	}

	DrawColoredFixedLine("Bitrate", value.String(), kTimingColor, x, y);
	y += lineH;

	DrawStateLine("IRQ Enabled", fDebugState.dmc.irq_enabled, false, x, y);
	y += lineH;

	DrawStateLine("Loop", fDebugState.dmc.loop, true, x, y);
	y += lineH;

	value.SetToFormat("$%04X", static_cast<unsigned>(fDebugState.dmc.sample_address));
	DrawFixedLine("Sample Address", value.String(), x, y);
	y += lineH;

	value.SetToFormat("$%04X", static_cast<unsigned>(fDebugState.dmc.current_address));
	DrawFixedLine("Current Address", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u bytes", static_cast<unsigned>(fDebugState.dmc.sample_length));
	DrawFixedLine("Sample Length", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.dmc.bytes_remaining));
	DrawFixedLine("Bytes Remaining", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.dmc.bits_remaining));
	DrawFixedLine("Bits Remaining", value.String(), x, y);
	y += lineH;

	DrawStateLine("Buffer Empty", fDebugState.dmc.sample_buffer_empty, false, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.dmc.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawGlobalPanel
//
// Draws the global APU register and effective state panel.
//
// The upper portion shows the most recently programmed raw values for $4015 and
// $4017.  The lower portion shows the current frame-counter, interrupt, status,
// timing, and global audio state.
//
// Parameters:
//   panel - Rectangle defining the bounds of the Global APU panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawGlobalPanel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Global APU");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 42.0f;

	DrawRegisterLine("$4015", "STATUS", fExplorerState.status, x, y);
	y += lineH;

	DrawRegisterLine("$4017", "FRAME", fExplorerState.frame_counter, x, y);
	y += lineH + 5.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	BString value;

	value.SetToFormat("%llu", static_cast<unsigned long long>(fDebugState.cycle));
	DrawFixedLine("APU Cycle", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%s", fDebugState.five_step_mode ? "5-Step" : "4-Step");
	DrawColoredFixedLine("Frame Mode", value.String(), kModeColor, x, y);
	y += lineH;

	value.SetToFormat("%s", fDebugState.frame_irq_inhibit ? "Yes" : "No");
	DrawColoredFixedLine("IRQ Inhibit", value.String(), kInhibitColor, x, y);
	y += lineH;

	value.SetToFormat("%u", static_cast<unsigned>(fDebugState.frame_step));
	DrawFixedLine("Frame Step", value.String(), x, y);
	y += lineH;

	value.SetToFormat("%llu", static_cast<unsigned long long>(fDebugState.next_frame_cycle));
	DrawFixedLine("Next Frame", value.String(), x, y);
	y += lineH;

	value.SetToFormat("$%02X", static_cast<unsigned>(fDebugState.status));
	DrawFixedLine("APU Status", value.String(), x, y);
	y += lineH;

	DrawStateLine("Frame IRQ", fDebugState.frame_irq, false, x, y);
	y += lineH;

	DrawStateLine("DMC IRQ", fDebugState.dmc_irq, false, x, y);
	y += lineH;

	DrawStateLine("Audio Muted", fDebugState.audio_muted, false, x, y);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawRegisterLine
//
// Draws one raw APU register row.  The register address and raw value are
// colorized to distinguish programmed register state from derived channel
// information.
//
// Parameters:
//   address - CPU-visible APU register address text.
//   name    - Short register name.
//   value   - Most recently programmed raw register value.
//   x       - Horizontal drawing position.
//   y       - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawRegisterLine (const char *address, const char *name, uint8 value, float x, float y)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	BString addressText;
	BString nameText;
	BString valueText;

	addressText.SetToFormat("%-7s", address);
	nameText.SetToFormat("%-11s", name);
	valueText.SetToFormat("$%02X", value);

	float drawX = x;

	SetHighColor(kAddressColor);
	DrawString(addressText.String(), BPoint(drawX, y));
	drawX += StringWidth(addressText.String());

	SetHighColor(kTextColor);
	DrawString(nameText.String(), BPoint(drawX, y));
	drawX += StringWidth(nameText.String());

	SetHighColor(kValueColor);
	DrawString(valueText.String(), BPoint(drawX, y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawTextLine
//
// Draws a simple labeled text value using the standard Explorer text color.
// This helper is intended for neutral informational values that do not require
// fixed-column alignment or semantic color highlighting.
//
// Parameters:
//   label - Text describing the value.
//   value - Text representation of the value.
//   x     - Horizontal drawing position.
//   y     - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawTextLine (const char *label, const char *value, float x, float y)
{
	BString line;

	line.SetToFormat("%s: %s", label, value);

	SetHighColor(kTextColor);
	DrawString(line.String(), BPoint(x, y));
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawBoolLine
//
// Draws a labeled boolean value using semantic color highlighting.  The label
// is drawn in the standard Explorer text color, while the value is drawn green
// for true and red for false.
//
// Parameters:
//   label - Text describing the boolean state.
//   value - Boolean value to display as "Yes" or "No".
//   x     - Horizontal drawing position.
//   y     - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawBoolLine (const char *label, bool value, float x, float y)
{
	BString line;

	line.SetToFormat("%-18s", label);
	SetHighColor(kTextColor);
	DrawString(line.String(), BPoint(x, y));

	const float valueX = x + StringWidth(line.String());

	if (value) {
		SetHighColor(kGoodColor);
	} else {
		SetHighColor(kBadColor);
	}

	DrawString(value ? "Yes" : "No", BPoint(valueX, y));
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawFixedLine
//
// Draws a labeled value using a fixed-width font and aligned label field.  This
// helper is used for neutral Explorer information where consistent column
// alignment is desirable and no semantic color highlighting is required.
//
// Parameters:
//   label - Text describing the value.
//   value - Text representation of the value.
//   x     - Horizontal drawing position.
//   y     - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawFixedLine (const char *label, const char *value, float x, float y)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);

	SetFont(&fixed);
	SetHighColor(kTextColor);

	BString line;
	line.SetToFormat("%-18s %s", label, value);
	DrawString(line.String(), BPoint(x, y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawStateLine
//
// Draws a labeled boolean state using semantic coloring.  The label is drawn in
// the normal Explorer text color while the value is green for the desirable
// state and red for the undesirable state.
//
// Parameters:
//   label        - Text describing the state.
//   value        - Current boolean value.
//   goodWhenTrue - true when a true value represents the desirable state.
//   x            - Horizontal drawing position.
//   y            - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawStateLine (const char *label, bool value, bool goodWhenTrue, float x, float y)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	BString labelText;

	labelText.SetToFormat("%-18s ", label);
	SetHighColor(kTextColor);
	DrawString(labelText.String(), BPoint(x, y));

	const float valueX = x + StringWidth(labelText.String());
	const bool good = (value == goodWhenTrue);

	if (good) {
		SetHighColor(kGoodColor);
	} else {
		SetHighColor(kBadColor);
	}

	DrawString(value ? "Yes" : "No", BPoint(valueX, y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawColoredFixedLine
//
// Draws a fixed-width labeled value with the label in the normal Explorer text
// color and the value in a caller-selected color.
//
// Parameters:
//   label - Text describing the value.
//   value - Value text to display.
//   color - Color used for the value.
//   x     - Horizontal drawing position.
//   y     - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawColoredFixedLine (const char *label, const char *value, rgb_color color, float x, float y)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	BString labelText;

	labelText.SetToFormat("%-18s ", label);
	SetHighColor(kTextColor);
	DrawString(labelText.String(), BPoint(x, y));

	const float valueX = x + StringWidth(labelText.String());

	SetHighColor(color);
	DrawString(value, BPoint(valueX, y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// APUExplorerView::HasROMLoaded
//
// Reports whether a cartridge is currently loaded.
//
// Parameters:
//   None.
//
// Returns:
//   true if a cartridge mapper is present.
// -----------------------------------------------------------------------------
bool
APUExplorerView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// APUExplorerView::DrawNoROMMessage
//
// Draws the empty-state message shown when no cartridge is loaded.
//
// Parameters:
//   panel - Rectangle defining the message area.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerView::DrawNoROMMessage(BRect panel)
{
	BFont previousFont;
	GetFont(&previousFont);

	SetFont(be_plain_font);
	SetFontSize(12.0f);

	const char *title = "No ROM Loaded";
	const char *message = "Load a cartridge to explore APU register state.";

	const float titleWidth = StringWidth(title);
	const float messageWidth = StringWidth(message);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading);
	const float centerY = panel.top + (panel.Height() * 0.5f);
	const float titleY = centerY - (lineH * 0.5f);
	const float messageY = titleY + lineH + 8.0f;

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(panel.left + ((panel.Width() - titleWidth) * 0.5f), titleY));
	
	SetHighColor(120, 120, 120);
	DrawString(message, BPoint(panel.left + ((panel.Width() - messageWidth) * 0.5f), messageY));

	SetFont(&previousFont);
}

