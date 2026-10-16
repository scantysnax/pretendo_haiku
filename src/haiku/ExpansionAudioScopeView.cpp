#include "ExpansionAudioScopeView.h"

#include <algorithm>

#include "Cart.h"
#include "Nes.h"


namespace {

constexpr rgb_color kBackgroundColor = {216, 216, 216, 255};
constexpr rgb_color kTextColor = {20, 20, 20, 255};

constexpr rgb_color kTimingColor = {0, 80, 145, 255};
constexpr rgb_color kTriggerColor = {105, 35, 135, 255};
constexpr rgb_color kSoloOnColor = {0, 120, 40, 255};
constexpr rgb_color kSoloOffColor = {170, 35, 35, 255};


constexpr rgb_color kWaveColors[] = {
	{40, 90, 190, 255},
	{120, 65, 170, 255},
	{40, 145, 80, 255},
	{210, 120, 30, 255},
	{190, 55, 55, 255},
	{50, 50, 50, 255},
	{30, 140, 150, 255},
	{145, 90, 40, 255}
};

constexpr float kOuterMargin = 4.0f;
constexpr float kPanelGap = 4.0f;

}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::ExpansionAudioScopeView
//
// Constructs the expansion-audio oscilloscope view.
//
// Parameters:
//   frame - Initial view frame.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioScopeView::ExpansionAudioScopeView (BRect frame)
	: BView(frame, "expansion_audio_scope_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(kBackgroundColor);
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::~ExpansionAudioScopeView
//
// Destroys the expansion-audio oscilloscope view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ExpansionAudioScopeView::~ExpansionAudioScopeView()
{
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::AttachedToWindow
//
// Performs initial setup after the view is attached to its window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	CaptureSamples();
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::KeyDown
//
// Handles keyboard controls for the expansion-audio oscilloscope.
//
// Controls:
//   Space       - Toggle live/frozen display.
//   Left        - Move frozen cursor one sample earlier.
//   Right       - Move frozen cursor one sample later.
//   Page Up     - Move frozen cursor 16 samples earlier.
//   Page Down   - Move frozen cursor 16 samples later.
//   Home        - Move cursor to oldest captured sample.
//   End         - Move cursor to newest captured sample.
//   Z           - Cycle horizontal zoom level.
//   C           - Clear oscilloscope history.
//   T           - Cycle trigger source.
//   S           - Cycle Solo source.
//   1-4         - Hide/show expansion-audio panels.
//
// Parameters:
//   bytes    - Keyboard input bytes.
//   numBytes - Number of bytes supplied.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == ' ') {
		fFreezeUpdates = !fFreezeUpdates;

		if (fFreezeUpdates) {
			if (fSampleCount != 0) {
				fCursorSample = static_cast<int32>(fSampleCount) - 1;
			} else {
				fCursorSample = -1;
			}
		} else {
			fCursorSample = -1;
			CaptureSamples();
		}

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 'z' || bytes[0] == 'Z')) {
		switch (fVisibleSampleCount) {
			case 1024:
				fVisibleSampleCount = 512;
				break;

			case 512:
				fVisibleSampleCount = 256;
				break;

			case 256:
				fVisibleSampleCount = 128;
				break;

			default:
				fVisibleSampleCount = nes::apu::APU_SCOPE_SAMPLE_CAPACITY;
				break;
		}

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 'c' || bytes[0] == 'C')) {
		nes::apu::debug_clear_scope();

		fSampleCount = 0;
		fCursorSample = -1;

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 't' || bytes[0] == 'T')) {
		CycleTriggerMode();

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 's' || bytes[0] == 'S')) {
		CycleSoloMode();

		Invalidate();
		return;
	}

	if (numBytes == 1 && bytes[0] >= '1' && bytes[0] <= '4') {
		const size_t channel = static_cast<size_t>(bytes[0] - '1');
		fChannelVisible[channel] = !fChannelVisible[channel];

		Invalidate();
		return;
	}
	
	if (numBytes == 1 && (bytes[0] == 'a' || bytes[0] == 'A')) {
		for (size_t index = 0; index < 4; ++index) {
			fChannelVisible[index] = true;
		}

		fSoloChannel = -1;

		Invalidate();
		return;
	}

	if (fFreezeUpdates && fSampleCount != 0 && numBytes == 1) {
		switch (bytes[0]) {
			case B_LEFT_ARROW:
				fCursorSample--;
				ClampCursor();
				Invalidate();
				return;

			case B_RIGHT_ARROW:
				fCursorSample++;
				ClampCursor();
				Invalidate();
				return;

			case B_PAGE_UP:
				fCursorSample -= 16;
				ClampCursor();
				Invalidate();
				return;

			case B_PAGE_DOWN:
				fCursorSample += 16;
				ClampCursor();
				Invalidate();
				return;

			case B_HOME:
				fCursorSample = 0;
				Invalidate();
				return;

			case B_END:
				fCursorSample = static_cast<int32>(fSampleCount) - 1;
				Invalidate();
				return;
		}
	}

	BView::KeyDown(bytes, numBytes);
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::MouseDown
//
// Allows the trigger position to be moved horizontally with the mouse.
//
// When trigger mode is enabled, clicking inside the waveform area moves the
// trigger reference position to the clicked horizontal location. The position
// is stored as a normalized fraction of the waveform width so it applies
// consistently to all expansion-audio panels.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::MouseDown (BPoint where)
{
	if (!fTriggerEnabled) {
		BView::MouseDown(where);
		return;
	}

	const float left = kOuterMargin + 8.0f;
	const float right = Bounds().right - kOuterMargin - 8.0f;
	const float width = right - left;

	if (width <= 0.0f) {
		return;
	}

	if (where.x < left ||
		where.x > right) {
		return;
	}

	fTriggerPosition = (where.x - left) / width;

	if (fTriggerPosition < 0.0f) {
		fTriggerPosition = 0.0f;
	}

	if (fTriggerPosition > 1.0f) {
		fTriggerPosition = 1.0f;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::Pulse
//
// Refreshes the expansion-audio sample history while the scope is live.
//
// When display freeze is active, the stored waveform snapshot is left
// untouched. The emulator and backend scope capture continue running; only this
// debugger view stops accepting new snapshots.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::Pulse()
{
	if (!fFreezeUpdates) {
		CaptureSamples();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::CaptureSamples
//
// Copies the current shared APU oscilloscope history and mapper debugger state.
//
// The expansion channel samples were captured by the APU at the same 48 kHz
// instants as the native channel data.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::CaptureSamples()
{
	fSampleCount = nes::apu::debug_scope_snapshot(fSamples, nes::apu::APU_SCOPE_SAMPLE_CAPACITY);

	fDebugState = {};
	fChannelCount = 0;

	if (nes::cart.mapper() == nullptr) {
		return;
	}

	fDebugState = nes::cart.mapper()->debug_audio_state();

	const mapper_audio_scope_sample_t scopeState = nes::cart.mapper()->debug_audio_scope_sample();

	fChannelCount = std::min(scopeState.channel_count, nes::apu::APU_SCOPE_EXPANSION_CHANNEL_CAPACITY);
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::Draw
//
// Draws the expansion-audio oscilloscope.
//
// In normal mode, only channels currently marked visible are drawn. The
// remaining panels expand vertically to fill the available waveform area.
//
// When Solo is enabled, only the selected Solo channel is drawn and it occupies
// the complete waveform area. Solo takes precedence over normal visibility
// settings.
//
// Parameters:
//   updateRect - Invalidated region supplied by the Application Kit.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(kBackgroundColor);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (fChannelCount == 0) {
		SetHighColor(kTextColor);

		DrawString("No expansion-audio channels are available.", BPoint(16.0f, 122.0f));

		return;
	}

	const float top = 88.0f;
	const float bottom = Bounds().bottom - kOuterMargin;

	/*
	 * Solo mode.
	 *
	 * Mapper-provided channels use indices 0..fChannelCount-1.
	 * Expansion Total is represented by fChannelCount.
	 */
	if (fSoloChannel >= 0) {
		BRect panel(kOuterMargin, top, Bounds().right - kOuterMargin, bottom);

		/*
		 * Individual expansion channel.
		 */
		if (fSoloChannel < static_cast<int32>(fChannelCount)) {
			const size_t channel = static_cast<size_t>(fSoloChannel);

			const char *title = nullptr;
			float maximumValue = 255.0f;

			if (channel < fDebugState.channel_count) {
				title = fDebugState.channels[channel].name.c_str();

				if (fDebugState.channels[channel].output_maximum != 0) {
					maximumValue = static_cast<float>(fDebugState.channels[channel].output_maximum);
				}
			}

			BString fallbackTitle;

			if (title == nullptr ||
				title[0] == '\0') {

				fallbackTitle.SetToFormat("Expansion Channel %zu", channel + 1);
				title = fallbackTitle.String();
			}

			DrawWaveformPanel(panel, title, channel, maximumValue);

			return;
		}

		/*
		 * Expansion Total.
		 */
		if (fSoloChannel == static_cast<int32>(fChannelCount)) {
			DrawTotalWaveformPanel(panel);

			return;
		}
	}

	/*
	 * Normal non-Solo layout.
	 *
	 * Count the currently visible panels so the remaining panels can expand
	 * vertically to fill the scope area.
	 */
	size_t visiblePanelCount = 0;

	for (size_t channel = 0; channel < fChannelCount; ++channel) {
		if (channel < 3 && fChannelVisible[channel]) {
			visiblePanelCount++;
		}
	}

	if (fChannelVisible[3]) {
		visiblePanelCount++;
	}

	if (visiblePanelCount == 0) {
		SetHighColor(kTextColor);

		DrawString("All expansion-audio channels are hidden.", BPoint(16.0f, 122.0f));

		return;
	}

	const float availableHeight = bottom - top - (kPanelGap * static_cast<float>(visiblePanelCount - 1));
	const float panelHeight = availableHeight / static_cast<float>(visiblePanelCount);

	size_t visiblePanelIndex = 0;

	/*
	 * Mapper-provided individual channels.
	 */
	for (size_t channel = 0; channel < fChannelCount; ++channel) {
		if (channel >= 3 ||
			!fChannelVisible[channel]) {

			continue;
		}

		const float panelTop = top + static_cast<float>(visiblePanelIndex) *(panelHeight + kPanelGap);
		const float panelBottom = panelTop + panelHeight;

		BRect panel(kOuterMargin, panelTop, Bounds().right - kOuterMargin, panelBottom);

		const char *title = nullptr;
		float maximumValue = 255.0f;

		if (channel < fDebugState.channel_count) {
			title = fDebugState.channels[channel].name.c_str();

			if (fDebugState.channels[channel].output_maximum != 0) {
				maximumValue = static_cast<float>(fDebugState.channels[channel].output_maximum);
			}
		}

		BString fallbackTitle;

		if (title == nullptr || title[0] == '\0') {

			fallbackTitle.SetToFormat("Expansion Channel %zu", channel + 1);
			title = fallbackTitle.String();
		}

		DrawWaveformPanel(panel, title, channel, maximumValue);

		visiblePanelIndex++;
	}

	/*
	 * Expansion Total.
	 */
	if (fChannelVisible[3]) {
		const float totalPanelTop = top + static_cast<float>(visiblePanelIndex) * (panelHeight + kPanelGap);
		const float totalPanelBottom = totalPanelTop + panelHeight;

		BRect totalPanel(kOuterMargin, totalPanelTop, Bounds().right - kOuterMargin, totalPanelBottom);

		DrawTotalWaveformPanel(totalPanel);
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::DrawHeaderPanel
//
// Draws the expansion-audio oscilloscope header using the same layout and
// debugger-panel styling as the Native APU scope.
//
// The header displays keyboard controls, capture/zoom information, trigger
// state, Solo state, and the current LIVE/FROZEN state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 82.0f);

	::DrawDebugPanel(this, panel, "Expansion Audio Scope");

	BFont previousFont;
	GetFont(&previousFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);

	SetFont(&fixed);

	const float line1Y = panel.top + 38.0f;
	const float line2Y = line1Y + 15.0f;

	const float line3Y = line2Y + 15.0f;

	/*
	 * First help line.
	 */
	SetHighColor(kTextColor);
	DrawString("Space: Freeze   Arrows: +/-1   PgUp/PgDn: +/-16  Z: Zoom   T: Trigger", BPoint(panel.left + 10.0f, line1Y));

	/*
	 * Second help line.
	 */
	DrawString("Home/End: Oldest/Newest   C: Clear   1-4: Hide/Show   S: Solo   A: All", BPoint(panel.left + 10.0f, line2Y));

	/*
	 * LIVE / FROZEN indicator at the right side of the second line.
	 */
	const char *stateText = fFreezeUpdates ? "[FROZEN]" : "[LIVE]";
	const float stateWidth = StringWidth(stateText);

	if (fFreezeUpdates) {
		SetHighColor(185, 35, 35);
	} else {
		SetHighColor(0, 120, 40);
	}

	DrawString(stateText, BPoint(panel.right - 10.0f - stateWidth, line2Y));

	/*
	 * Third line: capture and debugger state.
	 */
	const uint32 visibleCount = (fVisibleSampleCount < fSampleCount) ? fVisibleSampleCount : fSampleCount;
	const float windowMilliseconds = static_cast<float>(visibleCount) / 48000.0f * 1000.0f;
	float drawX = panel.left + 10.0f;

	BString text;

	/*
	 * Captured samples.
	 */
	SetHighColor(kTextColor);

	text.SetToFormat("Captured: %lu", static_cast<unsigned long>(fSampleCount));
	DrawString(text.String(), BPoint(drawX, line3Y));
	drawX += StringWidth(text.String());

	/*
	 * Visible samples.
	 */
	SetHighColor(kTimingColor);

	text.SetToFormat("  Visible: %lu", static_cast<unsigned long>(visibleCount));
	DrawString(text.String(), BPoint(drawX, line3Y));
	drawX += StringWidth(text.String());

	/*
	 * Visible time window.
	 */
	text.SetToFormat("  Window: %.3fms", windowMilliseconds);
	DrawString(text.String(), BPoint(drawX, line3Y));
	drawX += StringWidth(text.String());

	/*
	 * Trigger state.
	 */
	SetHighColor(kTextColor);
	DrawString("  Trigger: ", BPoint(drawX, line3Y));
	drawX += StringWidth("  Trigger: ");

	SetHighColor(kTriggerColor);

	const char *triggerName = TriggerChannelName();
	DrawString(triggerName, BPoint(drawX, line3Y));
	drawX += StringWidth(triggerName);

	/*
	 * Solo state.
	 */
	SetHighColor(kTextColor);
	DrawString("  Solo: ", BPoint(drawX, line3Y));
	drawX += StringWidth("  Solo: ");

	const bool soloEnabled = fSoloChannel >= 0;

	SetHighColor(soloEnabled ? kSoloOnColor : kSoloOffColor);
	const char *soloText = soloEnabled ? "ON" : "OFF";
	DrawString(soloText, BPoint(drawX, line3Y));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::DrawWaveformPanel
//
// Draws one mapper-provided expansion-audio waveform using the same debugger
// panel styling and waveform geometry as the Native APU scope.
//
// The displayed sample range follows the current horizontal zoom level. In live
// mode the newest samples are shown, or the trigger-aligned range when trigger
// mode is enabled. While frozen, the visible range follows the inspection
// cursor.
//
// The upper-right corner reports the minimum and maximum raw sample values
// contained in the currently visible waveform range.
//
// A small display-only gain is applied to make expansion-audio signals easier
// to inspect. Captured values and Min/Max statistics remain unchanged.
//
// Parameters:
//   panel        - Destination panel rectangle.
//   title        - Channel name.
//   channel      - Expansion channel index in scope_sample_t::expansion[].
//   maximumValue - Maximum raw output level for vertical scaling.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::DrawWaveformPanel (BRect panel, const char *title, size_t channel, float maximumValue)
{
	::DrawDebugPanel(this, panel, title);

	/*
	 * Visible Min / Max statistics.
	 */
	if (fSampleCount != 0) {
		float minimumVisibleValue;
		float maximumVisibleValue;

		ComputeVisibleMinMax(channel, minimumVisibleValue, maximumVisibleValue);

		BString statistics;
		statistics.SetToFormat( "Min: %u   Max: %u", static_cast<unsigned>(minimumVisibleValue),
													 static_cast<unsigned>(maximumVisibleValue));
		BFont previousFont;
		GetFont(&previousFont);

		BFont fixed(be_fixed_font);
		fixed.SetSize(10.0f);

		SetFont(&fixed);
		SetHighColor(70, 70, 70);

		const float statisticsWidth = StringWidth(statistics.String());
		const float statisticsX = panel.right - 10.0f - statisticsWidth;
		DrawString(statistics.String(), BPoint(statisticsX, panel.top + 17.0f));

		SetFont(&previousFont);
	}

	const float left = panel.left + 8.0f;
	const float right = panel.right - 8.0f;
	const float top = panel.top + 32.0f;
	const float bottom = panel.bottom - 8.0f;
	const float width = right - left;

	const float height = bottom - top;

	if (width <= 0.0f ||
		height <= 0.0f) {
		return;
	}

	/*
	* Match the Native APU scope trigger marker.
 	*
 	* In live trigger mode, the selected trigger edge is positioned at the
 	* user-selected horizontal trigger position.
 	*/
	const float centerY = top + (height * 0.5f);

	SetPenSize(1.0f);
	SetHighColor(155, 155, 155);

	StrokeLine(BPoint(left, centerY), BPoint(right, centerY));

	/*
	 * Match the Native APU scope trigger marker.
	 *
	 * In live trigger mode, the selected trigger edge is positioned
	 * approximately one quarter of the way across the waveform display.
	 */
	if (fTriggerEnabled && !fFreezeUpdates) {

		const float triggerX = left + (width * fTriggerPosition);
		SetPenSize(1.0f);
		SetHighColor(0, 90, 170);
		StrokeLine(BPoint(triggerX, top), BPoint(triggerX, bottom));
	}

	if (fSampleCount < 2 || maximumValue <= 0.0f) {
		return;
	}

	uint32 startIndex;
	uint32 endIndex;

	ComputeVisibleSampleRange(startIndex, endIndex);

	if (endIndex <= startIndex) {
		return;
	}

	const uint32 visibleCount = endIndex - startIndex + 1;
	const rgb_color waveColor = kWaveColors[channel % (sizeof(kWaveColors) / sizeof(kWaveColors[0]))];

	SetHighColor(waveColor);
	SetPenSize(2.0f);

	bool havePrevious = false;
	BPoint previous;

	for (uint32 visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
		const uint32 sampleIndex = startIndex + visibleIndex;

		if (channel >= fSamples[sampleIndex].expansion_channel_count) {

			havePrevious = false;
			continue;
		}

		const float sampleValue = static_cast<float>(fSamples[sampleIndex].expansion[channel]);
		float normalized = sampleValue / maximumValue;

		const float channelGain = 1.5f;

		normalized *= channelGain;

		if (normalized < 0.0f) {
			normalized = 0.0f;
		}

		if (normalized > 1.0f) {
			normalized = 1.0f;
		}

		const float sampleX = left + (width * static_cast<float>(visibleIndex) / static_cast<float>(visibleCount - 1));
		const float sampleY = bottom - (normalized * height);
		const BPoint currentPoint(sampleX, sampleY);

		if (havePrevious) {
			StrokeLine(previous, currentPoint);
		}

		previous = currentPoint;
		havePrevious = true;
	}

	if (fFreezeUpdates &&fCursorSample >= static_cast<int32>(startIndex) &&
		fCursorSample <= static_cast<int32>(endIndex)) {

		const uint32 cursorVisibleIndex = static_cast<uint32>(fCursorSample) - startIndex;
		const float cursorX = left + (width * static_cast<float>(cursorVisibleIndex) / static_cast<float>(visibleCount - 1));

		SetPenSize(1.0f);
		SetHighColor(185, 35, 35);

		StrokeLine(BPoint(cursorX, top), BPoint(cursorX, bottom));
	}

	SetPenSize(1.0f);
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::ClampCursor
//
// Keeps the frozen oscilloscope cursor within the currently captured sample
// range.
//
// If no samples are available, the cursor is disabled by setting it to -1.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------

void
ExpansionAudioScopeView::ClampCursor()
{
	if (fSampleCount == 0) {
		fCursorSample = -1;
		return;
	}

	if (fCursorSample < 0) {
		fCursorSample = 0;
	}

	if (fCursorSample >= static_cast<int32>(fSampleCount)) {
		fCursorSample = static_cast<int32>(fSampleCount) - 1;
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::ComputeVisibleSampleRange
//
// Computes the currently visible oscilloscope sample range.
//
// Frozen mode follows the inspection cursor and attempts to keep the cursor
// centered in the visible sample window.
//
// In live trigger mode, the most recent usable rising edge is positioned at
// the user-selected trigger position. The trigger position is stored as a
// normalized value from 0.0 to 1.0 and may be changed with the mouse.
//
// Without an active trigger, or when no suitable trigger edge can be found,
// live mode displays the newest available samples.
//
// Parameters:
//   startIndex - Receives the first visible sample index.
//   endIndex   - Receives the final visible sample index, inclusive.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::ComputeVisibleSampleRange (uint32 &startIndex, uint32 &endIndex) const
{
	startIndex = 0;
	endIndex = 0;

	if (fSampleCount == 0) {
		return;
	}

	const uint32 visibleCount = (fVisibleSampleCount < fSampleCount) ? fVisibleSampleCount : fSampleCount;

	/*
	 * Frozen mode always follows the inspection cursor.
	 */
	if (fFreezeUpdates && fCursorSample >= 0) {
		const int32 halfVisible = static_cast<int32>(visibleCount / 2);
		int32 start = fCursorSample - halfVisible;

		if (start < 0) {
			start = 0;
		}

		int32 end = start + static_cast<int32>(visibleCount) - 1;
		const int32 lastSample = static_cast<int32>(fSampleCount) - 1;

		if (end > lastSample) {
			end = lastSample;

			start = end - static_cast<int32>(visibleCount) + 1;

			if (start < 0) {
				start = 0;
			}
		}

		startIndex = static_cast<uint32>(start);
		endIndex = static_cast<uint32>(end);

		return;
	}

	/*
	 * Live trigger mode.
	 *
	 * The requested trigger position determines how many samples should
	 * appear before the trigger edge. For example:
	 *
	 *     0.25 = trigger at 25% of the graph width
	 *     0.50 = trigger at the center
	 *     0.75 = trigger at 75% of the graph width
	 *
	 * We search only as far forward as we can while still leaving enough
	 * post-trigger samples to completely fill the visible window.
	 */
	if (fTriggerEnabled && visibleCount >= 2) {
		uint32 preTrigger = static_cast<uint32>(static_cast<float>(visibleCount - 1) * fTriggerPosition);

		if (preTrigger >= visibleCount) {
			preTrigger = visibleCount - 1;
		}

		const uint32 postTrigger = visibleCount - preTrigger - 1;

		if (fSampleCount > postTrigger) {
			const uint32 latestUsableTrigger = fSampleCount - postTrigger - 1;
			uint32 triggerIndex = 0;

			if (FindTriggerSample(fTriggerChannel, latestUsableTrigger, triggerIndex)) {

				/*
				 * Normally the trigger must have enough samples before
				 * it to appear at the requested horizontal position.
				 */
				if (triggerIndex >= preTrigger) {
					startIndex = triggerIndex - preTrigger;
					endIndex = startIndex + visibleCount - 1;

					return;
				}
			}
		}
	}

	/*
	 * Normal live mode, or trigger mode with no suitable rising edge.
	 *
	 * Display the newest available samples.
	 */
	endIndex = fSampleCount - 1;

	startIndex = fSampleCount - visibleCount;
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::ComputeVisibleMinMax
//
// Computes the minimum and maximum captured values for one expansion-audio
// channel across the currently visible waveform range.
//
// The calculation follows the active horizontal zoom and frozen-cursor window,
// so the values describe exactly the samples currently being displayed.
//
// Parameters:
//   channel      - Expansion-audio channel index.
//   minimumValue - Receives the minimum visible sample value.
//   maximumValue - Receives the maximum visible sample value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::ComputeVisibleMinMax (size_t channel, float &minimumValue, float &maximumValue) const
{
	minimumValue = 0.0f;
	maximumValue = 0.0f;

	if (fSampleCount == 0) {
		return;
	}

	uint32 startIndex;
	uint32 endIndex;

	ComputeVisibleSampleRange(startIndex, endIndex);

	if (endIndex < startIndex || endIndex >= fSampleCount) {
		return;
	}

	bool haveValue = false;

	for (uint32 index = startIndex; index <= endIndex; ++index) {
		if (channel >= fSamples[index].expansion_channel_count) {
			continue;
		}

		const float value = static_cast<float>(fSamples[index].expansion[channel]);

		if (!haveValue) {
			minimumValue = value;
			maximumValue = value;
			haveValue = true;
			continue;
		}

		if (value < minimumValue) {
			minimumValue = value;
		}

		if (value > maximumValue) {
			maximumValue = value;
		}
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::ComputeVisibleTotalMinMax
//
// Computes the minimum and maximum combined expansion-audio output values
// across the currently visible waveform range.
//
// The calculation follows the current zoom level and frozen-cursor window so
// the statistics describe exactly what is visible in the Total panel.
//
// Parameters:
//   minimumValue - Receives the minimum visible total value.
//   maximumValue - Receives the maximum visible total value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::ComputeVisibleTotalMinMax (float &minimumValue, float &maximumValue) const
{
	minimumValue = 0.0f;
	maximumValue = 0.0f;

	if (fSampleCount == 0) {
		return;
	}

	uint32 startIndex;
	uint32 endIndex;

	ComputeVisibleSampleRange(startIndex, endIndex);

	if (endIndex < startIndex || endIndex >= fSampleCount) {
		return;
	}

	minimumValue = static_cast<float>(fSamples[startIndex].expansion_total);
	maximumValue = minimumValue;

	for (uint32 index = startIndex + 1; index <= endIndex; ++index) {
		const float value = static_cast<float>(fSamples[index].expansion_total);

		if (value < minimumValue) {
			minimumValue = value;
		}

		if (value > maximumValue) {
			maximumValue = value;
		}
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::CycleTriggerMode
//
// Advances the oscilloscope through the available expansion-audio trigger
// sources.
//
// The current VRC6 sequence is:
//
//   OFF -> Pulse 1 -> Pulse 2 -> Saw -> Expansion Total -> OFF
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::CycleTriggerMode()
{
	if (!fTriggerEnabled) {
		fTriggerEnabled = true;
		fTriggerChannel = TRIGGER_CHANNEL_0;
		return;
	}

	switch (fTriggerChannel) {
		case TRIGGER_CHANNEL_0:
			fTriggerChannel = TRIGGER_CHANNEL_1;
			break;

		case TRIGGER_CHANNEL_1:
			fTriggerChannel = TRIGGER_CHANNEL_2;
			break;

		case TRIGGER_CHANNEL_2:
			fTriggerChannel = TRIGGER_TOTAL;
			break;

		case TRIGGER_TOTAL:
			fTriggerEnabled = false;
			fTriggerChannel = TRIGGER_CHANNEL_0;
			break;
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::TriggerChannelName
//
// Returns the display name of the currently selected trigger source.
//
// Parameters:
//   None.
//
// Returns:
//   Human-readable trigger-channel name, or "OFF" when triggering is disabled.
// -----------------------------------------------------------------------------
const char*
ExpansionAudioScopeView::TriggerChannelName() const
{
	if (!fTriggerEnabled) {
		return "OFF";
	}

	switch (fTriggerChannel) {
		case TRIGGER_CHANNEL_0:
			return "Pulse 1";

		case TRIGGER_CHANNEL_1:
			return "Pulse 2";

		case TRIGGER_CHANNEL_2:
			return "Saw";

		case TRIGGER_TOTAL:
			return "Total";
	}

	return "OFF";
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::TriggerThreshold
//
// Returns the default rising-edge trigger threshold for one expansion-audio
// trigger source.
//
// The individual VRC6 channel thresholds are approximately the midpoint of
// their normal raw output ranges. Expansion Total uses approximately the
// midpoint of the current combined VRC6 raw output range.
//
// Parameters:
//   channel - Trigger source whose threshold is requested.
//
// Returns:
//   Rising-edge trigger threshold.
// -----------------------------------------------------------------------------
float
ExpansionAudioScopeView::TriggerThreshold (trigger_channel channel) const
{
	switch (channel) {
		case TRIGGER_CHANNEL_0:
		case TRIGGER_CHANNEL_1:
			return 7.5f;

		case TRIGGER_CHANNEL_2:
			return 15.5f;

		case TRIGGER_TOTAL:
			return 30.5f;
	}

	return 0.0f;
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::FindTriggerSample
//
// Searches backward through the captured expansion-audio history for the most
// recent rising-edge crossing on the selected trigger source, beginning at the
// supplied latest usable sample index.
//
// Limiting the search to latestIndex ensures that enough post-trigger samples
// remain available to keep the trigger point near the quarter-width marker.
//
// Parameters:
//   channel      - Expansion-audio trigger source.
//   latestIndex  - Latest sample index that may be used as a trigger point.
//   triggerIndex - Receives the matching sample index.
//
// Returns:
//   true if a rising-edge trigger was found, otherwise false.
// -----------------------------------------------------------------------------
bool
ExpansionAudioScopeView::FindTriggerSample (trigger_channel channel, uint32 latestIndex, uint32 &triggerIndex) const
{
	if (fSampleCount < 2) {
		return false;
	}

	if (latestIndex >= fSampleCount) {
		latestIndex = fSampleCount - 1;
	}

	if (latestIndex == 0) {
		return false;
	}

	const float threshold = TriggerThreshold(channel);

	for (uint32 index = latestIndex; index > 0;   --index) {

		float previousValue = 0.0f;
		float currentValue = 0.0f;

		if (channel == TRIGGER_TOTAL) {
			previousValue = static_cast<float>(fSamples[index - 1].expansion_total);
			currentValue = static_cast<float>(fSamples[index].expansion_total);
		} else {
			const size_t channelIndex = static_cast<size_t>(channel);

			if (channelIndex >= fSamples[index - 1].expansion_channel_count ||
				channelIndex >= fSamples[index].expansion_channel_count) {
				continue;
			}

			previousValue = static_cast<float>(fSamples[index - 1].expansion[channelIndex]);
			currentValue = static_cast<float>(fSamples[index].expansion[channelIndex]);
		}

		if (previousValue < threshold && currentValue >= threshold) {
			triggerIndex = index;
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::CycleSoloMode
//
// Cycles through the available expansion-audio Solo targets.
//
// The mapper-provided channels are selected first, followed by the combined
// Expansion Total output. Cycling past Expansion Total disables Solo.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::CycleSoloMode()
{
	if (fChannelCount == 0) {
		fSoloChannel = -1;
		return;
	}

	if (fSoloChannel < 0) {
		fSoloChannel = 0;
		return;
	}

	fSoloChannel++;

	if (fSoloChannel >
		static_cast<int32>(fChannelCount)) {

		fSoloChannel = -1;
	}
}


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView::DrawTotalWaveformPanel
//
// Draws the combined mapper expansion-audio waveform.
//
// This panel uses the same debugger-panel styling, visible-range Min/Max
// statistics, trigger marker, zoom behavior, and frozen inspection cursor as
// the individual expansion-audio channel panels.
//
// Parameters:
//   panel - Destination panel rectangle.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ExpansionAudioScopeView::DrawTotalWaveformPanel (BRect panel)
{
	::DrawDebugPanel(this, panel, "Expansion Total");

	/*
	 * Visible Min / Max statistics.
	 */
	if (fSampleCount != 0) {
		float minimumVisibleValue;
		float maximumVisibleValue;

		ComputeVisibleTotalMinMax(minimumVisibleValue, maximumVisibleValue);

		BString statistics;
		statistics.SetToFormat("Min: %u   Max: %u", static_cast<unsigned>(minimumVisibleValue),
													static_cast<unsigned>(maximumVisibleValue));

		BFont previousFont;
		GetFont(&previousFont);

		BFont fixed(be_fixed_font);
		fixed.SetSize(10.0f);

		SetFont(&fixed);
		SetHighColor(70, 70, 70);

		const float statisticsWidth = StringWidth(statistics.String());
		const float statisticsX = panel.right - 10.0f - statisticsWidth;

		DrawString(statistics.String(), BPoint(statisticsX, panel.top + 17.0f));
		SetFont(&previousFont);
	}

	const float left = panel.left + 8.0f;
	const float right = panel.right - 8.0f;
	const float graphTop = panel.top + 32.0f;

	const float graphBottom = panel.bottom - 8.0f;
	const float width = right - left;
	const float height = graphBottom - graphTop;

	if (width <= 0.0f || height <= 0.0f) {

		return;
	}

	/*
	 * Horizontal center guide.
	 */
	const float centerY = graphTop + (height * 0.5f);

	SetPenSize(1.0f);
	SetHighColor(155, 155, 155);

	StrokeLine(BPoint(left, centerY), BPoint(right, centerY));

	/*
	 * Trigger reference marker.
	 */
	if (fTriggerEnabled && !fFreezeUpdates) {
		const float triggerX = left + (width * fTriggerPosition);

		SetPenSize(1.0f);
		SetHighColor(0, 90, 170);
		StrokeLine(BPoint(triggerX, graphTop), BPoint(triggerX, graphBottom));
	}

	if (fSampleCount < 2) {
		return;
	}

	uint32 startIndex;
	uint32 endIndex;

	ComputeVisibleSampleRange(startIndex, endIndex);

	if (endIndex <= startIndex) {
		return;
	}

	const uint32 visibleCount = endIndex - startIndex + 1;

	/*
	 * Current VRC6 combined raw range:
	 *
	 * Pulse 1: 0..15
	 * Pulse 2: 0..15
	 * Saw:     0..31
	 *
	 * Total:   0..61
	 */
	const float maximumValue = 61.0f;

	SetHighColor(25, 25, 25);
	SetPenSize(2.0f);

	BPoint previous;

	for (uint32 visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
		const uint32 sampleIndex = startIndex + visibleIndex;
		const float sampleValue = static_cast<float>(fSamples[sampleIndex].expansion_total);

		float normalized = sampleValue / maximumValue;

		if (normalized < 0.0f) {
			normalized = 0.0f;
		}

		if (normalized > 1.0f) {
			normalized = 1.0f;
		}

		const float sampleX = left + (width * static_cast<float>(visibleIndex) /static_cast<float>(visibleCount - 1));
		const float sampleY = graphBottom - (normalized * height);
		const BPoint currentPoint(sampleX, sampleY);

		if (visibleIndex != 0) {
			StrokeLine(previous, currentPoint);
		}

		previous = currentPoint;
	}

	/*
	 * Frozen inspection cursor.
	 */
	if (fFreezeUpdates && fCursorSample >= static_cast<int32>(startIndex) &&
		fCursorSample <= static_cast<int32>(endIndex)) {

		const uint32 cursorVisibleIndex = static_cast<uint32>(fCursorSample) - startIndex;
		const float cursorX = left + (width * static_cast<float>(cursorVisibleIndex) /
			 				  static_cast<float>(visibleCount - 1));

		SetPenSize(1.0f);
		SetHighColor(185, 35, 35);

		StrokeLine(BPoint(cursorX, graphTop), BPoint(cursorX, graphBottom));
	}

	SetPenSize(1.0f);
}

