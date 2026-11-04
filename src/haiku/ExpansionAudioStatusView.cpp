
#include "ExpansionAudioStatusView.h"

#include <CheckBox.h>
#include <Font.h>
#include <String.h>
#include <Window.h>

#include "Cart.h"
#include "Nes.h"


namespace {

constexpr bigtime_t kPulseRate = 100000;

constexpr uint32 kMessageExpansionChannelEnable = 'EXCE';

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
	return panel.top + kHeaderHeight + kPanelPadding + kLineHeight;
}

} // end anonymous namespace


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::ExpansionAudioStatusView
//
// Creates the expansion-audio status debugger view.
//
// One reusable Enabled checkbox is created for each possible mapper expansion
// audio channel.  Only controls corresponding to channels exposed by the active
// mapper are shown.
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

	for (size_t i = 0; i < 8; ++i) {
		BMessage *message = new BMessage(kMessageExpansionChannelEnable);
		message->AddInt32("channel", static_cast<int32>(i));

		BString name;
		name.SetToFormat("expansion_channel_%zu_enable", i);

		fChannelCheckBoxes[i] = new BCheckBox(BRect(0.0f, 0.0f, 0.0f, 0.0f), name.String(), "Enabled", message);
		fChannelCheckBoxes[i]->ResizeToPreferred();
		fChannelCheckBoxes[i]->SetViewColor(kHeaderColor);
		
		AddChild(fChannelCheckBoxes[i]);
		fChannelCheckBoxes[i]->Hide();
	}
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
// The expansion-channel checkboxes target this view directly. Their positions
// and checked states are then synchronized with the active mapper.
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

	for (size_t i = 0; i < 8; ++i) {
		if (fChannelCheckBoxes[i] != nullptr) {
			fChannelCheckBoxes[i]->SetTarget(this);
		}
	}

	CaptureState();
	UpdateChannelControls();
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
// ExpansionAudioStatusView::UpdateChannelControls
//
// Positions and synchronizes the mapper expansion-audio channel checkboxes.
//
// For devices with more than three channels, controls are positioned at the
// right side of the two three-channel panel rows. For one-to-three-channel
// devices, the original expansion-audio panel geometry is preserved.
//
// A checked control means the channel is audible. An unchecked control means
// the debugger has muted that mapper channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::UpdateChannelControls()
{
	for (size_t i = 0; i < 8; ++i) {
		if (fChannelCheckBoxes[i] == nullptr) {
			continue;
		}

		if (i >= fState.channel_count) {
			fChannelCheckBoxes[i]->Hide();
			continue;
		}

		fChannelCheckBoxes[i]->SetValue(fState.channels[i].muted ? B_CONTROL_OFF : B_CONTROL_ON);

		if (fChannelCheckBoxes[i]->IsHidden()) {
			fChannelCheckBoxes[i]->Show();
		}
	}

	if (fState.channel_count > 3) {
		const float globalHeight = PanelHeightForLines(fState.field_count);
		size_t maximumChannelLines = 0;
		const size_t channelCount = std::min<size_t>(fState.channel_count, 6);

		for (size_t i = 0; i < channelCount; ++i) {
			const size_t lines = 4 + fState.channels[i].field_count;

			if (lines > maximumChannelLines) {
				maximumChannelLines = lines;
			}
		}

		const float channelHeight = PanelHeightForLines(maximumChannelLines);
		const float row1Y = kTopMargin + globalHeight + kSectionGap;
		const float row2Y = row1Y + channelHeight + kSectionGap;
		const float columnX[3] = {
			kColumn1X,
			kColumn2X,
			kColumn3X
		};

		const float panelWidth[3] = {
			kGlobalPanelWidth,
			kChannelPanelWidth,
			kChannelPanelWidth
		};

		for (size_t i = 0; i < channelCount; ++i) {
			const size_t column = i % 3;
			const size_t row = i / 3;

			BCheckBox *checkBox = fChannelCheckBoxes[i];

			if (checkBox == nullptr) {
				continue;
			}

			const float x = columnX[column] + panelWidth[column] - checkBox->Bounds().Width() - kPanelPadding;
			const float y = (row == 0 ? row1Y : row2Y) + 1.0f;

			checkBox->MoveTo(x, y);
		}

		return;
	}

	/*
	 * Original VRC6-style layout.
	 */
	const size_t globalLines = fState.field_count;
	size_t channel1Lines = 0;
	size_t channel2Lines = 0;

	if (fState.channel_count > 0) {
		channel1Lines = 4 + fState.channels[0].field_count;
	}

	if (fState.channel_count > 1) {
		channel2Lines = 4 + fState.channels[1].field_count;
	}

	float topPanelHeight = PanelHeightForLines(globalLines);
	const float channel1Height = PanelHeightForLines(channel1Lines);
	const float channel2Height = PanelHeightForLines(channel2Lines);

	if (channel1Height > topPanelHeight) {
		topPanelHeight = channel1Height;
	}

	if (channel2Height > topPanelHeight) {
		topPanelHeight = channel2Height;
	}

	const float bottomY = kTopMargin + topPanelHeight + kSectionGap;

	if (fState.channel_count > 0) {
		BCheckBox *checkBox = fChannelCheckBoxes[0];
		checkBox->MoveTo(kColumn2X + kChannelPanelWidth - checkBox->Bounds().Width() - kPanelPadding,
						 kTopMargin + 1.0f);
	}

	if (fState.channel_count > 1) {
		BCheckBox *checkBox = fChannelCheckBoxes[1];
		checkBox->MoveTo(kColumn3X + kChannelPanelWidth - checkBox->Bounds().Width() - kPanelPadding,
						 kTopMargin + 1.0f);
	}

	if (fState.channel_count > 2) {
		BCheckBox *checkBox = fChannelCheckBoxes[2];
		checkBox->MoveTo(kColumn1X + kGlobalPanelWidth - checkBox->Bounds().Width() - kPanelPadding,
						 bottomY + 1.0f);
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
	UpdateChannelControls();
	Invalidate();
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::MessageReceived
//
// Handles expansion-audio channel Enabled controls.
//
// The UI presents the generic mapper mute state as an Enabled checkbox:
// checked means audible and unchecked means muted.
//
// Channel indices are validated against both the debugger control capacity and
// the currently active mapper so stale UI messages cannot target channels that
// do not exist after a cartridge change.
//
// Parameters:
//   message - Incoming Application Kit message.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioStatusView::MessageReceived (BMessage *message)
{
	if (message->what == kMessageExpansionChannelEnable) {
		int32 channel = -1;

		if (message->FindInt32("channel", &channel) != B_OK) {
			return;
		}

		if (channel < 0 || channel >= 8) {
			return;
		}

		Mapper *mapper = nes::cart.mapper();

		if (mapper == nullptr) {
			return;
		}

		if (static_cast<size_t>(channel) >= mapper->audio_channel_count()) {
			return;
		}

		BCheckBox *checkBox = fChannelCheckBoxes[channel];

		if (checkBox == nullptr) {
			return;
		}

		mapper->set_audio_channel_muted(static_cast<size_t>(channel), checkBox->Value() == B_CONTROL_OFF);

		CaptureState();
		UpdateChannelControls();
		Invalidate();

		return;
	}

	BView::MessageReceived(message);
}


// -----------------------------------------------------------------------------
// ExpansionAudioStatusView::Draw
//
// Draws the expansion-audio status page.
//
// Expansion-audio devices with up to three channels use the original layout
// shared with the native APU Status page:
//
//   Top:
//     Device      Channel 1      Channel 2
//
//   Bottom:
//     Channel 3
//
// Devices with more than three channels use a compact multi-channel layout.
// The mapper-wide device panel spans the full width at the top, followed by
// two rows of three channel panels. This allows six-channel devices such as
// VRC7 to fit within the existing APU Status window without increasing its
// size.
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


	// -------------------------------------------------------------------------
	// Six-channel layout
	// -------------------------------------------------------------------------

	if (fState.channel_count > 3) {
		const size_t globalLines = fState.field_count;
		const float globalHeight = PanelHeightForLines(globalLines);
		const float topY = kTopMargin;
		const float rightEdge = kColumn3X + kChannelPanelWidth;
		const BRect globalPanel(kColumn1X, topY, rightEdge, topY + globalHeight);

		DrawPanelBackground(globalPanel, fState.name.empty() ? "Expansion Audio" : fState.name.c_str());
		DrawGlobalPanel(globalPanel);

		/*
		 * Determine a common channel-panel height so all six
		 * panels remain aligned.
		 */
		size_t maximumChannelLines = 0;
		const size_t visibleChannelCount = std::min<size_t>(fState.channel_count, 6);

		for (size_t i = 0; i < visibleChannelCount; ++i) {
			const size_t lines = 4 + fState.channels[i].field_count;

			if (lines > maximumChannelLines) {
				maximumChannelLines = lines;
			}
		}

		const float channelHeight = PanelHeightForLines(maximumChannelLines);
		const float row1Y = globalPanel.bottom + kSectionGap;
		const float row2Y = row1Y + channelHeight + kSectionGap;
		const float columnX[3] = {
			kColumn1X,
			kColumn2X,
			kColumn3X
		};

		for (size_t i = 0; i < visibleChannelCount; ++i) {
			const size_t column = i % 3;
			const size_t row = i / 3;
			const float panelY = row == 0 ? row1Y : row2Y;
			const float panelWidth = column == 0 ? kGlobalPanelWidth : kChannelPanelWidth;
			const BRect channelPanel(columnX[column], panelY, columnX[column] + panelWidth, 
									 panelY + channelHeight);

			DrawPanelBackground(channelPanel, fState.channels[i].name.c_str());
			DrawChannelPanel(channelPanel, fState.channels[i]);
		}

		return;
	}


	// -------------------------------------------------------------------------
	// Original one-to-three-channel layout
	// -------------------------------------------------------------------------

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
		DrawChannelPanel(channel2Panel, fState.channels[1]);
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
		case MapperAudioDebugFieldType::Boolean:
			DrawBooleanLine(field.label, field.value != 0, x, y);
			break;

		case MapperAudioDebugFieldType::Hex:
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

		case MapperAudioDebugFieldType::Text:
			DrawTextLine(field.label, field.text != nullptr ? field.text : "", x, y);
			break;

		case MapperAudioDebugFieldType::Output:
			value.SetToFormat("%lu", static_cast<unsigned long>(field.value));
			DrawTextLine(field.label, value.String(), x, y);
			break;

		case MapperAudioDebugFieldType::Percent:
		{
			double percent = 0.0;

			if (field.maximum != 0) {
				percent = (static_cast<double>(field.value) / static_cast<double>(field.maximum)) * 100.0;
			}

			value.SetToFormat("%.1f%%", percent);
			DrawTextLine(field.label, value.String(), x, y);
		}
		break;

		case MapperAudioDebugFieldType::Timing:
			DrawBooleanLine(field.label, field.value != 0, x, y);
			break;

		case MapperAudioDebugFieldType::Value:
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
// fields. The header checkbox controls whether the channel is audible, while
// the Key On row reflects the mapper channel's current synthesis/key state.
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

	DrawBooleanLine("Key On", channel.enabled, x, y);
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

	for (size_t i = 0; i < channel.field_count; ++i) {
		DrawDebugField(channel.fields[i], x, y);
		y += kLineHeight;
	}
}

