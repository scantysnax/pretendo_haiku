
#include "APUScopeView.h"


namespace {
	const rgb_color kTextColor = { 25, 25, 25, 255 };
	const rgb_color kGoodColor = { 0, 120, 40, 255 };
	const rgb_color kBadColor = { 170, 35, 35, 255 };
	const rgb_color kTimingColor = { 0, 80, 145, 255 };
	const rgb_color kTriggerColor = { 105, 35, 135, 255 };
	const rgb_color kSoloColor = { 150, 75, 0, 255 };
};


// -----------------------------------------------------------------------------
// APUScopeView::APUScopeView
//
// Constructs the APU oscilloscope debugger view.
//
// Parameters:
//   frame  - Initial view bounds.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUScopeView::APUScopeView (BRect frame, PretendoWindow *parent)
	: BView (frame, "apu_scope_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	(void)parent;

	SetViewColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// APUScopeView::~APUScopeView
//
// Destroys the APU oscilloscope debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUScopeView::~APUScopeView()
{
}


// -----------------------------------------------------------------------------
// APUScopeView::AttachedToWindow
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
APUScopeView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	if (HasROMLoaded()) {
		CaptureSamples();
	}
}


// -----------------------------------------------------------------------------
// APUScopeView::Draw
//
// Draws the APU Scope debugger view.
//
// Normally, all currently enabled channel panels share the available waveform
// area equally.  Hidden channels consume no layout space.
//
// When solo mode is active, only the selected channel is rendered and it uses
// the entire waveform area.
//
// Parameters:
//   updateRect - Region requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		DrawNoROMMessage();
		return;
	}

	struct panel_info_t {
		const char *title;
		scope_channel channel;
		float maximumValue;
	};

	const panel_info_t panels[6] = {
		{
			"Square 1",
			SCOPE_SQUARE1,
			15.0f
		},
		{
			"Square 2",
			SCOPE_SQUARE2,
			15.0f
		},
		{
			"Triangle",
			SCOPE_TRIANGLE,
			15.0f
		},
		{
			"Noise",
			SCOPE_NOISE,
			15.0f
		},
		{
			"DMC",
			SCOPE_DMC,
			127.0f
		},
		{
			"Mixed",
			SCOPE_MIXED,
			255.0f
		}
	};

	int32 visiblePanelCount = 0;

	if (fSoloChannel >= 0 && fSoloChannel < 6) {
		visiblePanelCount = 1;
	} else {
		for (int32 index = 0; index < 6; index++) {
			if (fChannelVisible[index]) {
				visiblePanelCount++;
			}
		}
	}

	const float margin = 4.0f;
	const float gap = 4.0f;
	const float top = 88.0f;
	const float bottom = Bounds().bottom - margin;

	if (visiblePanelCount == 0) {
		BFont prevFont;
		GetFont(&prevFont);

		BFont fixed(be_fixed_font);
		fixed.SetSize(10.0f);

		SetFont(&fixed);
		SetHighColor(90, 90, 90);

		const char *message = "No channels visible - press A to show all";
		const float messageWidth = StringWidth(message);

		DrawString(message, BPoint((Bounds().Width() - messageWidth) * 0.5f, top + ((bottom - top) * 0.5f)));
		SetFont(&prevFont);
		return;
	}

	const float availableHeight = bottom - top - (gap * static_cast<float>(visiblePanelCount - 1));
	const float panelHeight = availableHeight / static_cast<float>(visiblePanelCount);
	float panelTop = top;
	
	int32 drawnPanels = 0;

	for (int32 index = 0; index < 6; index++) {
		if (fSoloChannel >= 0) {
			if (index != fSoloChannel) {
				continue;
			}
		} else {
			if (!fChannelVisible[index]) {
				continue;
			}
		}

		drawnPanels++;

		float panelBottom = panelTop + panelHeight;

		if (drawnPanels == visiblePanelCount) {
			panelBottom = bottom;
		}

		BRect panel(margin, panelTop, Bounds().right - margin, panelBottom);

		DrawWaveformPanel(panel, panels[index].title, panels[index].channel, panels[index].maximumValue);
		panelTop = panelBottom + gap;
	}
}


