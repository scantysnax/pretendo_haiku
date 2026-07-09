
#include "PPUMemoryView.h"

#include "DebugHelpers.h"
#include "PretendoWindow.h"

#include "Ppu.h"


class PPUMemoryScrollBar : public BScrollBar
{
	public:
	PPUMemoryScrollBar (BRect frame, PPUMemoryView *owner)
		:
		BScrollBar(
			frame,
			"ppu_memory_scrollbar",
			owner,
			0.0f,
			1023.0f,
			B_VERTICAL
		)
	{
		fOwner = owner;

		SetSteps(1.0f, 16.0f);
	}

	virtual void ValueChanged (float value)
	{
		if (fOwner) {
			fOwner->ScrollBarChanged(value);
		}
	}

	private:
	PPUMemoryView *fOwner = nullptr;
};


PPUMemoryView::PPUMemoryView (BRect frame, PretendoWindow *parent)
	: BView(frame, "ppu_memory_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


PPUMemoryView::~PPUMemoryView()
{
}


void
PPUMemoryView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (!fScrollBar) {
		BRect scrollFrame(
			Bounds().right - B_V_SCROLL_BAR_WIDTH,
			88.0f,
			Bounds().right,
			Bounds().bottom - 8.0f
		);

		fScrollBar = new PPUMemoryScrollBar(scrollFrame, this);
		AddChild(fScrollBar);
	}

	LayoutScrollBar();
	UpdateScrollBar();

	MakeFocus(true);
}


void
PPUMemoryView::Pulse()
{
	if (!fFreezeUpdates) {
		Invalidate();
	}
}


void
PPUMemoryView::KeyDown (const char* bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			break;

		case '1':
			SetBaseAddress(0x0000);
			break;

		case '2':
			SetBaseAddress(0x1000);
			break;

		case 'n':
		case 'N':
			SetBaseAddress(0x2000);
			break;

		case 'm':
		case 'M':
			SetBaseAddress(0x2400);
			break;

		case ',':
			SetBaseAddress(0x2800);
			break;

		case '.':
			SetBaseAddress(0x2c00);
			break;

		case 'p':
		case 'P':
			SetBaseAddress(0x3f00);
			break;

		case B_UP_ARROW:
			ScrollRows(-1);
			break;

		case B_DOWN_ARROW:
			ScrollRows(1);
			break;

		case B_PAGE_UP:
			ScrollRows(-16);
			break;

		case B_PAGE_DOWN:
			ScrollRows(16);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


void
PPUMemoryView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();
	DrawMemoryPanel();
}


void
PPUMemoryView::FrameResized(float width, float height)
{
	(void)width;
	(void)height;

	LayoutScrollBar();

	BView::FrameResized(width, height);
}

// -----------------------------------------------------------------------------
// PPUMemoryView::LayoutScrollBar
//
// Positions the vertical memory scrollbar along the right edge of the memory
// panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::LayoutScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const float top = 88.0f;
	const float bottom = Bounds().bottom - 8.0f;
	const float width = B_V_SCROLL_BAR_WIDTH;

	fScrollBar->MoveTo(Bounds().right - width, top);
	fScrollBar->ResizeTo(width, bottom - top);

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::UpdateScrollBar
//
// Synchronizes the scrollbar value with the current base PPU memory address.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	fUpdatingScrollBar = true;

	const float row = static_cast<float>((fBaseAddress & 0x3ff0) >> 4);
	fScrollBar->SetValue(row);

	fUpdatingScrollBar = false;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ScrollBarChanged
