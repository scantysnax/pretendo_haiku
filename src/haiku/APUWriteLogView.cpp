
#include "APUWriteLogView.h"


// -----------------------------------------------------------------------------
// APUWriteLogScrollBar
//
// Vertical scrollbar used by APUWriteLogView.  Instead of scrolling the target
// BView directly, changes are forwarded into the view's retained-history scroll
// state so the fixed header remains stationary.
// -----------------------------------------------------------------------------
class APUWriteLogScrollBar : public BScrollBar
{
	public:
	APUWriteLogScrollBar (BRect frame, APUWriteLogView *owner);

	protected:
	virtual void ValueChanged (float newValue);

	private:
	APUWriteLogView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// APUWriteLogScrollBar::APUWriteLogScrollBar
//
// Creates the vertical scrollbar used by the APU Write Log.
//
// Parameters:
//   frame - Rectangle defining the scrollbar bounds.
//   owner - APUWriteLogView that owns the scrolling state.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUWriteLogScrollBar::APUWriteLogScrollBar (BRect frame, APUWriteLogView *owner)
	: BScrollBar(frame, "apu_write_log_scroll_bar", nullptr, 0.0f, 0.0f, B_VERTICAL)
{
	fOwner = owner;
}


// -----------------------------------------------------------------------------
// APUWriteLogScrollBar::ValueChanged
//
// Forwards scrollbar movement to the owning APU Write Log view.
//
// Parameters:
//   newValue - New scrollbar position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogScrollBar::ValueChanged (float newValue)
{
	BScrollBar::ValueChanged(newValue);

	if (fOwner) {
		fOwner->ScrollBarValueChanged(newValue);
	}
}


// -----------------------------------------------------------------------------
// APUWriteLogView::APUWriteLogView
//
// Constructs the APU Write Log debugger view.
//
// The view displays a live rolling history of CPU writes to APU registers.
// Keyboard focus, live/frozen snapshot handling, and log drawing are managed by
// the view after it is attached to a window.
//
// The parent argument is retained for consistency with the other debugger-view
// constructors but is not currently required by this view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow; currently unused.
//
// Returns:
//   Constructed APUWriteLogView instance.
// -----------------------------------------------------------------------------
APUWriteLogView::APUWriteLogView(BRect frame, PretendoWindow *parent)
	: BView(frame, "apu_write_log_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	(void)parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// APUWriteLogView::~APUWriteLogView
//
// Destroys the APU Write Log debugger view.
//
// No additional cleanup is currently required because debugger snapshot storage
// is managed automatically and the view does not own external APU resources.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUWriteLogView::~APUWriteLogView()
{
}


// -----------------------------------------------------------------------------
// APUWriteLogView::AttachedToWindow
//
// Completes APU Write Log view setup after attachment to a window.
//
// The view receives keyboard focus immediately and captures the current log so
// the first redraw does not need to wait for a pulse.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	const float scrollBarWidth = B_V_SCROLL_BAR_WIDTH;

	BRect scrollFrame(Bounds().right - scrollBarWidth - 4.0f, 70.0f, 
					  Bounds().right - 4.0f,Bounds().bottom - 8.0f);

	fScrollBar = new APUWriteLogScrollBar(scrollFrame, this);
	AddChild(fScrollBar);

	if (HasROMLoaded()) {
		CaptureLogSnapshot();
	}

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::Pulse
//
// Periodically refreshes the APU write-log snapshot while the view is live and
// following the newest entries.  When browsing retained history, the current
// debugger-owned snapshot is preserved so the visible entries remain stable.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::Pulse()
{
	if (!HasROMLoaded()) {
		if (!fLogSnapshot.empty()) {
			fLogSnapshot.clear();

			fFirstVisibleRow = 0;
			fFollowNewest = true;

			UpdateScrollBar();
			Invalidate();
		}

		return;
	}

	if (fFreezeUpdates) {
		return;
	}

	if (!fFollowNewest) {
		return;
	}

	CaptureLogSnapshot();
	Invalidate();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::CaptureLogSnapshot
//
// Captures the current APU write log into debugger-owned storage.
//
// The APU core performs the ring-buffer traversal using one captured logical
// starting position, preventing the debugger from seeing a different ring
// origin for every entry while the emulator is running.
//
// The resulting snapshot is stored oldest to newest.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::CaptureLogSnapshot()
{
	const uint32 count = nes::apu::apu_write_log_count();
	fLogSnapshot.resize(count);

	if (count != 0) {
		const uint32 copied = nes::apu::apu_write_log_snapshot(fLogSnapshot.data(), count);
		fLogSnapshot.resize(copied);
	}

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::KeyDown
//
// Handles keyboard controls for the APU write-log debugger.
//
// Space toggles live/frozen state.  Entering freeze captures the current log
// immediately so the frozen display represents the state at the moment the key
// was pressed.
//
// C clears both the emulator write log and the debugger-side snapshot.
//
// Parameters:
//   bytes    - Key bytes received from the keyboard event.
//   numBytes - Number of bytes in the key event.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		if (bytes[0] == ' ') {
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();

			return;
		}

		BView::KeyDown(bytes, numBytes);

		return;
	}

	switch (bytes[0]) {
		case ' ':
		{
			if (!fFreezeUpdates) {
				CaptureLogSnapshot();
				fFreezeUpdates = true;
			} else {
				fFreezeUpdates = false;
				CaptureLogSnapshot();

				if (fFollowNewest) {
					FollowNewest();
				}
			}

			Invalidate();
			break;
		}

		case 'c':
		case 'C':
		{
			nes::apu::clear_apu_write_log();

			fLogSnapshot.clear();
			fFirstVisibleRow = 0;
			fFollowNewest = true;

			UpdateScrollBar();
			Invalidate();
			break;
		}
		
		case B_UP_ARROW:
			ScrollRows(-1);
			break;

		case B_DOWN_ARROW:
			ScrollRows(1);
			break;

		case B_PAGE_UP:
			ScrollPages(-1);
			break;

		case B_PAGE_DOWN:
			ScrollPages(1);
			break;

		case B_END:
			FollowNewest();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// APUWriteLogView::MessageReceived
//
// Handles messages sent to the APU Write Log view.  Mouse-wheel messages are
// translated into row scrolling while all other messages are passed to BView.
//
// Parameters:
//   message - Message received by the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::MessageReceived(BMessage *message)
{
	if (!message) {
		return;
	}

	if (message->what == B_MOUSE_WHEEL_CHANGED) {
		float deltaY = 0.0f;

		if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK) {
			if (deltaY != 0.0f) {
				int32 rows = static_cast<int32>(deltaY * 3.0f);

				if (rows == 0) {
					rows = deltaY > 0.0f ? 1 : -1;
				}

				ScrollRows(rows);
			}
		}

		return;
	}

	BView::MessageReceived(message);
}


// -----------------------------------------------------------------------------
// APUWriteLogView::Draw
//
// Draws the complete APU Write Log debugger.
//
// The background and header are drawn first.  If no ROM is loaded, a friendly
// empty-state panel is shown.  Otherwise the current debugger-side log snapshot
// is rendered in the Recent Writes panel.
//
// Parameters:
//   updateRect - Area of the view requested for redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(4.0f, 70.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
		
		::DrawDebugPanel(this, panel, "Recent Writes");
		DrawNoROMMessage(panel);

		return;
	}