// -----------------------------------------------------------------------------
// APUScopeView::Pulse
//
// Refreshes the oscilloscope sample snapshot while the Scope is live.
//
// When display freeze is active, the stored waveform snapshot is left
// untouched.  The APU itself and its backend scope capture continue running;
// only this debugger view stops accepting new snapshots.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::Pulse()
{
	if (!HasROMLoaded()) {
		Invalidate();
		return;
	}

	if (!fFreezeUpdates) {
		CaptureSamples();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// APUScopeView::KeyDown
//
// Handles keyboard controls for the APU Scope debugger.
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
//   C           - Clear scope history.
//   T           - Cycle rising-edge trigger source.
//   1-6         - Toggle channel visibility.
//   S           - Cycle solo channel.
//   A           - Show all channels and cancel solo mode.
//
// Parameters:
//   bytes    - Keyboard input bytes.
//   numBytes - Number of bytes supplied.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == ' ') {
		if (!HasROMLoaded()) {
			return;
		}

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

	if (numBytes == 1 && (bytes[0] == 'c' || bytes[0] == 'C')) {
		if (!HasROMLoaded()) {
			return;
		}

		nes::apu::debug_clear_scope();

		fSampleCount = 0;
		fCursorSample = -1;

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 'z' || bytes[0] == 'Z')) {
		if (!HasROMLoaded()) {
			return;
		}

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

	if (numBytes == 1 && (bytes[0] == 't' || bytes[0] == 'T')) {
		if (!HasROMLoaded()) {
			return;
		}

		CycleTriggerMode();

		Invalidate();
		return;
	}

	if (numBytes == 1 && bytes[0] >= '1' && bytes[0] <= '6') {
		if (!HasROMLoaded()) {
			return;
		}

		const scope_channel channel = static_cast<scope_channel>(bytes[0] - '1');
		ToggleChannelVisible(channel);

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 's' || bytes[0] == 'S')) {
		if (!HasROMLoaded()) {
			return;
		}

		CycleSoloChannel();

		Invalidate();
		return;
	}

	if (numBytes == 1 && (bytes[0] == 'a' || bytes[0] == 'A')) {
		if (!HasROMLoaded()) {
			return;
		}

		for (int32 index = 0; index < 6; index++) {
			fChannelVisible[index] = true;
		}

		fSoloChannel = -1;

		Invalidate();
		return;
	}

	if (fFreezeUpdates && fSampleCount != 0) {
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
// APUScopeView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently active.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded, otherwise false.
// -----------------------------------------------------------------------------
bool
APUScopeView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// APUScopeView::DrawNoROMMessage
//
// Draws the centered no-ROM message used when no cartridge is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::DrawNoROMMessage()
{
	const char *title = "No ROM Loaded";
	const char *message = "Load a cartridge to view APU waveforms.";

	BFont prevFont;
	GetFont(&prevFont);

	BFont font(be_plain_font);
	font.SetSize(12.0f);

	SetFont(&font);
	SetHighColor(90, 90, 90);

	font_height fh;
	GetFontHeight(&fh);

	const float centerY = Bounds().Height() * 0.5f;
	const float titleWidth = StringWidth(title);
	DrawString(title, BPoint((Bounds().Width() - titleWidth) * 0.5f, centerY));

	font.SetSize(10.0f);
	SetFont(&font);
	SetHighColor(135, 135, 135);

	const float messageWidth = StringWidth(message);
	DrawString(message, BPoint((Bounds().Width() - messageWidth) * 0.5f, centerY + 24.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUScopeView::DrawHeaderPanel
//
// Draws the APU Scope debugger header together with keyboard controls, zoom,
// trigger state, solo state, sample timing information, and frozen cursor
// values.
//
// Scope samples are captured at 48 kHz.
//
// Semantic colors are used sparingly:
//   Green       - LIVE state.
//   Red         - FROZEN state.
//   Blue        - Timing/window information.
//   Purple      - Trigger source.
//   Orange      - Solo source.
//   Dark gray   - Normal controls and sample values.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 82.0f);
	::DrawDebugPanel(this, panel, "APU Scope");

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);

	SetFont(&fixed);

	const float x = panel.left + 10.0f;
	const float line1Y = panel.top + 38.0f;
	const float line2Y = line1Y + 15.0f;
	const float line3Y = line2Y + 15.0f;

	SetHighColor(kTextColor);
	DrawString("Space: Freeze   Arrows: +/-1   PgUp/PgDn: +/-16   Z: Zoom   T: Trigger", BPoint(x, line1Y));
	DrawString("Home/End: Oldest/Newest   C: Clear   1-6: Hide/Show   S: Solo   A: All", BPoint(x, line2Y));

	const double sampleRate = nes::apu::kOutputFrequency;
	const uint32 visibleSamples = (fVisibleSampleCount < fSampleCount) ? fVisibleSampleCount : fSampleCount;
	const double visibleMilliseconds = (static_cast<double>( visibleSamples) / sampleRate) * 1000.0;

	if (!fFreezeUpdates) {
		float drawX = x;

		BString text;

		text.SetToFormat("Captured: %lu  ", static_cast<unsigned long>(fSampleCount));
		SetHighColor(kTextColor);
		DrawString(text.String(), BPoint(drawX, line3Y));
		drawX += StringWidth(text.String());

		text.SetToFormat("Visible: %lu  Window: %.3fms  ", static_cast<unsigned long>(visibleSamples),
						 visibleMilliseconds);
		SetHighColor(kTimingColor);
		DrawString(text.String(), BPoint(drawX, line3Y));
		drawX += StringWidth(text.String());

		SetHighColor(kTextColor);
		DrawString("Trigger: ", BPoint(drawX, line3Y));
		drawX += StringWidth("Trigger: ");

		SetHighColor(kTriggerColor);
		DrawString(TriggerChannelName(), BPoint(drawX, line3Y));
		drawX += StringWidth(TriggerChannelName());

		SetHighColor(kTextColor);
		DrawString("  Solo: ", BPoint(drawX, line3Y));
		drawX += StringWidth("  Solo: ");

		SetHighColor(kSoloColor);
		DrawString(SoloChannelName(), BPoint(drawX, line3Y));
	} else if (fCursorSample >= 0 && fCursorSample < static_cast<int32>(fSampleCount)) {
		const nes::apu::apu_scope_sample_t &sample = fSamples[fCursorSample];
		const int32 samplesFromNewest = static_cast<int32>(fSampleCount) - 1 - fCursorSample;
		const double millisecondsFromNewest = (static_cast<double>(samplesFromNewest) / sampleRate) * 1000.0;
		float drawX = x;

		BString text;
		text.SetToFormat("Sample: %ld/%lu  ", static_cast<long>(fCursorSample + 1),
						 static_cast<unsigned long>(fSampleCount));
		SetHighColor(kTextColor);
		DrawString(text.String(), BPoint(drawX, line3Y));
		drawX += StringWidth(text.String());

		text.SetToFormat("Time: -%.3fms  Visible:%lu  ", millisecondsFromNewest,
						 static_cast<unsigned long>(visibleSamples));
		SetHighColor(kTimingColor);
		DrawString(text.String(), BPoint(drawX, line3Y));
		drawX += StringWidth(text.String());

		text.SetToFormat(" Square1: %u  Square2: %u  Triangle: %u  Noise: %u  DMC: %u  Mix: %.0f", 
						 static_cast<unsigned>(sample.square1), static_cast<unsigned>(sample.square2),
						 static_cast<unsigned>(sample.triangle), static_cast<unsigned>(sample.noise),
						 static_cast<unsigned>(sample.dmc), sample.mixed);
		SetHighColor(kTextColor);
		DrawString(text.String(), BPoint(drawX, line3Y));
	} else {
		SetHighColor(kTextColor);
		DrawString("Captured: 0   Scope history cleared", BPoint(x, line3Y));
	}

	BString state;
	state.SetToFormat("[%s]", fFreezeUpdates ? "FROZEN" : "LIVE");
	const float stateWidth = StringWidth(state.String());
	const float stateX = panel.right - 10.0f - stateWidth;

	if (fFreezeUpdates) {
		SetHighColor(kBadColor);
	} else {
		SetHighColor(kGoodColor);
	}

	DrawString(state.String(), BPoint(stateX, line2Y));
	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUScopeView::CaptureSamples
//
// Captures the latest debugger oscilloscope sample history together with the
// current effective APU channel state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::CaptureSamples()
{
	fSampleCount = nes::apu::debug_scope_snapshot(fSamples, nes::apu::APU_SCOPE_SAMPLE_CAPACITY);
	fDebugState = nes::apu::debug_state();
}


// -----------------------------------------------------------------------------
// APUScopeView::DrawWaveformPanel
//
// Draws one oscilloscope panel using the currently selected horizontal zoom.
//
// In live mode, the newest visible samples are shown.  While frozen, the
// visible range follows the cursor and attempts to keep it centered.
//
// When trigger mode is active in live mode, a blue vertical marker indicates
// the trigger position.
//
// The panel title reports current effective channel state and timing
// information.  The right side of the title area reports the minimum and
// maximum values contained in the currently visible waveform range.
//
// Individual APU channels are displayed as unipolar signals.  Square 1,
// Square 2, and DMC receive a small visual gain so their waveforms are easier
// to inspect.  Triangle and Noise remain at their natural display scale.
//
// The mixed PCM trace is displayed as a bipolar waveform centered around
// unsigned PCM silence at 128.  A separate vertical gain is applied to the
// mixed trace.
//
// These gains affect only waveform rendering.  Captured sample values,
// Min/Max statistics, trigger calculations, and frozen cursor values remain
// unchanged.
//
// Parameters:
//   panel        - Rectangle defining the waveform panel.
//   title        - Base channel title displayed at the top of the panel.
//   channel      - APU channel to render.
//   maximumValue - Maximum expected unipolar channel output value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::DrawWaveformPanel (BRect panel, const char *title, scope_channel channel, float maximumValue)
{
	BString panelTitle;

	switch (channel) {
		case SCOPE_SQUARE1:
		{
			if (!fDebugState.square1.enabled) {
				panelTitle.SetToFormat("%s   OFF", title);
			} else {
				const double frequency = kNTSCCPUClock /
										 (16.0 * (static_cast<double>(fDebugState.square1.timer_period) + 1.0));

				if (fDebugState.square1.muted) {
					panelTitle.SetToFormat("%s   MUTED   %.1f Hz", title, frequency);
				} else {
					panelTitle.SetToFormat("%s   ON   %.1f Hz", title, frequency);
				}
			}
			break;
		}

		case SCOPE_SQUARE2:
		{
			if (!fDebugState.square2.enabled) {
				panelTitle.SetToFormat("%s   OFF", title);
			} else {
				const double frequency = kNTSCCPUClock /
										 (16.0 * (static_cast<double>(fDebugState.square2.timer_period) + 1.0));

				if (fDebugState.square2.muted) {
					panelTitle.SetToFormat("%s   MUTED   %.1f Hz", title, frequency);
				} else {
					panelTitle.SetToFormat("%s   ON   %.1f Hz", title, frequency);
				}
			}
			break;
		}

		case SCOPE_TRIANGLE:
		{
			if (!fDebugState.triangle.enabled) {
				panelTitle.SetToFormat("%s   OFF", title);
			} else {
				const double frequency = kNTSCCPUClock /
										 (32.0 * (static_cast<double>(fDebugState.triangle.timer_period) + 1.0));

				if (fDebugState.triangle.muted) {
					panelTitle.SetToFormat("%s   MUTED   %.1f Hz", title, frequency);
				} else {
					panelTitle.SetToFormat("%s   ON   %.1f Hz", title, frequency);
				}
			}
			break;
		}

		case SCOPE_NOISE:
		{
			if (!fDebugState.noise.enabled) {
				panelTitle.SetToFormat("%s   OFF", title);
			} else {
				double clockRate = 0.0;

				if (fDebugState.noise.timer_period != 0) {
					clockRate = kNTSCCPUClock /
								static_cast<double>(fDebugState.noise.timer_period);
				}

				if (fDebugState.noise.muted) {
					if (clockRate > 0.0) {
						panelTitle.SetToFormat("%s   MUTED   Clock: %.1f Hz", title, clockRate);
					} else {
						panelTitle.SetToFormat("%s   MUTED   Clock: -", title);
					}
				} else {
					if (clockRate > 0.0) {
						panelTitle.SetToFormat("%s   ON   Clock: %.1f Hz", title, clockRate);
					} else {
						panelTitle.SetToFormat("%s   ON   Clock: -", title);
					}
				}
			}
			break;
		}

		case SCOPE_DMC:
		{
			if (!fDebugState.dmc.enabled) {
				panelTitle.SetToFormat("%s   OFF", title);
			} else {
				double bitRate = 0.0;

				if (fDebugState.dmc.timer_period != 0) {
					bitRate = kNTSCCPUClock / static_cast<double>(fDebugState.dmc.timer_period);
				}

				const char *stateText;

				if (fDebugState.dmc.muted) {
					stateText = "MUTED";
				} else if (fDebugState.dmc.active) {
					stateText = "ACTIVE";
				} else {
					stateText = "ON";
				}

				if (bitRate > 0.0) {
					panelTitle.SetToFormat("%s   %s   %.1f bit/s", title, stateText, bitRate);
				} else {
					panelTitle.SetToFormat("%s   %s   Bitrate: -", title, stateText);
				}
			}
			break;
		}

		case SCOPE_MIXED:
			panelTitle.SetTo(title);
			break;
	}

	::DrawDebugPanel(this, panel,
		panelTitle.String());

	if (fSampleCount != 0) {
		float minimumValue;
		float maximumVisibleValue;

		ComputeVisibleMinMax(channel, minimumValue, maximumVisibleValue);

		BString statistics;

		if (channel == SCOPE_MIXED) {
			statistics.SetToFormat("Min: %.0f   Max: %.0f", minimumValue, maximumVisibleValue);
		} else {
			statistics.SetToFormat("Min: %u   Max: %u", static_cast<unsigned>(minimumValue),
								   static_cast<unsigned>(maximumVisibleValue));
		}

		BFont prevFont;
		GetFont(&prevFont);

		BFont fixed(be_fixed_font);
		fixed.SetSize(10.0f);

		SetFont(&fixed);
		SetHighColor(70, 70, 70);

		const float statisticsWidth = StringWidth(statistics.String());
		const float statisticsX = panel.right - 10.0f - statisticsWidth;

		DrawString(statistics.String(), BPoint(statisticsX, panel.top + 17.0f));

		SetFont(&prevFont);
	}

	const float left = panel.left + 8.0f;
	const float right = panel.right - 8.0f;
	const float top = panel.top + 32.0f;
	const float bottom = panel.bottom - 8.0f;
	const float width = right - left;
	const float height = bottom - top;

	if (width <= 0.0f || height <= 0.0f) {
		return;
	}

	const float centerY = top + (height * 0.5f);

	SetPenSize(1.0f);
	SetHighColor(155, 155, 155);
	StrokeLine(BPoint(left, centerY), BPoint(right, centerY));

	if (fTriggerEnabled && !fFreezeUpdates) {

		const float triggerX = left + (width * 0.25f);

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

	switch (channel) {
		case SCOPE_SQUARE1:
			SetHighColor(0, 80, 160);
			break;

		case SCOPE_SQUARE2:
			SetHighColor(100, 40, 150);
			break;

		case SCOPE_TRIANGLE:
			SetHighColor(0, 120, 55);
			break;

		case SCOPE_NOISE:
			SetHighColor(150, 75, 0);
			break;

		case SCOPE_DMC:
			SetHighColor(170, 35, 35);
			break;

		case SCOPE_MIXED:
			SetHighColor(25, 25, 25);
			break;
	}

	SetPenSize(2.0f);

	BPoint previousPoint;

	for (uint32 visibleIndex = 0; visibleIndex < visibleCount; visibleIndex++) {
		const uint32 sampleIndex = startIndex + visibleIndex;
		const float sampleValue = SampleValue(fSamples[sampleIndex], channel);

		const float sampleX = left + (width * static_cast<float>(visibleIndex) / 
							  static_cast<float>(visibleCount - 1));
		float sampleY;

		if (channel == SCOPE_MIXED) {
			float normalized = (sampleValue - 128.0f) / 127.0f;
			const float mixedGain = 1.75f;

			normalized *= mixedGain;

			if (normalized < -1.0f) {
				normalized = -1.0f;
			}

			if (normalized > 1.0f) {
				normalized = 1.0f;
			}

			sampleY = centerY - (normalized * (height * 0.5f));
		} else {
			float normalized = sampleValue / maximumValue;
			float channelGain = 1.0f;

			switch (channel) {
				case SCOPE_SQUARE1:
				case SCOPE_SQUARE2:
				case SCOPE_DMC:
					channelGain = 1.5f;
					break;

				case SCOPE_TRIANGLE:
				case SCOPE_NOISE:
				case SCOPE_MIXED:
					break;
			}

			normalized *= channelGain;

			if (normalized < 0.0f) {
				normalized = 0.0f;
			}

			if (normalized > 1.0f) {
				normalized = 1.0f;
			}

			sampleY = bottom - (normalized * height);
		}

		const BPoint currentPoint(sampleX, sampleY);

		if (visibleIndex != 0) {
			StrokeLine(previousPoint, currentPoint);
		}

		previousPoint = currentPoint;
	}

	if (fFreezeUpdates && fCursorSample >= static_cast<int32>(startIndex) &&
		fCursorSample <= static_cast<int32>(endIndex)) {
		const uint32 cursorVisibleIndex = static_cast<uint32>(fCursorSample) - startIndex;
		const float cursorX = left + (width * static_cast<float>( cursorVisibleIndex) /
							  static_cast<float>(visibleCount - 1));

		SetPenSize(1.0f);
		SetHighColor(185, 35, 35);
		StrokeLine(BPoint(cursorX, top), BPoint(cursorX, bottom));
	}

	SetPenSize(1.0f);
}


// -----------------------------------------------------------------------------
// APUScopeView::SampleValue
//
// Returns the waveform value for one channel from an APU scope sample.
//
// Parameters:
//   sample  - Scope sample containing all captured APU channel outputs.
//   channel - Channel whose value should be returned.
//
// Returns:
//   Channel sample value as a floating-point number.
// -----------------------------------------------------------------------------
float
APUScopeView::SampleValue (const nes::apu::apu_scope_sample_t &sample, scope_channel channel) const
{
	switch (channel) {
		case SCOPE_SQUARE1:
			return static_cast<float>(sample.square1);

		case SCOPE_SQUARE2:
			return static_cast<float>(sample.square2);

		case SCOPE_TRIANGLE:
			return static_cast<float>(sample.triangle);

		case SCOPE_NOISE:
			return static_cast<float>(sample.noise);

		case SCOPE_DMC:
			return static_cast<float>(sample.dmc);

		case SCOPE_MIXED:
			return sample.mixed;
	}

	return 0.0f;
}


// -----------------------------------------------------------------------------
// APUScopeView::ComputeVisibleSampleRange
//
// Computes the currently visible scope sample range.
//
// In frozen mode, the visible window follows the inspection cursor and attempts
// to keep it centered.
//
// In live trigger mode, the most recent rising edge from the selected trigger
// source is positioned approximately one quarter of the way across the display.
//
// Without a trigger, live mode simply displays the newest available samples.
//
// Parameters:
//   startIndex - Receives the first visible sample index.
//   endIndex   - Receives the final visible sample index, inclusive.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::ComputeVisibleSampleRange (uint32 &startIndex, uint32 &endIndex) const
{
	startIndex = 0;
	endIndex = 0;

	if (fSampleCount == 0) {
		return;
	}

	const uint32 visibleCount = (fVisibleSampleCount < fSampleCount) ? fVisibleSampleCount : fSampleCount;

	/*
	 * Frozen mode always follows the inspection cursor.  Triggering does not
	 * interfere with frozen waveform inspection.
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
	 */
	if (fTriggerEnabled && visibleCount >= 2) {
		uint32 triggerIndex = 0;

		if (FindTriggerSample(fTriggerChannel, triggerIndex)) {
			const uint32 preTrigger = visibleCount / 4;
			int32 start = static_cast<int32>(triggerIndex) - static_cast<int32>(preTrigger);

			if (start < 0) {
				start = 0;
			}

			const int32 maximumStart = static_cast<int32>(fSampleCount - visibleCount);

			if (start > maximumStart) {
				start = maximumStart;
			}

			startIndex = static_cast<uint32>(start);
			endIndex = startIndex + visibleCount - 1;

			return;
		}
	}

	/*
	 * Normal live mode, or trigger enabled but no suitable edge was found.
	 */
	endIndex = fSampleCount - 1;
	startIndex = fSampleCount - visibleCount;
}


// -----------------------------------------------------------------------------
// APUScopeView::ComputeVisibleMinMax
//
// Computes the minimum and maximum sample values for one channel across the
// currently visible waveform range.
//
// The calculation follows the active zoom, trigger, and frozen-cursor window,
// so the reported statistics always describe exactly what is being displayed.
//
// Parameters:
//   channel      - Scope channel whose visible values should be examined.
//   minimumValue - Receives the minimum visible sample value.
//   maximumValue - Receives the maximum visible sample value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::ComputeVisibleMinMax (scope_channel channel, float &minimumValue, float &maximumValue) const
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

	minimumValue = SampleValue(fSamples[startIndex], channel);
	maximumValue = minimumValue;

	for (uint32 index = startIndex + 1; index <= endIndex; index++) {
		const float value = SampleValue(fSamples[index], channel);

		if (value < minimumValue) {
			minimumValue = value;
		}

		if (value > maximumValue) {
			maximumValue = value;
		}
	}
}


// -----------------------------------------------------------------------------
// APUScopeView::ClampCursor
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
APUScopeView::ClampCursor()
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
// APUScopeView::FindTriggerSample
//
// Searches backward through the captured scope history for the most recent
// rising-edge crossing on the selected trigger channel.
//
// A rising edge occurs when the previous sample is below the channel threshold
// and the current sample is at or above it.
//
// Parameters:
//   channel      - Scope channel used as the trigger source.
//   triggerIndex - Receives the matching sample index.
//
// Returns:
//   true if a rising-edge trigger was found, otherwise false.
// -----------------------------------------------------------------------------
bool
APUScopeView::FindTriggerSample (scope_channel channel, uint32 &triggerIndex) const
{
	if (fSampleCount < 2) {
		return false;
	}

	const float threshold = TriggerThreshold(channel);

	for (uint32 index = fSampleCount - 1; index > 0; index--) {
		const float previousValue = SampleValue(fSamples[index - 1], channel);
		const float currentValue = SampleValue(fSamples[index], channel);

		if (previousValue < threshold && currentValue >= threshold) {
			triggerIndex = index;
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// APUScopeView::TriggerThreshold
//
// Returns the default rising-edge trigger threshold for one scope channel.
//
// The threshold is approximately the midpoint of each channel's normal output
// range.  Mixed audio uses unsigned PCM silence ($80 / 128) as its trigger
// center.
//
// Parameters:
//   channel - Scope channel whose trigger threshold is requested.
//
// Returns:
//   Trigger threshold value.
// -----------------------------------------------------------------------------
float
APUScopeView::TriggerThreshold (scope_channel channel) const
{
	switch (channel) {
		case SCOPE_SQUARE1:
		case SCOPE_SQUARE2:
		case SCOPE_TRIANGLE:
		case SCOPE_NOISE:
			return 7.5f;

		case SCOPE_DMC:
			return 63.5f;

		case SCOPE_MIXED:
			return 128.0f;
	}

	return 0.0f;
}


// -----------------------------------------------------------------------------
// APUScopeView::TriggerChannelName
//
// Returns a short display name for the currently selected trigger source.
//
// Parameters:
//   None.
//
// Returns:
//   Human-readable trigger-channel name.
// -----------------------------------------------------------------------------
const char*
APUScopeView::TriggerChannelName() const
{
	if (!fTriggerEnabled) {
		return "OFF";
	}

	switch (fTriggerChannel) {
		case SCOPE_SQUARE1:
			return "Square 1";

		case SCOPE_SQUARE2:
			return "Square 2";

		case SCOPE_TRIANGLE:
			return "Triangle";

		case SCOPE_NOISE:
			return "Noise";

		case SCOPE_DMC:
			return "DMC";

		case SCOPE_MIXED:
			return "Mixed";
	}

	return "OFF";
}


// -----------------------------------------------------------------------------
// APUScopeView::CycleTriggerMode
//
// Advances the Scope through its available rising-edge trigger sources.
//
// The sequence is:
//   OFF -> Square 1 -> Square 2 -> Triangle -> Noise -> DMC -> Mixed -> OFF
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::CycleTriggerMode()
{
	if (!fTriggerEnabled) {
		fTriggerEnabled = true;
		fTriggerChannel = SCOPE_SQUARE1;
		return;
	}

	switch (fTriggerChannel) {
		case SCOPE_SQUARE1:
			fTriggerChannel = SCOPE_SQUARE2;
			break;

		case SCOPE_SQUARE2:
			fTriggerChannel = SCOPE_TRIANGLE;
			break;

		case SCOPE_TRIANGLE:
			fTriggerChannel = SCOPE_NOISE;
			break;

		case SCOPE_NOISE:
			fTriggerChannel = SCOPE_DMC;
			break;

		case SCOPE_DMC:
			fTriggerChannel = SCOPE_MIXED;
			break;

		case SCOPE_MIXED:
			fTriggerEnabled = false;
			fTriggerChannel = SCOPE_SQUARE1;
			break;
	}
}


// -----------------------------------------------------------------------------
// APUScopeView::ToggleChannelVisible
//
// Toggles whether one APU scope channel is visible in the normal multi-panel
// display.
//
// Solo mode takes precedence over the visibility flags.  The flags remain
// unchanged while soloing so the previous layout is restored when solo mode is
// turned off.
//
// Parameters:
//   channel - Scope channel whose visibility should be toggled.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::ToggleChannelVisible (scope_channel channel)
{
	const int32 index = static_cast<int32>(channel);

	if (index < 0 || index >= 6) {
		return;
	}

	fChannelVisible[index] = !fChannelVisible[index];
}


// -----------------------------------------------------------------------------
// APUScopeView::CycleSoloChannel
//
// Cycles through the available solo-channel modes.
//
// Solo mode temporarily expands one selected channel to occupy the entire
// waveform area.  Existing per-channel visibility selections are preserved and
// restored when solo mode returns to OFF.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeView::CycleSoloChannel()
{
	fSoloChannel++;

	if (fSoloChannel >= 6) {
		fSoloChannel = -1;
	}
}


// -----------------------------------------------------------------------------
// APUScopeView::SoloChannelName
//
// Returns the display name of the currently selected solo channel.
//
// Parameters:
//   None.
//
// Returns:
//   Current solo-channel name, or "OFF" when solo mode is disabled.
// -----------------------------------------------------------------------------
const char*
APUScopeView::SoloChannelName() const
{
	switch (fSoloChannel) {
		case SCOPE_SQUARE1:
			return "Square 1";

		case SCOPE_SQUARE2:
			return "Square 2";

		case SCOPE_TRIANGLE:
			return "Triangle";

		case SCOPE_NOISE:
			return "Noise";

		case SCOPE_DMC:
			return "DMC";

		case SCOPE_MIXED:
			return "Mixed";
	}

	return "OFF";
}


