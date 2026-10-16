#include "ExpansionAudioExplorerView.h"
#include "PretendoWindow.h"


namespace {
	const rgb_color kTextColor = { 25, 25, 25, 255 };
	const rgb_color kTimingColor = { 0, 80, 145, 255 };
	const rgb_color kOutputColor = { 150, 75, 0, 255 };
	const rgb_color kGoodColor = { 0, 120, 40, 255 };
	const rgb_color kBadColor = { 170, 35, 35, 255 };
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::ExpansionAudioExplorerView
//
// Creates the expansion-audio debugger view.
//
// Parameters:
//   frame  - Initial view bounds.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioExplorerView::ExpansionAudioExplorerView (BRect frame, PretendoWindow *parent)
	: BView(frame, "expansion_audio_explorer_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::~ExpansionAudioExplorerView
//
// Destroys the expansion-audio debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioExplorerView::~ExpansionAudioExplorerView()
{
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::AttachedToWindow
//
// Captures the initial mapper expansion-audio state after the view is attached
// to its window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	CaptureState();
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::CaptureState
//
// Captures the current debugger-facing expansion-audio state from the loaded
// cartridge mapper.
//
// The cached state is cleared before each capture so stale mapper information
// cannot remain visible after a cartridge is unloaded or replaced.
//
// If no mapper is currently loaded, the cleared state is retained.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::CaptureState()
{
	fState = {};

	if (nes::cart.mapper() != nullptr) {
		fState = nes::cart.mapper()->debug_audio_state();
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::Pulse
//
// Periodically refreshes the mapper expansion-audio snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::Pulse()
{
	CaptureState();
	Invalidate();
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::Draw
//
// Draws the expansion-audio header, mapper-wide state, and currently exposed
// mapper audio channels.
//
// The panel layout mirrors the Native APU Explorer:
//
//   Global / Channel 1 / Channel 2
//   Channel 3 / empty / empty
//
// This keeps the Native and Expansion tabs visually consistent while retaining
// mapper-provided debugger state.
//
// Parameters:
//   updateRect - Area of the view requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (fState.channel_count == 0) {
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
	BRect channel1Panel(col2X, row1Top, col2X + columnWidth, row1Bottom);
	BRect channel2Panel(col3X, row1Top, Bounds().right - margin, row1Bottom);
	BRect channel3Panel(col1X, row2Top, col1X + columnWidth, row2Bottom);
	DrawGlobalPanel(globalPanel);

	if (fState.channel_count > 0) {
		DrawChannelPanel(channel1Panel, fState.channels[0]);
	}

	if (fState.channel_count > 1) {
		DrawChannelPanel(channel2Panel, fState.channels[1]);
	}

	if (fState.channel_count > 2) {
		DrawChannelPanel(channel3Panel, fState.channels[2]);
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawHeaderPanel
//
// Draws the title and mapper audio type at the top of the Expansion tab.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 58.0f);
	::DrawDebugPanel(this, panel, "Expansion Audio");

	SetFontSize(11.0f);
	SetHighColor(35, 35, 35);

	BString text;
	text.SetToFormat("%s mapper-provided audio state", ExpansionTypeName());
	DrawString(text.String(), BPoint(panel.left + 8.0f, panel.top + 42.0f));
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawGlobalPanel
//
// Draws mapper-wide expansion-audio state.
//
// Mapper-provided global fields are displayed first. The current combined
// expansion-audio output is then shown together with an inline graphical meter,
// matching the Native APU Explorer presentation.
//
// Parameters:
//   panel - Rectangle defining the global expansion-audio panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::DrawGlobalPanel (BRect panel)
{
	BString title;
	title.SetToFormat("Global %s", fState.name.c_str());
	::DrawDebugPanel(this, panel, title.String());

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

	BString value;

	for (size_t i = 0; i < fState.field_count; ++i) {
		const mapper_audio_debug_field_t &field = fState.fields[i];

		if (field.label == nullptr) {
			continue;
		}

		if (field.type == mapper_audio_debug_field_type::Boolean) {
			DrawStateLine(field.label, field.value != 0, x, y);
		} else if (field.type == mapper_audio_debug_field_type::Hex) {
			value.SetToFormat("$%02X", static_cast<unsigned>(field.value));
			DrawFixedLine(field.label, value.String(), x, y);
		} else if (field.type == mapper_audio_debug_field_type::Text) {
			DrawFixedLine(field.label, field.text != nullptr ? field.text : "", x, y);
		} else if (field.type == mapper_audio_debug_field_type::Output) {
			value.SetToFormat("%u", static_cast<unsigned>(field.value));
			DrawColoredFixedLine(field.label, value.String(), kOutputColor, x, y);
		} else if (field.type == mapper_audio_debug_field_type::Percent) {
			double percent = 0.0;

			if (field.maximum != 0) {
				percent = (static_cast<double>(field.value) / static_cast<double>(field.maximum)) * 100.0;
			}

			value.SetToFormat("%.1f%%", percent);
			DrawColoredFixedLine(field.label, value.String(), kOutputColor, x, y);
		} else if (field.type == mapper_audio_debug_field_type::Timing) {
			value.SetToFormat("%s", field.value != 0 ? "On" : "Off");
			DrawColoredFixedLine(field.label, value.String(), kTimingColor, x, y);
		} else {
			value.SetToFormat("%u", static_cast<unsigned>(field.value));
			DrawFixedLine(field.label, value.String(), x, y);
		}

		y += lineH;
	}

	/*
	 * Combined expansion-audio output.
	 */
	value.SetToFormat("%u", static_cast<unsigned>(fState.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	BRect meterRect(panel.left + 155.0f, y - 8.0f, panel.right - 10.0f, y);
	DrawLevelMeter(meterRect, fState.output,  fState.output_maximum);
	
	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawChannelPanel
//
// Draws one generic mapper expansion-audio channel.
//
// Common channel state is shown first, including the current timer period,
// mapper-supplied frequency state, raw output level, and graphical output meter.
//
// The graphical meter is placed on the same row as Output so the channel-panel
// presentation matches the Native APU Explorer.
//
// Mapper-specific fields supplied through mapper_audio_channel_debug_t::fields[]
// are shown below.
//
// Parameters:
//   panel   - Rectangle defining the channel panel.
//   channel - Mapper-provided channel debugger snapshot.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::DrawChannelPanel (BRect panel, const mapper_audio_channel_debug_t &channel)
{
	::DrawDebugPanel(this, panel, channel.name.c_str());

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

	DrawStateLine("Enabled", channel.enabled, x, y);
	y += lineH;

	BString value;
	value.SetToFormat("%u", static_cast<unsigned>(channel.period));
	DrawColoredFixedLine("Timer Period", value.String(), kTimingColor, x, y);
	y += lineH;

	/*
	 * Mapper-supplied channel frequency.
	 */
	if (channel.halted) {
		DrawColoredFixedLine("Frequency", "Halted", kBadColor, x, y);
		y += lineH;
	} else if (channel.frequency_valid) {
		value.SetToFormat("%.1f Hz", channel.frequency_hz);
		DrawColoredFixedLine("Frequency", value.String(), kTimingColor, x, y);
		y += lineH;
	}

	/*
	 * Raw channel output and graphical meter.
	 */
	value.SetToFormat("%u", static_cast<unsigned>(channel.output));
	DrawColoredFixedLine("Output", value.String(), kOutputColor, x, y);

	BRect meterRect(panel.left + 155.0f, y - 8.0f, panel.right - 10.0f, y);
	DrawLevelMeter(meterRect, channel.output, channel.output_maximum);
	y += lineH;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 4.0f), BPoint(panel.right - 8.0f, y - 4.0f));
	y += lineH;

	/*
	 * Mapper-specific channel fields.
	 */
	for (size_t i = 0; i < channel.field_count; ++i) {
		const mapper_audio_debug_field_t &field = channel.fields[i];

		if (field.label == nullptr) {
			continue;
		}

		if (field.type == mapper_audio_debug_field_type::Boolean) {
			DrawStateLine(field.label, field.value != 0, x, y);
		} else if (field.type == mapper_audio_debug_field_type::Hex) {
			value.SetToFormat("$%02X", static_cast<unsigned>(field.value));
			DrawFixedLine(field.label, value.String(), x, y);
		} else if (field.type == mapper_audio_debug_field_type::Text) {
			DrawFixedLine(field.label, field.text != nullptr ? field.text : "", x, y);
		} else if (field.type == mapper_audio_debug_field_type::Output) {
			value.SetToFormat("%u", static_cast<unsigned>(field.value));
			DrawColoredFixedLine(field.label, value.String(), kOutputColor, x, y);
		} else if (field.type == mapper_audio_debug_field_type::Percent) {
			double percent = 0.0;

			if (field.maximum != 0) {
				percent = (static_cast<double>(field.value) /
					 	   static_cast<double>(field.maximum)) * 100.0;
			}

			value.SetToFormat("%.1f%%", percent);
			DrawColoredFixedLine(field.label, value.String(), kOutputColor, x, y);
		} else {
			value.SetToFormat("%u", static_cast<unsigned>(field.value));
			DrawFixedLine(field.label, value.String(), x, y);
		}

		y += lineH;
	}

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawLevelMeter
//
// Draws a compact horizontal level meter.
//
// The meter displays value relative to maximum. Values greater than maximum
// are clamped to the full meter width. A zero maximum produces an empty meter.
//
// Parameters:
//   rect    - Rectangle defining the meter bounds.
//   value   - Current level value.
//   maximum - Full-scale level value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::DrawLevelMeter (BRect rect, uint32_t value, uint32_t maximum)
{
	SetHighColor(105, 105, 105);
	StrokeRect(rect);

	BRect inner = rect;
	inner.InsetBy(1.0f, 1.0f);

	SetHighColor(195, 195, 195);
	FillRect(inner);

	if (maximum == 0 || value == 0) {
		return;
	}

	double level = static_cast<double>(value) / static_cast<double>(maximum);

	if (level > 1.0) {
		level = 1.0;
	}

	BRect fill = inner;
	fill.right = fill.left + (inner.Width() * level);

	SetHighColor(kOutputColor);
	FillRect(fill);
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawFixedLine
//
// Draws a fixed-width labeled value.
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
ExpansionAudioExplorerView::DrawFixedLine (const char *label, const char *value, float x, float y)
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
// ExpansionAudioExplorerView::DrawStateLine
//
// Draws a boolean channel state using the same semantic coloring as the native
// APU Explorer.
//
// Parameters:
//   label - Text describing the state.
//   value - Current boolean value.
//   x     - Horizontal drawing position.
//   y     - Vertical baseline position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioExplorerView::DrawStateLine (const char *label, bool value, float x, float y)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	BString labelText;
	labelText.SetToFormat ("%-18s ", label);
	SetHighColor(kTextColor);
	DrawString(labelText.String(), BPoint(x, y));

	const float valueX = x + StringWidth(labelText.String());

	SetHighColor(value ? kGoodColor : kBadColor);
	DrawString(value ? "Yes" : "No", BPoint(valueX, y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView::DrawColoredFixedLine
//
// Draws a fixed-width labeled value with a caller-selected value color.
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
ExpansionAudioExplorerView::DrawColoredFixedLine (const char *label, const char *value,
												  rgb_color color, float x, float y)
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
// ExpansionAudioExplorerView::ExpansionTypeName
//
// Returns a display name for the currently captured expansion-audio hardware.
//
// Parameters:
//   None.
//
// Returns:
//   Human-readable expansion-audio type name.
// -----------------------------------------------------------------------------
const char*
ExpansionAudioExplorerView::ExpansionTypeName() const
{
	switch (fState.type) {
		case ExpansionAudioType::VRC6:
			return "VRC6";

		case ExpansionAudioType::None:
		default:
			return "No";
	}
}