	DrawLogPanel();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::DrawHeaderPanel
//
// Draws the title/help panel for the APU write log viewer.  The panel shows the
// available keyboard controls together with the current freeze state and whether
// the view is following the newest log entries or browsing older history.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 58.0f);

	::DrawDebugPanel(this, panel, "APU Write Log");

	SetFontSize(11.0f);

	BString line;

	line.SetToFormat("Space: %s   C: clear   PgUp/PgDn: scroll   End: newest   View: %s   State: %s",
					(fFreezeUpdates ? "resume" : "freeze"), (fFollowNewest ? "FOLLOW" : "HISTORY"),
					(fFreezeUpdates ? "FROZEN" : "LIVE"));

	if (fFreezeUpdates) {
		SetHighColor(160, 80, 0);
	} else if (!fFollowNewest) {
		SetHighColor(70, 70, 140);
	} else {
		SetHighColor(35, 35, 35);
	}

	DrawString(line.String(), BPoint(panel.left + 8.0f, panel.top + 42.0f));
}


// -----------------------------------------------------------------------------
// APUWriteLogView::DrawLogPanel
//
// Draws the debugger-side APU write-log snapshot.
//
// The newest entries that fit in the panel are selected while preserving
// chronological order from top to bottom.  Older writes therefore appear above
// newer writes, with the most recent captured write at the bottom.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::DrawLogPanel()
{
	BRect panel(4.0f, 70.0f, Bounds().right - B_V_SCROLL_BAR_WIDTH - 8.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Recent Writes");

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float cycleX = panel.left + 8.0f;
	const float regX = cycleX + 92.0f;
	const float valX = regX + 100.0f;
	const float channelX = valX + 52.0f;
	const float descX = channelX + 82.0f;

	float y = panel.top + 48.0f;

	SetHighColor(80, 80, 80);

	DrawString("Cycle", BPoint(cycleX, y));
	DrawString("Register", BPoint(regX, y));
	DrawString("Value", BPoint(valX, y));
	DrawString("Channel", BPoint(channelX, y));
	DrawString("Meaning", BPoint(descX, y));
	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));
	y += 6.0f;

	const uint32 count = static_cast<uint32>(fLogSnapshot.size());

	if (count == 0) {
		SetHighColor(90, 90, 90);
		DrawString("No APU writes logged yet.", BPoint(cycleX, y + lineH));

		SetFont(&prevFont);

		return;
	}

	const int32 visibleRows = VisibleRowCount();
	int32 firstIndex = 0;

	if (fFollowNewest) {
		if (static_cast<int32>(count) > visibleRows) {
			firstIndex = static_cast<int32>(count) - visibleRows;
		}
	} else {
		firstIndex = fFirstVisibleRow;
	}

	if (firstIndex < 0) {
		firstIndex = 0;
	}

	const int32 maxFirstIndex = (static_cast<int32>(count) > visibleRows)
							  ? (static_cast<int32>(count) - visibleRows) : 0;

	if (firstIndex > maxFirstIndex) {
		firstIndex = maxFirstIndex;
	}

	int32 rows = static_cast<int32>(count) - firstIndex;

	if (rows > visibleRows) {
		rows = visibleRows;
	}

	BString s;
	BString desc;

	for (int32 row = 0; row < rows; row++) {
		const int32 index = firstIndex + row;
		const nes::apu::apu_write_log_entry_t &entry = fLogSnapshot[index];

		DescribeWrite(entry, desc);

		switch (entry.address) {
			case 0x4000:
			case 0x4001:
			case 0x4002:
			case 0x4003:
				SetHighColor(0, 70, 150);
				break;

			case 0x4004:
			case 0x4005:
			case 0x4006:
			case 0x4007:
				SetHighColor(0, 100, 130);
				break;

			case 0x4008:
			case 0x400a:
			case 0x400b:
				SetHighColor(0, 110, 70);
				break;

			case 0x400c:
			case 0x400e:
			case 0x400f:
				SetHighColor(120, 70, 0);
				break;

			case 0x4010:
			case 0x4011:
			case 0x4012:
			case 0x4013:
				SetHighColor(110, 0, 120);
				break;

			case 0x4015:
			case 0x4017:
				SetHighColor(150, 30, 30);
				break;

			default:
				SetHighColor(0, 0, 0);
				break;
		}

		s.SetToFormat("%llu", static_cast<unsigned long long>(entry.cycle));
		DrawString(s.String(), BPoint(cycleX, y));
		DrawString(RegisterName(entry.address), BPoint(regX, y));

		s.SetToFormat("$%02X", entry.value);
		DrawString(s.String(), BPoint(valX, y));
		DrawString(ChannelName(entry.address), BPoint(channelX, y));
		DrawString(desc.String(), BPoint(descX, y));
		y += lineH;
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// APUWriteLogView::VisibleRowCount
//
// Calculates how many APU write-log rows fit inside the Recent Writes panel.
//
// Parameters:
//   None.
//
// Returns:
//   Number of visible log rows that fit in the panel.
// -----------------------------------------------------------------------------
int32
APUWriteLogView::VisibleRowCount() const
{
	BRect panel(4.0f, 70.0f, Bounds().right - B_V_SCROLL_BAR_WIDTH - 8.0f, Bounds().bottom - 8.0f);
	
	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);

	font_height fh;
	fixed.GetHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;
	const float firstRowY = panel.top + 48.0f + lineH + 8.0f + 6.0f;
	const int32 rows = static_cast<int32>((panel.bottom - firstRowY - 8.0f) / lineH);

	return (rows > 0) ? rows : 0;
}


