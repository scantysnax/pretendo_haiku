
#include "APUFrameSequencerView.h"


namespace {
	const rgb_color kTextColor = { 25, 25, 25, 255 };
	const rgb_color kValueColor = { 0, 80, 145, 255 };
	const rgb_color kGoodColor = { 0, 120, 40, 255 };
	const rgb_color kBadColor = { 170, 35, 35, 255 };
	const rgb_color kWarningColor = { 155, 95, 0, 255 };
	const rgb_color kLineColor = { 100, 100, 100, 255 };
	const rgb_color kQuarterColor = { 0, 120, 40, 255 };
	const rgb_color kHalfColor = { 0, 80, 145, 255 };
	const rgb_color kIRQColor = { 170, 35, 35, 255 };
	const rgb_color kIdleColor = { 110, 110, 110, 255 };
	const rgb_color kMutedColor = { 100, 100, 100, 255 };
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::APUFrameSequencerView
//
// Creates the APU Frame Sequencer debugger view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Parent Pretendo window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUFrameSequencerView::APUFrameSequencerView (BRect frame, PretendoWindow *parent)
	: BView(frame, "apu_frame_sequencer_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	fEventCount(0),
	fFreezeUpdates(false)
{
	(void)parent;
	SetViewColor(B_TRANSPARENT_COLOR);
	CaptureState();

}


// -----------------------------------------------------------------------------
// APUSequencerView::~APUFrameSequencerView
//
// Destroys the APU Sequencer debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUFrameSequencerView::~APUFrameSequencerView()
{
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::AttachedToWindow
//
// Initializes keyboard focus and captures the current frame-sequencer state
// after the view has been attached to its window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	CaptureState();
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::CaptureState
//
// Captures the current side-effect-free APU frame-sequencer state together
// with the recent frame-event history.
//
// Both snapshots are captured together so frozen mode preserves a coherent
// debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::CaptureState()
{
	fState = nes::apu::debug_frame_sequencer_state();
	fEventCount = nes::apu::debug_frame_event_snapshot(fEvents, nes::apu::APU_FRAME_EVENT_CAPACITY);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::HasROMLoaded
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
APUFrameSequencerView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::Pulse
//
// Refreshes the frame-sequencer debugger while the view is active.
//
// When no ROM is loaded, the view is simply redrawn so the empty-state display
// remains current.
//
// In live mode, the latest frame-sequencer state and recent event history are
// captured on each pulse.  Frozen mode preserves the existing snapshot until
// the user returns the debugger to live mode.
//
// The display-only timeline animation determines whether the APU is actually
// advancing by comparing captured APU cycle counts during drawing.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::Pulse()
{
	if (!HasROMLoaded()) {
		Invalidate();
		return;
	}

	if (!fFreezeUpdates) {
		CaptureState();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::KeyDown
//
// Handles keyboard interaction for the Frame Sequencer debugger.
//
// Controls:
//   Space - Toggle live/frozen display.
//   C     - Clear recent frame-event history.
//
// Parameters:
//   bytes    - Keyboard input bytes.
//   numBytes - Number of bytes supplied.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == ' ') {
		if (!HasROMLoaded()) {
			return;
		}

		fFreezeUpdates = !fFreezeUpdates;

		if (!fFreezeUpdates) {
			CaptureState();
		}

		Invalidate();
		return;
	}
	
	if (numBytes == 1 && (bytes[0] == 'c' || bytes[0] == 'C')) {
		if (!HasROMLoaded()) {
			return;
		}

		nes::apu::debug_clear_frame_events();
		fEventCount = 0;

		Invalidate();
		return;
	}
	
	BView::KeyDown(bytes, numBytes);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::Draw
//
// Draws the APU Frame Sequencer debugger.
//
// The view contains the header, live frame-sequencer timeline, raw sequencer
// state, and a recent frame-event history.
//
// Parameters:
//   updateRect - Region requiring redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		DrawNoROMMessage();
		return;
	}

	DrawTimelinePanel();
	DrawStatePanel();
	DrawRecentEventsPanel();
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::DrawHeaderPanel
//
// Draws the debugger title, control help, and LIVE/FROZEN state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 66.0f);
	::DrawDebugPanel(this, panel, "APU Frame Sequencer");

	BFont prevFont;
	GetFont(&prevFont);

	BFont font(be_plain_font);
	font.SetSize(10.0f);
	SetFont(&font);

	const float x = panel.left + 10.0f;
	const float controlY = panel.top + 42.0f;

	SetHighColor(kTextColor);
	DrawString("Space: Freeze  C: Clear Events", BPoint(x, controlY));

	BString state;
	state.SetToFormat("[%s]", fFreezeUpdates ? "FROZEN" : "LIVE");

	const float stateWidth = StringWidth(state.String());
	const float stateX = panel.right - 10.0f - stateWidth;

	if (fFreezeUpdates) {
		SetHighColor(170, 35, 35);
	} else {
		SetHighColor(0, 120, 40);
	}

	DrawString(state.String(), BPoint(stateX, controlY));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::DrawStatePanel
//
// Draws the raw frame-sequencer debugger snapshot.
//
// This initial implementation intentionally presents the backend state directly.
// The visual frame timeline is shown above this panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::DrawStatePanel()
{
	BRect panel(4.0f, 236.0f, Bounds().right - 4.0f, 530);
	::DrawDebugPanel(this, panel, "Sequencer State");

	BFont prevFont;
	GetFont(&prevFont);

	BFont font(be_plain_font);
	font.SetSize(11.0f);
	SetFont(&font);
	
	float y = panel.top + 34.0f;
	const float labelX = panel.left + 14.0f;
	const float valueX = panel.left + 190.0f;
	const float lineHeight = 22.0f;

	BString text;
	SetHighColor(kTextColor);
	DrawString("Mode:", BPoint(labelX, y));

	text.SetToFormat("%s", fState.five_step_mode ? "5-Step" : "4-Step");
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Next Internal Step:", BPoint(labelX, y));

	text.SetToFormat("%u / %u", static_cast<unsigned>(fState.next_step),
								static_cast<unsigned>(fState.step_count));
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("APU Cycle:", BPoint(labelX, y));

	text.SetToFormat("%llu", static_cast<unsigned long long>(fState.apu_cycle));
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Next Event Cycle:", BPoint(labelX, y));

	text.SetToFormat("%llu", static_cast<unsigned long long>(fState.next_event_cycle));
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Cycles Remaining:", BPoint(labelX, y));

	text.SetToFormat("%llu", static_cast<unsigned long long>(fState.cycles_until_next_event));
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));
	y += lineHeight * 1.5f;

	SetHighColor(kTextColor);
	DrawString("Next Quarter Frame:", BPoint(labelX, y));

	SetHighColor(fState.next_event_quarter_frame ? kGoodColor : kTextColor);
	DrawString(fState.next_event_quarter_frame ? "Yes" : "No", BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Next Half Frame:", BPoint(labelX, y));

	SetHighColor(fState.next_event_half_frame ? kGoodColor : kTextColor);
	DrawString(fState.next_event_half_frame ? "Yes" : "No", BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Next IRQ Point:", BPoint(labelX, y));

	SetHighColor(fState.next_event_irq_point ? kWarningColor : kTextColor);
	DrawString(fState.next_event_irq_point ? "Yes" : "No", BPoint(valueX, y));
	y += lineHeight * 1.5f;

	SetHighColor(kTextColor);
	DrawString("IRQ Inhibit:", BPoint(labelX, y));

	SetHighColor(fState.irq_inhibit ? kWarningColor : kGoodColor);
	DrawString(fState.irq_inhibit ? "Yes" : "No", BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("Frame IRQ:", BPoint(labelX, y));

	SetHighColor(fState.frame_irq ? kBadColor : kGoodColor);
	DrawString(fState.frame_irq ? "Yes" : "No", BPoint(valueX, y));
	y += lineHeight;

	SetHighColor(kTextColor);
	DrawString("$4017:", BPoint(labelX, y));

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);
	SetFont(&fixed);

	text.SetToFormat("$%02X", static_cast<unsigned>(fState.frame_counter_register));
	SetHighColor(kValueColor);
	DrawString(text.String(), BPoint(valueX, y));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::DrawTimelinePanel
//
// Draws the timing structure of the active APU frame-sequencer mode together
// with a readable visual representation of sequence progression.
//
// Event-marker positions are based directly on the timing used by the
// emulator's frame-counter implementation.
//
// The exact sampled position is reconstructed from the next scheduled frame
// event and the number of APU cycles remaining until that event:
//
//     sampled position = next event position - cycles remaining
//
// The calculation wraps around the end of the logical sequence when necessary.
//
// In 4-step mode, six internal scheduling points are shown.  The final three
// points are intentionally extremely close because the current APU
// implementation performs the IRQ/final quarter+half-frame activity on
// consecutive APU cycles.
//
// A small bracket is drawn beneath those final three points to emphasize that
// they form one tightly grouped IRQ/final-frame region.
//
// In 5-step mode, five internal scheduling points are shown and no frame IRQ
// points are present.
//
// Because the real NES frame sequencer advances too quickly to animate clearly
// at debugger refresh rates, the blue marker is a display-only sweep running at
// a readable rate.  The exact captured sequencer position is shown separately.
//
// The display sweep advances only while captured APU cycles are advancing.
// Loading a ROM without running it, pausing emulation, or freezing this debugger
// therefore leaves the marker stationary.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::DrawTimelinePanel()
{
	BRect panel(4.0f, 72.0f, Bounds().right - 4.0f, 230.0f);
	::DrawDebugPanel(this, panel, fState.five_step_mode ? "Frame Timeline - 5-Step"
														: "Frame Timeline - 4-Step");

	BFont prevFont;
	GetFont(&prevFont);

	BFont font(be_plain_font);
	font.SetSize(10.0f);
	SetFont(&font);

	const rgb_color currentColor = { 0, 90, 170, 255 };

	const float left = panel.left + 24.0f;
	const float right = panel.right - 24.0f;
	const float timelineY = panel.top + 64.0f;
	const float labelY = timelineY - 18.0f;
	const float detailY = timelineY + 34.0f;
	const float width = right - left;

	SetPenSize(1.0f);
	SetHighColor(kLineColor);
	StrokeLine(BPoint(left, timelineY), BPoint(right, timelineY));

	uint32 currentSequenceCycle = 0;
	uint32 sequenceCycles = 1;

	if (!fState.five_step_mode) {
		const uint32 eventCycles[6] = {
			0,
			7456,
			14914,
			22371,
			22372,
			22373
		};

		sequenceCycles = 29830;

		for (int32 index = 0; index < 6; index++) {
			const float normalized = static_cast<float>(eventCycles[index]) /
									 static_cast<float>(sequenceCycles);

			const float x = left + (width * normalized);

			SetHighColor(kLineColor);
			StrokeLine(BPoint(x, timelineY - 7.0f), BPoint(x, timelineY + 7.0f));

			const char *eventName = "";
			rgb_color eventColor = kTextColor;

			switch (index) {
				case 0:
					eventName = "Quarter";
					eventColor = kQuarterColor;
					break;

				case 1:
					eventName = "Quarter + Half";
					eventColor = kHalfColor;
					break;

				case 2:
					eventName = "Quarter";
					eventColor = kQuarterColor;
					break;

				case 3:
					eventName = "IRQ";
					eventColor = kIRQColor;
					break;

				case 4:
					eventName = "Quarter + Half + IRQ";
					eventColor = kIRQColor;
					break;

				case 5:
					eventName = "IRQ";
					eventColor = kIRQColor;
					break;
			}

			SetHighColor(eventColor);

			float textX = x - (StringWidth(eventName) * 0.5f);
			float textY = labelY;

			if (index == 3) {
				textY = labelY - 12.0f;
			} else if (index == 4) {
				textY = labelY;
			} else if (index == 5) {
				textY = labelY + 12.0f;
			}

			if (index == 0 && textX < left) {
				textX = left + 2.0f;
			}

			DrawString(eventName, BPoint(textX, textY));
		}

		/*
		 * Visually group the final three closely spaced IRQ/final-frame
		 * scheduling points.
		 */
		const float clusterStartX = left + (width * (static_cast<float>(eventCycles[3]) /
													 static_cast<float>(sequenceCycles)));

		const float clusterEndX = left + (width * (static_cast<float>(eventCycles[5]) /
												   static_cast<float>(sequenceCycles)));

		const float clusterY = timelineY + 13.0f;

		SetHighColor(kIRQColor);
		SetPenSize(1.0f);

		StrokeLine(BPoint(clusterStartX, clusterY), BPoint(clusterEndX, clusterY));
		StrokeLine(BPoint(clusterStartX, clusterY - 3.0f), BPoint(clusterStartX, clusterY + 3.0f));
		StrokeLine(BPoint(clusterEndX, clusterY - 3.0f), BPoint(clusterEndX, clusterY + 3.0f));

		if (fState.next_step < 6) {
			const int64 nextPosition = static_cast<int64>(eventCycles[fState.next_step]);
			int64 currentPosition = nextPosition - static_cast<int64>(fState.cycles_until_next_event);

			while (currentPosition < 0) {
				currentPosition += static_cast<int64>(sequenceCycles);
			}

			currentSequenceCycle = static_cast<uint32>(currentPosition % sequenceCycles);
		}

		SetHighColor(kTextColor);
		DrawString("Quarter = Envelope + Triangle Linear Counter", BPoint(left, detailY));
		DrawString("Half = Length Counters + Square Sweep", BPoint(left, detailY + 15.0f));

		SetHighColor(kIRQColor);
		DrawString("IRQ = Frame IRQ point when not inhibited", BPoint(left, detailY + 30.0f));
	} else {
		const uint32 eventCycles[5] = {
			0,
			7458,
			14914,
			22372,
			29828
		};

		sequenceCycles = 37282;

		for (int32 index = 0; index < 5; index++) {
			const float normalized = static_cast<float>(eventCycles[index]) /
									 static_cast<float>(sequenceCycles);

			const float x = left + (width * normalized);

			SetHighColor(kLineColor);
			StrokeLine(BPoint(x, timelineY - 7.0f), BPoint(x, timelineY + 7.0f));

			const char *eventName = "";
			rgb_color eventColor = kTextColor;

			switch (index) {
				case 0:
					eventName = "Quarter + Half";
					eventColor = kHalfColor;
					break;

				case 1:
					eventName = "Quarter";
					eventColor = kQuarterColor;
					break;

				case 2:
					eventName = "Quarter + Half";
					eventColor = kHalfColor;
					break;

				case 3:
					eventName = "Quarter";
					eventColor = kQuarterColor;
					break;

				case 4:
					eventName = "-";
					eventColor = kIdleColor;
					break;
			}

			SetHighColor(eventColor);

			float textX = x - (StringWidth(eventName) * 0.5f);
			float textY = labelY;

			/*
			 * The first two 5-step labels are close enough that centering
			 * both can make them overlap.  Stagger them vertically while
			 * preserving their relationship to the correct event markers.
			 */
			if (index == 0) {
				textX = x + 4.0f;
				textY = labelY - 10.0f;
			} else if (index == 1) {
				textY = labelY + 10.0f;
			}

			DrawString(eventName, BPoint(textX, textY));
		}

		if (fState.next_step < 5) {
			const int64 nextPosition = static_cast<int64>(eventCycles[fState.next_step]);
			int64 currentPosition = nextPosition - static_cast<int64>(fState.cycles_until_next_event);

			while (currentPosition < 0) {
				currentPosition += static_cast<int64>(sequenceCycles);
			}

			currentSequenceCycle = static_cast<uint32>(currentPosition % sequenceCycles);
		}

		SetHighColor(kTextColor);
		DrawString("Quarter = Envelope + Triangle Linear Counter", BPoint(left, detailY));
		DrawString("Half = Length Counters + Square Sweep", BPoint(left, detailY + 15.0f));

		SetHighColor(kIdleColor);
		DrawString("5-step mode does not generate a frame IRQ", BPoint(left, detailY + 30.0f));
	}

	/*
	 * Advance the display-only timeline marker using elapsed wall-clock time.
	 *
	 * The marker advances only when the captured APU cycle count itself is
	 * changing.  This keeps the visual sweep stopped when a ROM is merely
	 * loaded, when emulation is paused, or when the debugger view is frozen.
	 */
	const float sequenceLength = static_cast<float>(sequenceCycles);
	const float targetSequenceCycle = static_cast<float>(currentSequenceCycle);
	const bigtime_t now = system_time();

	if (!fHaveDisplayedSequenceCycle) {
		fDisplayedSequenceCycle = targetSequenceCycle;

		fHaveDisplayedSequenceCycle = true;
		fLastDisplayTime = now;
		fLastDisplayedAPUCycle = fState.apu_cycle;
	}

	const bool apuRunning = fState.apu_cycle != fLastDisplayedAPUCycle;

	fLastDisplayedAPUCycle = fState.apu_cycle;

	if (fFreezeUpdates || !apuRunning) {
		/*
		 * Preserve the marker while the debugger is frozen or while the APU
		 * itself is not advancing.
		 *
		 * Resetting the timestamp prevents stopped or paused time from being
		 * counted as elapsed animation time when execution resumes.
		 */
		fLastDisplayTime = now;
	} else {
		if (fLastDisplayTime != 0) {
			const bigtime_t elapsed = now - fLastDisplayTime;
			const float elapsedSeconds = static_cast<float>(elapsed) / 1000000.0f;

			/*
			 * Advance by one complete displayed sequence per second.
			 */
			fDisplayedSequenceCycle += sequenceLength * elapsedSeconds;

			while (fDisplayedSequenceCycle >= sequenceLength) {
				fDisplayedSequenceCycle -= sequenceLength;
			}

			while (fDisplayedSequenceCycle < 0.0f) {
				fDisplayedSequenceCycle += sequenceLength;
			}
		}

		fLastDisplayTime = now;
	}

	/*
	 * Draw the smooth display-only position indicator.
	 */
	const float currentNormalized = fDisplayedSequenceCycle / sequenceLength;
	const float currentX = left + (width * currentNormalized);

	SetHighColor(currentColor);
	SetPenSize(2.0f);
	StrokeLine(BPoint(currentX, timelineY - 11.0f), BPoint(currentX, timelineY + 11.0f));
	FillTriangle(BPoint(currentX - 5.0f, timelineY - 15.0f),
				 BPoint(currentX + 5.0f, timelineY - 15.0f),
				 BPoint(currentX, timelineY - 9.0f));

	/*
	 * Show the exact captured sequencer position separately from the
	 * display-only sweep marker so the two are not mistaken for the same
	 * position.
	 */
	BString sampledText;

	sampledText.SetToFormat("Sampled position: %u", static_cast<unsigned>(currentSequenceCycle));
	const float sampledTextWidth = StringWidth(sampledText.String());

	SetHighColor(kValueColor);
	DrawString(sampledText.String(), BPoint(right - sampledTextWidth, timelineY + 24.0f));

	SetPenSize(1.0f);
	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::DrawRecentEventsPanel
//
// Draws the most recent APU frame-sequencer events.
//
// The event history is captured together with the main sequencer snapshot, so
// frozen mode preserves both the timeline and the event list.
//
// Events are displayed newest first.  Expanded event names are used so users do
// not need to interpret abbreviated Quarter/Half terminology.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::DrawRecentEventsPanel()
{
	BRect panel(4.0f, 536.0f, Bounds().right - 4.0f, Bounds().bottom - 4.0f);
	::DrawDebugPanel(this, panel, "Recent Frame Events");

	BFont prevFont;
	GetFont(&prevFont);

	BFont font(be_plain_font);
	font.SetSize(10.0f);
	SetFont(&font);

	const float cycleX = panel.left + 12.0f;
	const float eventX = panel.left + 130.0f;
	const float stepX = panel.left + 250.0f;
	const float modeX = panel.left + 300.0f;
	const float inhibitX = panel.left + 370.0f;
	float y = panel.top + 36.0f;
	const float lineHeight = 16.0f;

	SetHighColor(kMutedColor);
	DrawString("Cycle", BPoint(cycleX, y));
	DrawString("Event", BPoint(eventX, y));
	DrawString("Step", BPoint(stepX, y));
	DrawString("Mode", BPoint(modeX, y));
	DrawString("IRQ Inhibited", BPoint(inhibitX, y));
	y += lineHeight;

	if (fEventCount == 0) {
		SetHighColor(kMutedColor);
		DrawString("No frame events captured", BPoint(cycleX, y));

		SetFont(&prevFont);

		return;
	}

	const float availableHeight = panel.bottom - y - 8.0f;
	int32 maximumRows = static_cast<int32>(availableHeight / lineHeight);

	if (maximumRows < 1) {
		maximumRows = 1;
	}

	int32 rowsToDraw = static_cast<int32>(fEventCount);

	if (rowsToDraw > 8) {
		rowsToDraw = 8;
	}

	if (rowsToDraw > maximumRows) {
		rowsToDraw = maximumRows;
	}
	
	for (int32 row = 0; row < rowsToDraw; row++) {
		const int32 eventIndex = static_cast<int32>(fEventCount) - 1 - row;
		const nes::apu::frame_event_t &event = fEvents[eventIndex];

		BString text;
		text.SetToFormat("%llu", static_cast<unsigned long long>(event.cycle));

		SetHighColor(kValueColor);
		DrawString(text.String(), BPoint(cycleX, y));

		const char *eventName = "";
		rgb_color eventColor = kTextColor;

		switch (event.type) {
			case nes::apu::APU_FRAME_EVENT_QUARTER:
				eventName = "Quarter";
				eventColor = kQuarterColor;
				break;

			case nes::apu::APU_FRAME_EVENT_HALF:
				eventName = "Half";
				eventColor = kHalfColor;
				break;

			case nes::apu::APU_FRAME_EVENT_QUARTER_HALF:
				eventName = "Quarter + Half";
				eventColor = kHalfColor;
				break;

			case nes::apu::APU_FRAME_EVENT_QUARTER_HALF_IRQ:
				eventName = "Quarter + Half + IRQ";
				eventColor = kIRQColor;
				break;

			case nes::apu::APU_FRAME_EVENT_IRQ:
				eventName = "IRQ";
				eventColor = kIRQColor;
				break;
		}

		SetHighColor(eventColor);
		DrawString(eventName, BPoint(eventX, y));

		text.SetToFormat("%u", static_cast<unsigned>(event.step));
		SetHighColor(kTextColor);
		DrawString(text.String(), BPoint(stepX, y));
		DrawString(event.five_step_mode ? "5-Step" : "4-Step", BPoint(modeX, y));

		if (event.irq_inhibit) {
			SetHighColor(kIRQColor);
			DrawString("Yes", BPoint(inhibitX, y));
		} else {
			SetHighColor(kQuarterColor);
			DrawString("No", BPoint(inhibitX, y));
		}

		y += lineHeight;
	}

	SetFont(&prevFont);
}

// -----------------------------------------------------------------------------
// APUFrameSequencerView::DrawNoROMMessage
//
// Draws the empty-state message when no cartridge is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::DrawNoROMMessage()
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(12.0f);
	SetFont(&fixed);

	SetHighColor(90, 90, 90);

	const char *message = "No ROM loaded";
	const float width = StringWidth(message);

	DrawString(message, BPoint((Bounds().Width() - width) * 0.5f, Bounds().Height() * 0.5f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUFrameSequencerView::ResetView
//
// Resets debugger-local state after a new ROM is loaded.
//
// A new cartridge starts the Frame Sequencer debugger in live mode and clears
// display-only timeline state so the marker is initialized from the new APU
// sequencer state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUFrameSequencerView::ResetView()
{
	fFreezeUpdates = false;

	fHaveDisplayedSequenceCycle = false;
	fLastDisplayTime = 0;
	fLastDisplayedAPUCycle = 0;

	CaptureState();

	Invalidate();
}