//
// Handles vertical scrollbar movement.  Each scrollbar unit maps to one 16-byte
// PPU memory row.
//
// Parameters:
//   value - Scrollbar row index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::ScrollBarChanged(float value)
{
	if (fUpdatingScrollBar) {
		return;
	}

	int32 row = static_cast<int32>(value + 0.5f);

	if (row < 0) {
		row = 0;
	}

	if (row > 1023) {
		row = 1023;
	}
	
	fBaseAddress = static_cast<uint16>((row << 4) & 0x3ff0);

	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawHeaderPanel
//
// Draws the title/help panel for the PPU memory viewer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawHeaderPanel()
{
	const float rightEdge = fScrollBar
		? fScrollBar->Frame().left - 4.0f
		: Bounds().right - 4.0f;

	BRect panel(
		4.0f,
		4.0f,
		rightEdge,
		76.0f
	);

	::DrawDebugPanel(this, panel, "PPU Memory");

	SetFontSize(11.0f);

	BString line;
	line.SetToFormat(
		"Base: $%04X  %s   Space: %s",
		fBaseAddress,
		RegionName(fBaseAddress),
		fFreezeUpdates ? "resume live" : "freeze"
	);

	SetHighColor(35, 35, 35);
	DrawString(line.String(), BPoint(panel.left + 8.0f, panel.top + 42.0f));

	DrawString(
		"1/2 Pattern   N/M/,/. NTs   P Palette   Up/Down row   PgUp/PgDn page",
		BPoint(panel.left + 8.0f, panel.top + 60.0f)
	);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawMemoryPanel
//
// Draws a 16-byte-per-row hex view of raw PPU memory starting at fBaseAddress.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawMemoryPanel()
{
	const float rightEdge = fScrollBar
		? fScrollBar->Frame().left - 4.0f
		: Bounds().right - 4.0f;

	BRect panel(
		4.0f,
		88.0f,
		rightEdge,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Raw PPU Bytes");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float addrX = panel.left + 8.0f;
	const float byteX = addrX + 72.0f;
	const float asciiX = byteX + 312.0f;

	float y = panel.top + 42.0f;

	SetHighColor(80, 80, 80);
	DrawString("Address", BPoint(addrX, y));
	DrawString("00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F",
		BPoint(byteX, y));
	DrawString("Text", BPoint(asciiX, y));

	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(
		BPoint(panel.left + 8.0f, y - 8.0f),
		BPoint(panel.right - 8.0f, y - 8.0f)
	);

	y += 4.0f;

	const uint32 rows = static_cast<uint32>(
		(panel.bottom - y - 8.0f) / lineH
	);

	BString s;
	BString bytes;
	BString ascii;

	for (uint32 row = 0; row < rows; row++) {
		uint16 address = (fBaseAddress + row * 16) & 0x3fff;

		bytes.SetTo("");
		ascii.SetTo("");

		for (uint32 col = 0; col < 16; col++) {
			uint16 byteAddress = (address + col) & 0x3fff;
			uint8 value = nes::ppu::debug_read_ppu_memory(byteAddress);

			s.SetToFormat("%02X", value);
			bytes.Append(s);

			if (col == 7) {
				bytes.Append("  ");
			} else if (col != 15) {
				bytes.Append(" ");
			}

			if (value >= 32 && value <= 126) {
				ascii.Append(static_cast<char>(value), 1);
			} else {
				ascii.Append(".");
			}
		}

		if (address >= 0x3f00) {
			SetHighColor(120, 0, 120);
		} else if (address >= 0x2000) {
			SetHighColor(0, 90, 150);
		} else {
			SetHighColor(0, 0, 0);
		}

		s.SetToFormat("$%04X", address);
		DrawString(s.String(), BPoint(addrX, y));
		DrawString(bytes.String(), BPoint(byteX, y));

		SetHighColor(80, 80, 80);
		DrawString(ascii.String(), BPoint(asciiX, y));

		y += lineH;
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::SetBaseAddress
//
// Sets the starting PPU address displayed by the memory viewer.
//
// Parameters:
//   address - New base PPU address.  The value is clamped to the PPU address
//             range and aligned to a 16-byte row boundary.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::SetBaseAddress(uint16 address)
{
	fBaseAddress = address & 0x3ff0;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ScrollRows
//
// Scrolls the memory viewer by a signed number of 16-byte rows.
//
// Parameters:
//   rows - Number of rows to scroll.  Negative values scroll upward; positive
//          values scroll downward.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::ScrollRows(int32 rows)
{
	int32 address = static_cast<int32>(fBaseAddress);
	address += rows * 16;

	address &= 0x3ff0;

	fBaseAddress = static_cast<uint16>(address);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::RegionName
//
// Returns a short human-readable label for a PPU address region.
//
// Parameters:
//   address - PPU address.
//
// Returns:
//   Region label.
// -----------------------------------------------------------------------------
const char*
PPUMemoryView::RegionName (uint16 address) const
{
	address &= 0x3fff;

	if (address < 0x1000) {
		return "Pattern Table 0";
	}

	if (address < 0x2000) {
		return "Pattern Table 1";
	}
	
	if (address < 0x2400) {
		return "Nametable 0";
	}

	if (address < 0x2800) {
		return "Nametable 1";
	}

	if (address < 0x2c00) {
		return "Nametable 2";
	}

	if (address < 0x3000) {
		return "Nametable 3";
	}

	if (address < 0x3f00) {
		return "Nametable Mirrors";
	}

	return "Palette RAM";
}

