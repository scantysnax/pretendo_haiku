#include "ExpansionAudioStatusView.h"

#include <Font.h>
#include <String.h>
#include <Window.h>

#include "Cart.h"
#include "Nes.h"


namespace {

constexpr bigtime_t kPulseRate = 100000;

constexpr float kTopMargin = 14.0f;

constexpr float kColumn1X = 16.0f;
constexpr float kColumn2X = 306.0f;
constexpr float kColumn3X = 576.0f;

constexpr float kGlobalPanelWidth = 270.0f;
constexpr float kChannelPanelWidth = 250.0f;

constexpr float kLineHeight = 16.0f;
constexpr float kSectionGap = 14.0f;

constexpr float kPanelPadding = 10.0f;
constexpr float kHeaderHeight = 26.0f;

constexpr float kValueOffset = 108.0f;

const rgb_color kBackgroundColor = { 216, 216, 216, 255 };
const rgb_color kPanelColor = { 248, 248, 248, 255 };
const rgb_color kPanelBorderColor = { 170, 175, 180, 255 };
const rgb_color kHeaderColor = { 190, 190, 190, 255 };
const rgb_color kHeaderTextColor = { 35, 35, 35, 255 };
const rgb_color kLabelColor = { 60, 60, 60, 255 };
const rgb_color kValueColor = { 15, 15, 15, 255 };
const rgb_color kEnabledColor = { 30, 110, 45, 255 };
const rgb_color kDisabledColor = { 120, 120, 120, 255 };
const rgb_color kBadColor = { 170, 35, 35, 255 };


// -----------------------------------------------------------------------------
// PanelHeightForLines
//
// Calculates a panel height using the same geometry as the native APU Status
// view.
//
// Parameters:
//   lines - Number of status rows displayed inside the panel.
//
// Returns:
//   Required panel height.
// -----------------------------------------------------------------------------
float
PanelHeightForLines (size_t lines)
{
	return kHeaderHeight + (kPanelPadding * 2.0f) + (static_cast<float>(lines) * kLineHeight);
}


// -----------------------------------------------------------------------------
// PanelContentY
//
// Calculates the first text baseline inside a status panel.
//
// Parameters:
//   panel - Panel rectangle.
//
// Returns:
//   Baseline of the first status row.
// -----------------------------------------------------------------------------
float
PanelContentY (BRect panel)
{
	return
		panel.top + kHeaderHeight + kPanelPadding + kLineHeight;
}

}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::ExpansionAudioStatusView
//
// Creates the expansion-audio status debugger view.
//
// The expansion page intentionally follows the same visual language as the
// native APU Status view so both tabs appear to belong to one debugger.
//
// Parameters:
//   frame - Initial bounds of the view.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioStatusView::ExpansionAudioStatusView (BRect frame)
	: BView(frame, "expansion_audio_status_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::~ExpansionAudioStatusView
//
// Destroys the expansion-audio status debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioStatusView::~ExpansionAudioStatusView()
{
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::AttachedToWindow
//
// Completes initialization after the view is attached to its window.
//
// The view enables periodic refreshes and captures the initial mapper-provided
// expansion-audio debugger state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Window() != nullptr) {
		Window()->SetPulseRate(kPulseRate);
	}

	CaptureState();
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::CaptureState
//
// Captures the current debugger-facing expansion-audio state from the loaded
// cartridge mapper.
//
// The cached state is cleared before every capture so state belonging to a
// previously loaded mapper cannot remain visible.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::CaptureState()
{
	fState = {};

	if (nes::cart.mapper() != nullptr) {
		fState = nes::cart.mapper()->debug_audio_state();
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::Pulse
//
// Periodically refreshes the mapper expansion-audio state and redraws the view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::Pulse()
{
	CaptureState();
	Invalidate();
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::Draw
//
// Draws the expansion-audio status page.
//
// Panel positions, widths, margins, row spacing, and header geometry mirror the
// native APU Status view:
//
//   Top:
//     Device      Channel 1      Channel 2
//
//   Bottom:
//     Channel 3
//
// Additional expansion-audio channel types can later extend this arrangement
// without changing the mapper-facing debugger interface.
//
// Parameters:
//   updateRect - Region requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(kBackgroundColor);
	FillRect(Bounds());

	if (fState.channel_count == 0) {
		SetHighColor(kDisabledColor);

		const char *message = "No expansion audio available.";
		const float width = StringWidth(message);
		DrawString(message, BPoint((Bounds().Width() - width) * 0.5f, Bounds().Height() * 0.5f));
		return;
	}

	/*
	 * Match the native APU Status page's three-column geometry.
	 */
	const size_t globalLines = fState.field_count;

	size_t channel1Lines = 0;
	size_t channel2Lines = 0;
	size_t channel3Lines = 0;

	if (fState.channel_count > 0) {
		channel1Lines = 4 + fState.channels[0].field_count;
	}

	if (fState.channel_count > 1) {
		channel2Lines = 4 + fState.channels[1].field_count;
	}

	if (fState.channel_count > 2) {
		channel3Lines = 4 + fState.channels[2].field_count;
	}

	const float globalHeight = PanelHeightForLines(globalLines);
	const float channel1Height = PanelHeightForLines(channel1Lines);
	const float channel2Height = PanelHeightForLines(channel2Lines);
	float topPanelHeight = globalHeight;

	if (channel1Height > topPanelHeight) {
		topPanelHeight = channel1Height;
	}

	if (channel2Height > topPanelHeight) {
		topPanelHeight = channel2Height;
	}

	const float topY = kTopMargin;
	const float bottomY = topY + topPanelHeight + kSectionGap;

	const BRect globalPanel(kColumn1X, topY, kColumn1X + kGlobalPanelWidth, topY + topPanelHeight);
	DrawPanelBackground(globalPanel, fState.name.empty() ? "Expansion Audio" : fState.name.c_str());
	DrawGlobalPanel(globalPanel);

	if (fState.channel_count > 0) {
		const BRect channel1Panel(kColumn2X, topY, kColumn2X + kChannelPanelWidth, topY + topPanelHeight);
		
		DrawPanelBackground(channel1Panel, fState.channels[0].name.c_str());
		DrawChannelPanel(channel1Panel, fState.channels[0]);
	}

	if (fState.channel_count > 1) {
		const BRect channel2Panel(kColumn3X, topY, kColumn3X + kChannelPanelWidth, topY + topPanelHeight);

		DrawPanelBackground(channel2Panel, fState.channels[1].name.c_str());
		DrawChannelPanel(channel2Panel,fState.channels[1]);
	}

	if (fState.channel_count > 2) {
		const float channel3Height = PanelHeightForLines(channel3Lines);
		const BRect channel3Panel(kColumn1X, bottomY, kColumn1X + kGlobalPanelWidth, bottomY + channel3Height);

		DrawPanelBackground(channel3Panel, fState.channels[2].name.c_str());
		DrawChannelPanel(channel3Panel, fState.channels[2]);
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawHeader
//
// Retained for compatibility with the existing view interface.
//
// The expansion tab no longer draws a separate page heading because the native
// APU Status view also begins directly with its status panels.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawHeader()
{
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawPanelBackground
//
// Draws the background, border, header bar, and title for one expansion-audio
// status panel.
//
// The implementation mirrors APUStatusView::DrawPanelBackground().
//
// Parameters:
//   rect  - Panel rectangle.
//   title - Panel title.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawPanelBackground (BRect rect, const char *title)
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
	DrawString( title, BPoint(headerRect.left + kPanelPadding, baseline));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawTextLine
//
// Draws one label/value pair using the same column spacing and colors as the
// native APU Status view.
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
ExpansionAudioStatusView::DrawTextLine (const char *label, const char *value, float x, float y)
{
	if (!label || !value) {
		return;
	}

	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	SetHighColor(kValueColor);
	DrawString(value, BPoint(x + kValueOffset, y));
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawBooleanLine
//
// Draws one boolean label/value pair using the same presentation as the native
// APU Status view.
//
// Active values are green. Inactive values use the subdued status color.
//
// Parameters:
//   label - Description shown on the left.
//   value - Boolean state.
//   x     - Horizontal starting position.
//   y     - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawBooleanLine (const char *label, bool value, float x, float y)
{
	if (!label) {
		return;
	}

	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	SetHighColor(value ? kEnabledColor : kDisabledColor);
	DrawString(value ? "YES" : "NO", BPoint(x + kValueOffset, y));
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawColoredTextLine
//
// Draws a label/value pair using a caller-selected value color.
//
// The label placement still follows the native APU Status geometry.
//
// Parameters:
//   label - Description shown on the left.
//   value - Textual field value.
//   color - Value color.
//   x     - Horizontal starting position.
//   y     - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawColoredTextLine (const char *label, const char *value, rgb_color color, float x, float y)
{
	if (!label || !value) {
		return;
	}

	SetHighColor(kLabelColor);
	DrawString(label, BPoint(x, y));

	SetHighColor(color);
	DrawString(value, BPoint(x + kValueOffset, y));
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawLevelMeter
//
// Retained for compatibility with the current view interface.
//
// APU Status does not use graphical level meters, so the expansion page likewise
// leaves level visualization to the Explorer and Oscilloscope tools.
//
// Parameters:
//   rect    - Unused.
//   value   - Unused.
//   maximum - Unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawLevelMeter (BRect rect, uint32_t value, uint32_t maximum)
{
	(void)rect;
	(void)value;
	(void)maximum;
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawDebugField
//
// Draws one mapper-defined debugger field using native APU Status conventions.
//
// Ordinary numeric and textual values use the normal status value color.
// Booleans use green/gray YES/NO presentation. Hexadecimal values use the
// system fixed-width font.
//
// Parameters:
//   field - Mapper-provided debugger field.
//   x     - Horizontal starting position.
//   y     - Text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawDebugField (const mapper_audio_debug_field_t &field, float x, float y)
{
	if (!field.label) {
		return;
	}

	BString value;

	switch (field.type) {
		case mapper_audio_debug_field_type::Boolean:
			DrawBooleanLine(field.label, field.value != 0, x, y);
			break;

		case mapper_audio_debug_field_type::Hex:
		{
			value.SetToFormat("$%02lX", static_cast<unsigned long>(field.value));

			SetHighColor(kLabelColor);
			DrawString(field.label, BPoint(x, y));

			BFont prevFont;
			GetFont(&prevFont);

			SetFont(be_fixed_font);
			SetHighColor(kValueColor);
			DrawString(value.String(), BPoint(x + kValueOffset, y));

			SetFont(&prevFont);
		}
		break;

		case mapper_audio_debug_field_type::Text:
			DrawTextLine(field.label, field.text != nullptr ? field.text : "", x, y);
			break;

		case mapper_audio_debug_field_type::Output:
			value.SetToFormat("%lu", static_cast<unsigned long>(field.value));
			DrawTextLine(field.label, value.String(), x, y);
			break;

		case mapper_audio_debug_field_type::Percent:
		{
			double percent = 0.0;

			if (field.maximum != 0) {
				percent = (static_cast<double>(field.value) / static_cast<double>(field.maximum)) * 100.0;
			}

			value.SetToFormat("%.1f%%", percent);
			DrawTextLine(field.label, value.String(), x, y);
		}
		break;

		case mapper_audio_debug_field_type::Timing:
			DrawBooleanLine(field.label, field.value != 0, x, y);
			break;

		case mapper_audio_debug_field_type::Value:
		default:
			value.SetToFormat("%lu", static_cast<unsigned long>(field.value));
			DrawTextLine(field.label, value.String(), x, y);
			break;
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawGlobalPanel
//
// Draws mapper-wide expansion-audio status.
//
// The panel background and title are drawn by Draw() so this routine only draws
// the status rows.
//
// Parameters:
//   panel - Bounds of the mapper-wide panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawGlobalPanel (BRect panel)
{
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);

	for (size_t i = 0; i < fState.field_count; i++) {

		DrawDebugField(fState.fields[i], x, y);
		y += kLineHeight;
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::DrawChannelPanel
//
// Draws status for one mapper-provided expansion-audio channel.
//
// Common channel state is shown first, followed immediately by mapper-specific
// fields. The presentation follows the same compact label/value arrangement as
// the native APU channel panels.
//
// Parameters:
//   panel   - Bounds of the channel panel.
//   channel - Mapper-provided channel debugger state.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::DrawChannelPanel (BRect panel, const mapper_audio_channel_debug_t &channel)
{
	const float x = panel.left + kPanelPadding;
	float y = PanelContentY(panel);
	DrawBooleanLine("Enabled", channel.enabled, x, y);
	y += kLineHeight;

	BString value;
	value.SetToFormat("$%04X", static_cast<unsigned int>(channel.period));

	SetHighColor(kLabelColor);
	DrawString("Timer Period", BPoint(x, y));

	BFont prevFont;
	GetFont(&prevFont);

	SetFont(be_fixed_font);
	SetHighColor(kValueColor);
	DrawString(value.String(), BPoint(x + kValueOffset, y));
	SetFont(&prevFont);
	y += kLineHeight;

	if (channel.halted) {
		DrawColoredTextLine("Frequency", "Halted", kBadColor, x, y);
	} else if (channel.frequency_valid) {
		value.SetToFormat("%.1f Hz", channel.frequency_hz);
		DrawTextLine("Frequency", value.String(), x, y);
	} else {
		DrawTextLine("Frequency", "-", x, y);
	}

	y += kLineHeight;

	value.SetToFormat("%u", static_cast<unsigned int>(channel.output));
	DrawTextLine("Output", value.String(), x, y);
	y += kLineHeight;

	for (size_t i = 0; i < channel.field_count; i++) {
		DrawDebugField(channel.fields[i], x, y);
		y += kLineHeight;
	}
}
