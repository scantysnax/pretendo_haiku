
#include "PPUWriteLogView.h"


// -----------------------------------------------------------------------------
// PPUWriteLogScrollBar
//
// Vertical scrollbar used by PPUWriteLogView.  Instead of scrolling the target
// BView directly, changes are forwarded into the view's retained-history scroll
// state so the fixed header remains stationary.
// -----------------------------------------------------------------------------
class PPUWriteLogScrollBar : public BScrollBar
{
	public:
	PPUWriteLogScrollBar (BRect frame, PPUWriteLogView *owner);

	protected:
	virtual void ValueChanged(float newValue);

	private:
	PPUWriteLogView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// PPUWriteLogScrollBar::PPUWriteLogScrollBar
//
// Creates the vertical scrollbar used by the PPU Write Log.
//
// Parameters:
//   frame - Rectangle defining the scrollbar bounds.
//   owner - PPUWriteLogView that owns the scrolling state.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PPUWriteLogScrollBar::PPUWriteLogScrollBar(BRect frame, PPUWriteLogView *owner)
	: BScrollBar(frame, "ppu_write_log_scroll_bar", nullptr, 0.0f, 0.0f, B_VERTICAL)
{
	fOwner = owner;
}


// -----------------------------------------------------------------------------
// PPUWriteLogScrollBar::ValueChanged
//
// Forwards scrollbar movement to the owning PPU Write Log view.
//
// Parameters:
//   newValue - New scrollbar position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogScrollBar::ValueChanged(float newValue)
{
	BScrollBar::ValueChanged(newValue);

	if (fOwner) {
		fOwner->ScrollBarValueChanged(newValue);
	}
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::PPUWriteLogView
//
// Constructs the PPU Write Log debugger view.
//
// The view displays a live rolling history of CPU writes to PPU-facing
// registers.  Keyboard focus, live/frozen snapshot handling, and log drawing are
// managed by the view after it is attached to a window.
//
// The parent argument is retained for consistency with the other debugger-view
// constructors but is not currently required by this view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow; currently unused.
//
// Returns:
//   Constructed PPUWriteLogView instance.
// -----------------------------------------------------------------------------
PPUWriteLogView::PPUWriteLogView (BRect frame, PretendoWindow *parent)
	: BView(frame, "ppu_write_log_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	(void)parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::~PPUWriteLogView
//
// Destroys the PPU Write Log debugger view.
//
// No additional cleanup is currently required because debugger snapshot storage
// is managed automatically and the view does not own external PPU resources.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PPUWriteLogView::~PPUWriteLogView()
{
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::AttachedToWindow
//
// Completes PPU Write Log view setup after attachment to a window.
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
PPUWriteLogView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	const float scrollBarWidth = B_V_SCROLL_BAR_WIDTH;
	BRect scrollFrame(Bounds().right - scrollBarWidth - 4.0f, 70.0f, 
					  Bounds().right - 4.0f, Bounds().bottom - 8.0f);

	fScrollBar = new PPUWriteLogScrollBar(scrollFrame, this);
	AddChild(fScrollBar);

	if (HasROMLoaded()) {
		CaptureLogSnapshot();
	}

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::Pulse
//
// Periodically refreshes the PPU write-log snapshot while the view is live and
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
PPUWriteLogView::Pulse()
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
// PPUWriteLogView::CaptureLogSnapshot
//
// Captures the current rolling PPU write log into debugger-owned storage and
// updates the scrollbar to reflect the new snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::CaptureLogSnapshot()
{
	const uint32 count = nes::ppu::write_log_count();
	fLogSnapshot.resize(count);

	if (count != 0) {
		const uint32 copied = nes::ppu::write_log_snapshot(fLogSnapshot.data(), count);
		fLogSnapshot.resize(copied);
	}

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::KeyDown
//
// Handles keyboard controls for the PPU write-log debugger.
//
// Space toggles between live and frozen display modes.  Entering frozen mode
// captures the current write-log snapshot immediately so the displayed contents
// represent the state at the moment freezing occurs.  Returning to live mode
// refreshes the snapshot immediately.
//
// C clears both the emulator-side PPU write log and the debugger-side snapshot.
//
// Parameters:
//   bytes    - Keyboard input bytes.
//   numBytes - Number of bytes supplied.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::KeyDown (const char *bytes, int32 numBytes)
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
			nes::ppu::clear_write_log();

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
// PPUWriteLogView::MessageReceived
//
// Handles messages sent to the PPU Write Log view.  Mouse-wheel messages are
// translated into row scrolling while all other messages are passed to BView.
//
// Parameters:
//   message - Message received by the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::MessageReceived(BMessage *message)
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
// PPUWriteLogView::Draw
//
// Draws the complete PPU Write Log debugger.
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
PPUWriteLogView::Draw (BRect updateRect)
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
// PPUWriteLogView::DrawHeaderPanel
//
// Draws the title/help panel for the PPU write log viewer.  The panel shows the
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
PPUWriteLogView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 58.0f);
	::DrawDebugPanel(this, panel, "PPU Write Log");

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
// PPUWriteLogView::DrawLogPanel
//
// Draws the current PPU write-log snapshot.  The panel displays the retained
// writes in chronological order and respects the current history scroll
// position while preserving automatic follow-newest behavior when enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::DrawLogPanel()
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

	const float frameX = panel.left + 8.0f;
	const float dotX = frameX + 70.0f;
	const float scanX = dotX + 50.0f;
	const float regX = scanX + 58.0f;
	const float valX = regX + 96.0f;
	const float descX = valX + 52.0f;

	float y = panel.top + 48.0f;

	SetHighColor(80, 80, 80);

	DrawString("Frame", BPoint(frameX, y));
	DrawString("Dot", BPoint(dotX, y));
	DrawString("Scanline", BPoint(scanX, y));
	DrawString("Register", BPoint(regX, y));
	DrawString("Value", BPoint(valX, y));
	DrawString("Meaning", BPoint(descX, y));

	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));
	y += 6.0f;

	const uint32 count = static_cast<uint32>(fLogSnapshot.size());

	if (count == 0) {
		SetHighColor(90, 90, 90);
		DrawString("No PPU writes logged yet.", BPoint(frameX, y + lineH));
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
							  ? static_cast<int32>(count) - visibleRows : 0;

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
		const nes::ppu::write_log_entry_t &entry = fLogSnapshot[index];

		DescribeWrite(entry, desc);

		switch (entry.address) {
			case 0x2000:
			case 0x2001:
				SetHighColor(0, 70, 150);
				break;

			case 0x2003:
			case 0x2004:
			case 0x4014:
				SetHighColor(110, 0, 120);
				break;

			case 0x2005:
			case 0x2006:
				SetHighColor(0, 110, 70);
				break;

			case 0x2007:
				SetHighColor(120, 70, 0);
				break;

			default:
				SetHighColor(0, 0, 0);
				break;
		}

		s.SetToFormat("%llu", static_cast<unsigned long long>(entry.frame));
		DrawString(s.String(), BPoint(frameX, y));

		s.SetToFormat("%u", static_cast<unsigned int>(entry.dot));
		DrawString(s.String(), BPoint(dotX, y));

		s.SetToFormat("%u", static_cast<unsigned int>(entry.scanline));
		DrawString(s.String(), BPoint(scanX, y));
		DrawString(RegisterName(entry.address), BPoint(regX, y));

		s.SetToFormat("$%02X", entry.value);
		DrawString(s.String(), BPoint(valX, y));
		DrawString(desc.String(), BPoint(descX, y));
		y += lineH;
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::VisibleRowCount
//
// Calculates how many PPU write-log rows fit inside the Recent Writes panel.
//
// Parameters:
//   None.
//
// Returns:
//   Number of visible log rows that fit in the panel.
// -----------------------------------------------------------------------------
int32
PPUWriteLogView::VisibleRowCount() const
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
// PPUWriteLogView::ScrollRows
//
// Scrolls the visible PPU write-log history by the requested number of rows.
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
PPUWriteLogView::ScrollRows (int32 rows)
{
	const int32 count = static_cast<int32>(fLogSnapshot.size());
	const int32 visibleRows = VisibleRowCount();

	if (count <= visibleRows || visibleRows <= 0) {
		fFirstVisibleRow = 0;
		fFollowNewest = true;

		UpdateScrollBar();
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
// PPUWriteLogView::ScrollPages
//
// Scrolls the visible PPU write-log history by whole visible pages.
//
// Parameters:
//   pages - Signed number of pages to move.  Negative values move toward older
//           entries; positive values move toward newer entries.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::ScrollPages (int32 pages)
{
	const int32 visibleRows = VisibleRowCount();

	if (visibleRows <= 0) {
		return;
	}

	ScrollRows(pages * visibleRows);
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::FollowNewest
//
// Returns the PPU write-log view to the newest available entries and enables
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
PPUWriteLogView::FollowNewest()
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
// PPUWriteLogView::UpdateScrollBar
//
// Updates the PPU write-log scrollbar range, step sizes, thumb proportion, and
// current value to match the retained log snapshot and visible row count.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::UpdateScrollBar()
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
// PPUWriteLogView::ScrollBarValueChanged
//
// Updates the visible PPU write-log position when the scrollbar is moved.
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
PPUWriteLogView::ScrollBarValueChanged (float value)
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


// -----------------------------------------------------------------------------
// PPUWriteLogView::RegisterName
//
// Converts a PPU-facing CPU register address into a readable register name.
//
// Parameters:
//   address - CPU-visible PPU register address.
//
// Returns:
//   Human-readable register name.
// -----------------------------------------------------------------------------
const char*
PPUWriteLogView::RegisterName (uint16 address) const
{
	switch (address) {
		case 0x2000:
			return "PPUCTRL";

		case 0x2001:
			return "PPUMASK";

		case 0x2002:
			return "PPUSTATUS";

		case 0x2003:
			return "OAMADDR";

		case 0x2004:
			return "OAMDATA";

		case 0x2005:
			return "PPUSCROLL";

		case 0x2006:
			return "PPUADDR";

		case 0x2007:
			return "PPUDATA";

		case 0x4014:
			return "OAMDMA";

		default:
			return "UNKNOWN";
	}
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::DescribeWrite
//
// Produces a short human-readable description for one PPU-facing register
// write.
//
// PPUCTRL and PPUMASK values are decoded into useful rendering state.
// PPUSCROLL and PPUADDR use the captured shared write-latch state so first and
// second writes can be identified accurately rather than inferred from nearby
// log entries.
//
// For PPUSCROLL:
//   first write  - horizontal scroll: coarse X and fine X
//   second write - vertical scroll: coarse Y and fine Y
//
// For PPUADDR:
//   first write  - high address byte
//   second write - low address byte
//
// Parameters:
//   entry - Captured PPU write-log entry.
//   text  - Destination description string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::DescribeWrite (const nes::ppu::write_log_entry_t &entry, BString &text) const
{
	const uint16 address = entry.address;
	const uint8 value = entry.value;

	text.SetTo("");

	switch (address) {
		case 0x2000:
		{
			const uint16 ntBase = 0x2000 + ((value & 0x3) * 0x400);
			const uint16 bgPT = (value & 0x10) ? 0x1000 : 0x0000;
			const bool sprite8x16 = (value & 0x20) != 0;
			const bool nmi = (value & 0x80) != 0;
			const uint16 increment = (value & 0x4) ? 32 : 1;

			text.SetToFormat("NT: $%04X, BG: $%04X, SPR: %s, Inc: +%u, NMI: %s", ntBase, bgPT,
							 sprite8x16 ? "8x16" : "8x8", static_cast<unsigned>(increment),
							 nmi ? "On": "Off");
		}
		break;

		case 0x2001:
		{
			const bool bg = (value & 0x8) != 0;
			const bool sprites = (value & 0x10) != 0;

			const bool grayscale = (value & 0x1) != 0;
			text.SetToFormat("BG: %s, SPR: %s, Gray: %s", bg ? "On" : "Off", sprites ? "On" : "Off",
							 grayscale ? "On" : "Off");
		}
		break;

		case 0x2002:
			text.SetTo("Write to read-only status");
			break;

		case 0x2003:
			text.SetToFormat("OAM Address: $%02X", value);
			break;

		case 0x2004:
			text.SetToFormat("OAM Data: $%02X", value);
			break;

		case 0x2005:
		{
			const bool secondWrite = entry.write_latch != 0;
			const uint8 coarse = (value >> 3) & 0x1f;
			const uint8 fine = value & 0x7;

			if (!secondWrite) {
				text.SetToFormat("1st/X: coarse %u, fine %u", 
								 static_cast<unsigned>(coarse),
								 static_cast<unsigned>(fine));
			} else {
				text.SetToFormat("2nd/Y: coarse %u, fine %u",
								 static_cast<unsigned>(coarse),
								 static_cast<unsigned>(fine));
			}
		}
		break;

		case 0x2006:
		{
			const bool secondWrite = entry.write_latch != 0;

			if (!secondWrite) {
				/*
				 * Only six bits of the first PPUADDR write contribute
				 * to the 14-bit PPU address.
				 */
				text.SetToFormat("1st/High Address: $%02X", value & 0x3f);
			} else {
				text.SetToFormat("2nd/Low Address: $%02X", value);
			}
		}
		break;

		case 0x2007:
			text.SetToFormat("VRAM Data: $%02X", value);
			break;

		case 0x4014:
			text.SetToFormat("DMA From CPU Page: $%02X00", value);
			break;

		default:
			text.SetTo("Unknown Write");
			break;
	}
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::HasROMLoaded
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
PPUWriteLogView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the PPU Write Log window is opened
// without a loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect PPU writes.";

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