// -----------------------------------------------------------------------------
// APUWriteLogView::ScrollRows
//
// Scrolls the visible APU write-log history by the requested number of rows.
// Scrolling upward disables automatic following of the newest entries.  If the
// scroll position reaches the newest possible page, follow mode is restored.
//
// Parameters:
//   rows - Signed number of rows to move.  Negative values move toward older
//          entries; positive values move toward newer entries.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::ScrollRows (int32 rows)
{
	const int32 count = static_cast<int32>(fLogSnapshot.size());
	const int32 visibleRows = VisibleRowCount();

	if (count <= visibleRows || visibleRows <= 0) {
		fFirstVisibleRow = 0;
		fFollowNewest = true;

		Invalidate();
		return;
	}

	const int32 newestFirstRow = count - visibleRows;

	if (fFollowNewest) {
		fFirstVisibleRow = newestFirstRow;
	}

	fFirstVisibleRow += rows;

	if (fFirstVisibleRow < 0) {
		fFirstVisibleRow = 0;
	}

	if (fFirstVisibleRow >= newestFirstRow) {
		fFirstVisibleRow = newestFirstRow;
		fFollowNewest = true;
	} else {
		fFollowNewest = false;
	}
	
	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::ScrollPages
//
// Scrolls the visible APU write-log history by whole visible pages.
//
// Parameters:
//   pages - Signed number of pages to move.  Negative values move toward older
//           entries; positive values move toward newer entries.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::ScrollPages (int32 pages)
{
	const int32 visibleRows = VisibleRowCount();

	if (visibleRows <= 0) {
		return;
	}

	ScrollRows(pages * visibleRows);
}


// -----------------------------------------------------------------------------
// APUWriteLogView::FollowNewest
//
// Returns the APU write-log view to the newest available entries and enables
// automatic following.  When the view is live, the latest core log snapshot is
// captured immediately before positioning the view at the bottom.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::FollowNewest()
{
	fFollowNewest = true;

	if (!fFreezeUpdates && HasROMLoaded()) {
		CaptureLogSnapshot();
	}

	const int32 count = static_cast<int32>(fLogSnapshot.size());
	const int32 visibleRows = VisibleRowCount();

	if (count > visibleRows && visibleRows > 0) {
		fFirstVisibleRow = count - visibleRows;
	} else {
		fFirstVisibleRow = 0;
	}

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// APUWriteLogView::UpdateScrollBar
//
// Updates the APU write-log scrollbar range, step sizes, thumb proportion, and
// current value to match the retained log snapshot and visible row count.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const int32 count = static_cast<int32>(fLogSnapshot.size());
	const int32 visibleRows = VisibleRowCount();

	if (count <= 0 || visibleRows <= 0 || count <= visibleRows) {
		fFirstVisibleRow = 0;
		fFollowNewest = true;

		fScrollBar->SetRange(0.0f, 0.0f);
		fScrollBar->SetSteps(1.0f, 1.0f);
		fScrollBar->SetProportion(1.0f);
		fScrollBar->SetValue(0.0f);

		return;
	}

	const int32 maxFirstRow = count - visibleRows;

	if (fFollowNewest) {
		fFirstVisibleRow = maxFirstRow;
	} else {
		if (fFirstVisibleRow < 0) {
			fFirstVisibleRow = 0;
		}

		if (fFirstVisibleRow > maxFirstRow) {
			fFirstVisibleRow = maxFirstRow;
		}
	}

	fScrollBar->SetRange(0.0f, static_cast<float>(maxFirstRow));
	fScrollBar->SetSteps(1.0f, static_cast<float>(visibleRows));
	fScrollBar->SetProportion(static_cast<float>(visibleRows) / static_cast<float>(count));
	fScrollBar->SetValue(static_cast<float>(fFirstVisibleRow));
}


// -----------------------------------------------------------------------------
// APUWriteLogView::ScrollBarValueChanged
//
// Updates the visible APU write-log position when the scrollbar is moved.
// Moving the scrollbar away from the newest page disables automatic following;
// returning it to the bottom restores follow-newest mode.
//
// Parameters:
//   value - New scrollbar value representing the first visible log row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::ScrollBarValueChanged (float value)
{
	const int32 count = static_cast<int32>(fLogSnapshot.size());
	const int32 visibleRows = VisibleRowCount();

	if (count <= visibleRows || visibleRows <= 0) {
		fFirstVisibleRow = 0;
		fFollowNewest = true;

		Invalidate();
		return;
	}

	const int32 maxFirstRow = count - visibleRows;
	int32 firstRow = static_cast<int32>(value + 0.5f);

	if (firstRow < 0) {
		firstRow = 0;
	}

	if (firstRow > maxFirstRow) {
		firstRow = maxFirstRow;
	}

	fFirstVisibleRow = firstRow;

	if (fFirstVisibleRow >= maxFirstRow) {
		fFirstVisibleRow = maxFirstRow;
		fFollowNewest = true;
	} else {
		fFollowNewest = false;
	}

	Invalidate();
}


uint16
APUWriteLogView::NoiseTimerPeriodFromIndex (uint8 index) const
{
	return kNoisePeriodTable[index & 0xf];
}


double
APUWriteLogView::NoiseClockRateHzFromIndex (uint8 index) const
{
	const uint16 period = NoiseTimerPeriodFromIndex(index);

	if (period == 0) {
		return 0.0;
	}

	return kNTSCCPUClock / static_cast<double>(period);
}


// -----------------------------------------------------------------------------
// APUWriteLogView::RegisterName
//
// Converts an APU register address into a readable register name.
//
// Parameters:
//   address - CPU-visible APU register address.
//
// Returns:
//   Human-readable register name.
// -----------------------------------------------------------------------------
const char*
APUWriteLogView::RegisterName (uint16 address) const
{
	switch (address) {
		case 0x4000:
			return "SQ1 CTRL";

		case 0x4001:
			return "SQ1 SWEEP";

		case 0x4002:
			return "SQ1 TIMERL";

		case 0x4003:
			return "SQ1 TIMERH";

		case 0x4004:
			return "SQ2 CTRL";

		case 0x4005:
			return "SQ2 SWEEP";

		case 0x4006:
			return "SQ2 TIMERL";

		case 0x4007:
			return "SQ2 TIMERH";

		case 0x4008:
			return "TRI CTRL";

		case 0x400a:
			return "TRI TIMERL";

		case 0x400b:
			return "TRI TIMERH";

		case 0x400c:
			return "NOISE CTRL";

		case 0x400e:
			return "NOISE PERIOD";

		case 0x400f:
			return "NOISE LEN";

		case 0x4010:
			return "DMC CTRL";

		case 0x4011:
			return "DMC LOAD";

		case 0x4012:
			return "DMC ADDR";

		case 0x4013:
			return "DMC LEN";

		case 0x4015:
			return "STATUS";

		case 0x4017:
			return "FRAME";

		default:
			return "UNKNOWN";
	}
}


// -----------------------------------------------------------------------------
// APUWriteLogView::ChannelName
//
// Returns the APU channel associated with a CPU-visible APU register address.
//
// Parameters:
//   address - CPU-visible APU register address.
//
// Returns:
//   Human-readable APU channel or subsystem name.
// -----------------------------------------------------------------------------
const char*
APUWriteLogView::ChannelName (uint16 address) const
{
	if (address >= 0x4000 && address <= 0x4003) {
		return "Square 1";
	}

	if (address >= 0x4004 && address <= 0x4007) {
		return "Square 2";
	}

	if (address == 0x4008 ||
		address == 0x400a ||
		address == 0x400b) {
		return "Triangle";
	}

	if (address == 0x400c ||
		address == 0x400e ||
		address == 0x400f) {
		return "Noise";
	}

	if (address >= 0x4010 && address <= 0x4013) {
		return "DMC";
	}

	if (address == 0x4015 ||
		address == 0x4017) {
		return "APU";
	}

	return "Unknown";
}


// -----------------------------------------------------------------------------
// APUWriteLogView::DescribeWrite
//
// Produces a short human-readable description for one APU register write.
//
// Parameters:
//   entry - Captured APU write-log entry.
//   text  - Destination description string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::DescribeWrite (const nes::apu::apu_write_log_entry_t &entry, BString &text) const
{
	const uint16 address = entry.address;
	const uint8 value = entry.value;
	text.SetTo("");

	switch (address) {
		case 0x4000:
		case 0x4004:
		{
			const unsigned duty = static_cast<unsigned>((value >> 6) & 0x3);
			const bool lengthHalt = (value & 0x20) != 0;
			const bool constantVolume = (value & 0x10) != 0;
			const unsigned volume = static_cast<unsigned>(value & 0xf);

			const char *dutyName = "";

			switch (duty) {
				case 0:
					dutyName = "12.5%";
					break;

				case 1:
					dutyName = "25%";
					break;

				case 2:
					dutyName = "50%";
					break;

				case 3:
					dutyName = "25% neg";
					break;
			}

			text.SetToFormat("Duty: %s, Halt/Loop: %s, %s %u", dutyName, lengthHalt ? "On" : "Off",
							 constantVolume ? "Volume:" : "Envelope:", volume);
		}
		break;

		case 0x4001:
		case 0x4005:
		{
			const bool enabled = (value & 0x80) != 0;
			const unsigned period = static_cast<unsigned>((value >> 4) & 0x7);
			const bool negate = (value & 0x8) != 0;
			const unsigned shift = static_cast<unsigned>(value & 0x7);

			text.SetToFormat("Sweep: %s, Period: %u, Direction: %s, Shift: %u", enabled ? "On" : "Off", period,
							 negate ? "Down" : "Up", shift);
		}
		break;

		case 0x4002:
		case 0x4006:
		{
			const double frequency = kNTSCCPUClock /  (16.0 * (static_cast<double>(entry.timer_period) + 1.0));

			text.SetToFormat("Timer Low: $%02X, Period: %u, Frequency: %.1f Hz", value,
							 static_cast<unsigned>(entry.timer_period), frequency);
		}
		break;

		case 0x4003:
		case 0x4007:
		{
			const unsigned lengthIndex = static_cast<unsigned>((value >> 3) & 0x1f);
			const unsigned lengthValue = static_cast<unsigned>(kLengthCounterTable[lengthIndex]);
			const double frequency = kNTSCCPUClock / (16.0 * (static_cast<double>(entry.timer_period) + 1.0));

			text.SetToFormat("Timer High: %u, Length Index: %u, Length: %u, Period: %u, Frequency: %.1f Hz, Sequence Reset",
							 static_cast<unsigned>(value & 0x7), lengthIndex, lengthValue,
							 static_cast<unsigned>(entry.timer_period), frequency);
		}
		break;

		case 0x4008:
			text.SetToFormat("Length Halt: %s, Linear Reload: %u", (value & 0x80) ? "On" : "Off",
							 static_cast<unsigned>(value & 0x7f));
			break;

		case 0x400a:
		{
			const double frequency = kNTSCCPUClock / (32.0 * (static_cast<double>(entry.timer_period) + 1.0));

			text.SetToFormat("Timer Low: $%02X, Period: %u, Frequency: %.1f Hz", value,
							 static_cast<unsigned>(entry.timer_period), frequency);
		}
		break;

		case 0x400b:
		{
			const unsigned lengthIndex = static_cast<unsigned>((value >> 3) & 0x1f);
			const unsigned lengthValue = static_cast<unsigned>(kLengthCounterTable[lengthIndex]);
			const double frequency = kNTSCCPUClock / (32.0 * (static_cast<double>(entry.timer_period) + 1.0));

			text.SetToFormat("Timer High: %u, Length Index: %u, Length: %u, Period: %u, "
							 "Frequency: %.1f Hz, Linear Reload Set", static_cast<unsigned>(value & 0x7), 
							 lengthIndex, lengthValue, static_cast<unsigned>(entry.timer_period), frequency);
		}
		break;

		case 0x400c:
		{
			const bool lengthHalt = (value & 0x20) != 0;
			const bool constantVolume = (value & 0x10) != 0;
			const unsigned volume = static_cast<unsigned>(value & 0xf);

			text.SetToFormat("Halt/Loop: %s, Mode: %s, %s: %u", lengthHalt ? "On" : "Off",
							 constantVolume ? "Constant Volume" : "Envelope",
							 constantVolume ? "Volume" : "Envelope Period", volume);
		}
		break;

		case 0x400e:
		{			
			const uint8 periodIndex = static_cast<uint8>(value & 0xf);
			const uint16 timerPeriod = NoiseTimerPeriodFromIndex(periodIndex);
			const double clockHz = NoiseClockRateHzFromIndex(periodIndex);

			text.SetToFormat("Mode: %s, Index: %u, Timer: %u, Clock: %.2f Hz", (value & 0x80) ? "Short" : "Long",
							 static_cast<unsigned>(periodIndex), static_cast<unsigned>(timerPeriod), clockHz);
		} 
		break;

		case 0x400f:
		{
			const unsigned lengthIndex = static_cast<unsigned>((value >> 3) & 0x1f);
			const unsigned lengthValue = static_cast<unsigned>(kLengthCounterTable[lengthIndex]);

			text.SetToFormat("Length Index: %u, Length: %u, Length Reload, Envelope Restart", 
							 lengthIndex, lengthValue);
		}
		break;

		case 0x4010:
		{
			static const uint16 dmcPeriods[16] = {
				428, 380, 340, 320,
				286, 254, 226, 214,
				190, 160, 142, 128,
				106, 84, 72, 54
			};

			const unsigned rateIndex = static_cast<unsigned>(value & 0xf);
			const unsigned period = static_cast<unsigned>(dmcPeriods[rateIndex]);
			const double bitRate = kNTSCCPUClock / static_cast<double>(period);

			text.SetToFormat("IRQ: %s, Loop: %s, Rate: %u ($%X), %u cycles, %.1f Hz",
							 (value & 0x80) ? "On" : "Off",
							 (value & 0x40) ? "On" : "Off",
							 rateIndex, rateIndex, period, bitRate);
		}
		break;

		case 0x4011:
			text.SetToFormat("DMC Output Level: %u", static_cast<unsigned>(value & 0x7f));
			break;

		case 0x4012:
			text.SetToFormat("Sample Address: $%04X",
							 static_cast<unsigned>(0xc000 | (static_cast<uint16>(value) << 6)));
			break;

		case 0x4013:
			text.SetToFormat("Sample Length: %u bytes",
							 static_cast<unsigned>((static_cast<uint16>(value) << 4) | 1));
			break;

		case 0x4015:
		{
			const uint8 enableMask = static_cast<uint8>(value & 0x1f);
			const bool square1Enabled = (enableMask & 0x01) != 0;
			const bool square2Enabled = (enableMask & 0x02) != 0;
			const bool triangleEnabled = (enableMask & 0x04) != 0;
			const bool noiseEnabled = (enableMask & 0x08) != 0;
			const bool dmcEnabled = (enableMask & 0x10) != 0;

			BString changes;

			if (entry.has_previous_enable_mask) {
				const uint8 changed = static_cast<uint8>(entry.previous_enable_mask ^ enableMask);

				if (changed & 0x1) {
					if (changes.Length() != 0) {
						changes << ", ";
					}

					changes << "SQ1: " << (square1Enabled ? "On" : "Off");
				}

				if (changed & 0x2) {
					if (changes.Length() != 0) {
						changes << ", ";
					}

					changes << "SQ2: " << (square2Enabled ? "On" : "Off");
				}

				if (changed & 0x4) {
					if (changes.Length() != 0) {
						changes << ", ";
					}

					changes << "TRI: " << (triangleEnabled ? "On" : "Off");
				}

				if (changed & 0x8) {
					if (changes.Length() != 0) {
						changes << ", ";
					}

					changes << "NOI: " << (noiseEnabled ? "On" : "Off");
				}

				if (changed & 0x10) {
					if (changes.Length() != 0) {
						changes << ", ";
					}

					changes << "DMC: " << (dmcEnabled ? "On" : "Off");
				}
			}

			if (changes.Length() == 0) {
				changes.SetTo("None");
			}

			text.SetToFormat(
				"Enable Mask: $%02X, SQ1: %s SQ2: %s TRI: %s NOI: %s DMC: %s, Changed: %s",
				static_cast<unsigned>(enableMask),
				square1Enabled ? "On" : "Off",
				square2Enabled ? "On" : "Off",
				triangleEnabled ? "On" : "Off",
				noiseEnabled ? "On" : "Off",
				dmcEnabled ? "On" : "Off",
				changes.String());
		}
		break;

		case 0x4017:
		{
			const bool fiveStep = (value & 0x80) != 0;
			const bool irqInhibit = (value & 0x40) != 0;

			if (fiveStep) {
				if (irqInhibit) {
					text.SetTo(
						"Frame: 5-Step, IRQ Inhibit: Yes, Frame IRQ Cleared, Immediate Quarter/Half Clock");
				} else {
					text.SetTo(
						"Frame: 5-Step, IRQ Inhibit: No, Immediate Quarter/Half Clock");
				}
			} else {
				if (irqInhibit) {
					text.SetTo("Frame: 4-Step, IRQ Inhibit: Yes, Frame IRQ Cleared");
				} else {
					text.SetTo("Frame: 4-Step, IRQ Inhibit: No, Frame IRQ Allowed");
				}
			}
		}
		break;
		
		default:
			text.SetTo("Unknown APU Write");
			break;
	}
}


// -----------------------------------------------------------------------------
// APUWriteLogView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM/mapper is currently loaded.
// -----------------------------------------------------------------------------
bool
APUWriteLogView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// APUWriteLogView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the APU Write Log window is opened
// without a loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUWriteLogView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font(prevFont);
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect APU writes.";

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

