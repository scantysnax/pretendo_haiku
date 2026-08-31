
#include "PPUWriteLogView.h"


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
PPUWriteLogView::PPUWriteLogView(BRect frame, PretendoWindow *parent)
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

	if (HasROMLoaded()) {
		CaptureLogSnapshot();
	}
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::Pulse
//
// Refreshes the debugger-side PPU write-log snapshot while the view is live.
//
// If no ROM is loaded, any stale snapshot is discarded.  Frozen mode preserves
// the existing snapshot and does not sample the emulator write log.
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
			Invalidate();
		}

		return;
	}

	if (fFreezeUpdates) {
		return;
	}

	CaptureLogSnapshot();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::CaptureLogSnapshot
//
// Captures the current PPU write log into debugger-owned storage.
//
// The PPU core performs the ring-buffer traversal using one captured logical
// starting position, preventing the debugger from seeing a different ring
// origin for every entry while the emulator is running.
//
// The resulting snapshot is stored oldest to newest. DrawLogPanel() reverses
// that order for display so the newest write appears at the top.
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
	fLogSnapshot.clear();

	if (!HasROMLoaded()) {
		return;
	}

	const uint32 count = nes::ppu::ppu_write_log_count();

	if (count == 0) {
		return;
	}

	fLogSnapshot.resize(count);

	const uint32 captured = nes::ppu::ppu_write_log_snapshot(fLogSnapshot.data(),
							static_cast<uint32>(fLogSnapshot.size()));

	fLogSnapshot.resize(captured);
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::KeyDown
//
// Handles keyboard controls for the PPU write-log debugger.
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
				/*
				 * Capture the exact log state that will become frozen.
				 */
				CaptureLogSnapshot();
				fFreezeUpdates = true;
			} else {
				fFreezeUpdates = false;

				/*
				 * Refresh immediately when returning to live mode so
				 * the display does not retain the old frozen image until
				 * the next pulse.
				 */
				CaptureLogSnapshot();
			}

			Invalidate();
			break;
		}

		case 'c':
		case 'C':
		{
			nes::ppu::clear_ppu_write_log();

			fLogSnapshot.clear();

			Invalidate();
			break;
		}

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
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
// Draws the title/help panel for the PPU write log viewer.
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
	line.SetToFormat("Space: %s   C: clear log   State: %s",
					(fFreezeUpdates ? "resume" : "freeze"),
					(fFreezeUpdates ? "frozen" : "live"));

	if (fFreezeUpdates) {
		SetHighColor(160, 80, 0);
	} else {
		SetHighColor(35, 35, 35);
	}

	DrawString(line.String(), BPoint(panel.left + 8.0f, panel.top + 42.0f));
}


// -----------------------------------------------------------------------------
// PPUWriteLogView::DrawLogPanel
//
// Draws the debugger-side PPU write-log snapshot.
//
// The newest entries that fit in the panel are selected while preserving
// chronological order from top to bottom.  Older writes therefore appear above
// newer writes, with the most recent captured write at the bottom.
//
// Drawing reads only from fLogSnapshot.  Live log sampling and freeze behavior
// are handled separately by CaptureLogSnapshot() and the view update paths.
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
	BRect panel(4.0f, 70.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
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
	const float dotX = frameX + 58.0f;
	const float scanX = dotX + 42.0f;
	const float regX = scanX + 46.0f;
	const float valX = regX + 74.0f;
	const float descX = valX + 42.0f;
	float y = panel.top + 48.0f;

	SetHighColor(80, 80, 80);
	DrawString("Frame", BPoint(frameX, y));
	DrawString("Dot", BPoint(dotX, y));
	DrawString("Scan", BPoint(scanX, y));
	DrawString("Reg", BPoint(regX, y));
	DrawString("Val", BPoint(valX, y));
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

	const uint32 maxRows = static_cast<uint32>((panel.bottom - y - 8.0f) / lineH);
	uint32 rows = count;

	if (rows > maxRows) {
		rows = maxRows;
	}

	/*
	 * fLogSnapshot is stored oldest -> newest.
	 *
	 * If the entire snapshot does not fit, begin far enough into the
	 * snapshot that the newest entries remain visible.  Entries are then
	 * drawn forward so time progresses downward through the panel.
	 */
	const uint32 firstIndex = count - rows;
	
	BString s;
	BString desc;

	for (uint32 row = 0; row < rows; row++) {
		const uint32 index = firstIndex + row;
		const nes::ppu::ppu_write_log_entry_t &entry = fLogSnapshot[index];

		DescribeWrite(entry, desc);

		if (entry.address == 0x2005 || entry.address == 0x2006) {
			SetHighColor(0, 90, 150);
		} else if (entry.address == 0x2007) {
			SetHighColor(120, 70, 0);
		} else if (entry.address == 0x4014) {
			SetHighColor(120, 0, 120);
		} else {
			SetHighColor(0, 0, 0);
		}

		s.SetToFormat("%llu", static_cast<unsigned long long>(entry.frame));
		DrawString(s.String(), BPoint(frameX, y));

		s.SetToFormat("%u", static_cast<unsigned>(entry.dot));
		DrawString(s.String(), BPoint(dotX, y));

		s.SetToFormat("%u", static_cast<unsigned>(entry.scanline));
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
PPUWriteLogView::DescribeWrite (const nes::ppu::ppu_write_log_entry_t &entry, BString &text) const
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

			text.SetToFormat("NT $%04X, BG $%04X, SPR %s, inc +%u, NMI %s", ntBase, bgPT,
							 sprite8x16 ? "8x16" : "8x8", static_cast<unsigned>(increment),
							 nmi ? "on": "off");
		}
		break;

		case 0x2001:
		{
			const bool bg = (value & 0x8) != 0;
			const bool sprites = (value & 0x10) != 0;

			const bool grayscale = (value & 0x1) != 0;
			text.SetToFormat("BG %s, SPR %s, gray %s", bg ? "on" : "off", sprites ? "on" : "off",
							 grayscale ? "on" : "off");
		}
		break;

		case 0x2002:
			text.SetTo("write to read-only status");
			break;

		case 0x2003:
			text.SetToFormat("OAM address $%02X", value);
			break;

		case 0x2004:
			text.SetToFormat("OAM data $%02X", value);
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
				text.SetToFormat("1st/high address $%02X", value & 0x3f);
			} else {
				text.SetToFormat("2nd/low address $%02X", value);
			}
		}
		break;

		case 0x2007:
			text.SetToFormat("VRAM data $%02X", value);
			break;

		case 0x4014:
			text.SetToFormat("DMA from CPU page $%02X00", value);
			break;

		default:
			text.SetTo("unknown write");
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

