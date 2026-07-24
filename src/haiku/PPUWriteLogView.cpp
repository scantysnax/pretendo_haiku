
#include "PPUWriteLogView.h"

#include "Cart.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"

#include "Ppu.h"

#include <cmath>


PPUWriteLogView::PPUWriteLogView (BRect frame, PretendoWindow *parent)
	: BView (frame, "ppu_write_log_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


PPUWriteLogView::~PPUWriteLogView()
{
}


void
PPUWriteLogView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);
}


void
PPUWriteLogView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	if (!fFreezeUpdates) {
		Invalidate();
	}
}


void
PPUWriteLogView::KeyDown(const char *bytes, int32 numBytes)
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
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			break;

		case 'c':
		case 'C':
			nes::ppu::clear_ppu_write_log();
			Invalidate();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


void
PPUWriteLogView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(
			4.0f,
			70.0f,
			Bounds().right - 4.0f,
			Bounds().bottom - 8.0f
		);

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
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		58.0f
	);

	::DrawDebugPanel(this, panel, "PPU Write Log");

	SetFontSize(11.0f);

	BString line;
	line.SetToFormat(
		"Space: %s   C: clear log   State: %s",
		fFreezeUpdates ? "resume" : "freeze",
		fFreezeUpdates ? "frozen" : "live"
	);

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
// Draws the rolling PPU write log.  Entries are displayed from newest to oldest
// so the most recent PPU writes are visible at the top.
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
	BRect panel(
		4.0f,
		70.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);
	
	::DrawDebugPanel(this, panel, "Recent Writes");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	SetFont(&mono);

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
	StrokeLine(
		BPoint(panel.left + 8.0f, y - 8.0f),
		BPoint(panel.right - 8.0f, y - 8.0f)
	);

	y += 6.0f;
	
	uint32 count = nes::ppu::ppu_write_log_count();

	if (count == 0) {
		SetHighColor(90, 90, 90);
		DrawString("No PPU writes logged yet.", BPoint(frameX, y + lineH));
		SetFont(&prevFont);
		return;
	}

	const uint32 maxRows = static_cast<uint32>((panel.bottom - y - 8.0f) / lineH);

	BString s;
	BString desc;

	uint32 rows = count;

	if (rows > maxRows) {
		rows = maxRows;
	}

	for (uint32 row = 0; row < rows; row++) {
		uint32 index = count - 1 - row;
		nes::ppu::ppu_write_log_entry_t entry =
			nes::ppu::ppu_write_log_entry(index);

		DescribeWrite(entry.address, entry.value, desc);

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
// Produces a short human-readable description for a PPU register write.
//
// Parameters:
//   address - CPU-visible PPU register address.
//   value   - Value written.
//   text    - Destination string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUWriteLogView::DescribeWrite (uint16 address, uint8 value, BString &text) const
{
	text.SetTo("");

	switch (address) {
		case 0x2000:
		{
			const uint16 ntBase = 0x2000 + ((value & 0x3) * 0x400);
			const uint16 bgPT = (value & 0x10) ? 0x1000 : 0x0000;
			const bool sprite8x16 = (value & 0x20) != 0;
			const bool nmi = (value & 0x80) != 0;

			text.SetToFormat(
				"NT $%04X, BG $%04X, SPR %s, NMI %s",
				ntBase,
				bgPT,
				sprite8x16 ? "8x16" : "8x8",
				nmi ? "on" : "off"
			);
		} break;

		case 0x2001:
		{
			const bool bg = (value & 0x8) != 0;
			const bool sprites = (value & 0x10) != 0;
			const bool mono = (value & 0x1) != 0;

			text.SetToFormat(
				"BG %s, SPR %s, mono %s",
				bg ? "on" : "off",
				sprites ? "on" : "off",
				mono ? "on" : "off"
			);
		} break;

		case 0x2002:
			text.SetTo("write to read-only status");
			break;

		case 0x2003:
			text.SetToFormat("OAM address $%02X", value);
			break;

		case 0x2004:
			text.SetTo("OAM data byte");
			break;

		case 0x2005:
			text.SetTo("scroll write");
			break;

		case 0x2006:
			text.SetTo("VRAM address write");
			break;

		case 0x2007:
			text.SetTo("VRAM data write");
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

	const char* title = "No ROM loaded";
	const char* detail = "Load a cartridge to inspect PPU writes.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80, 255);
	DrawString(
		title,
		BPoint(
			centerX - (StringWidth(title) * 0.5f),
			centerY - 8.0f
		)
	);

	SetHighColor(120, 120, 120, 255);
	DrawString(
		detail,
		BPoint(
			centerX - (StringWidth(detail) * 0.5f),
			centerY + fh.ascent + 8.0f
		)
	);

	SetFont(&prevFont);
}

