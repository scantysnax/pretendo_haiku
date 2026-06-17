
#include "PaletteDebugView.h"
#include "PretendoWindow.h"
#include "DebugHelpers.h"

#include "Cart.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"

#include <cstdio>
#include <cstring>


// -----------------------------------------------------------------------------
// PaletteDebugView::PaletteDebugView
//
// Creates the palette debugger view and stores the parent PretendoWindow used
// for access to the host palette mapping.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow, used to retrieve the host color palette.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PaletteDebugView::PaletteDebugView(BRect frame, PretendoWindow* parent)
	:
	BView(frame, "palette_debug_view", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE)
{
	fParent = parent;
	fHostPalette = fParent ? fParent->Palette() : nullptr;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

	MakeFocus(true);
}


// -----------------------------------------------------------------------------
// PaletteDebugView::~PaletteDebugView
//
// Destroys the palette debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PaletteDebugView::~PaletteDebugView()
{
}



// -----------------------------------------------------------------------------
// PaletteDebugView::AttachedToWindow
//
// Finalizes view setup after the palette debugger is attached to a window.  The
// view takes focus and enables pointer tracking so palette entries can be
// inspected by hover.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	// Make sure the view continues receiving mouse motion events
	// cleanly while the pointer is inside the palette viewer.
	SetEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}



// -----------------------------------------------------------------------------
// PaletteDebugView::MessageReceived
//
// Handles messages sent to the palette debugger view.  PaletteDebugView does
// not currently consume custom messages, so messages are forwarded to BView.
//
// Parameters:
//   message - Message received by the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}



// -----------------------------------------------------------------------------
// PaletteDebugView::Pulse
//
// Refreshes the palette debugger during live updates.  When palette updates are
// frozen, the current display is kept stable for inspection.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::Pulse()
{
	if (fFreezeUpdates)
		return;

	Invalidate();
}



// -----------------------------------------------------------------------------
// PaletteDebugView::Draw
//
// Draws the complete palette debugger UI, including the controls panel,
// background palette panel, sprite palette panel, and selected-color details.
//
// Parameters:
//   updateRect - Region requested for redraw.  The view redraws all panels.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216, 255);
	FillRect(Bounds());

	DrawHeaderUI();
	DrawBackgroundPalettes();
	DrawSpritePalettes();
	DrawSelectedInfo();
}


// -----------------------------------------------------------------------------
// PaletteDebugView::DrawHeaderUI
//
// Draws the controls/help panel at the top of the palette debugger.  This shows
// mouse behavior, freeze behavior, and highlight legend information.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawHeaderUI()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		96.0f
	);

	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 58.0f;

	float y = panel.top + 34.0f;

	auto drawKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35, 255);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawKV("Mouse:", "hover inspect / click lock");
	drawKV("Space:", fFreezeUpdates
		? "unfreeze palette updates"
		: "freeze palette updates");
	drawKV("X:", "mirrored palette entry");
	//drawKV("Blue:", "source palette from NameTable");
	drawKV("Blue:", "external source palette");
}



// -----------------------------------------------------------------------------
// PaletteDebugView::DrawBackgroundPalettes
//
// Draws the background palette section for palette RAM entries $3F00-$3F0F.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawBackgroundPalettes()
{
	BRect panel(
		4.0f,
		108.0f,
		Bounds().right - 4.0f,
		248.0f
	);

	DrawPalettePanel(panel, "Background Palettes", false);
}


// -----------------------------------------------------------------------------
// PaletteDebugView::DrawSpritePalettes
//
// Draws the sprite palette section for palette RAM entries $3F10-$3F1F.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawSpritePalettes()
{
	BRect panel(
		4.0f,
		258.0f,
		Bounds().right - 4.0f,
		398.0f
	);

	DrawPalettePanel(panel, "Sprite Palettes", true);
}



// -----------------------------------------------------------------------------
// PaletteDebugView::DrawPalettePanel
//
// Draws one palette group panel.  The same renderer is used for both
// background palettes and sprite palettes, with the sprites flag selecting the
// base palette RAM address range.
//
// The panel also draws local hover/lock highlighting and optional external
// highlights sent by other debugger views.
//
// Parameters:
//   panel   - Rectangle containing the palette panel.
//   title   - Panel title text.
//   sprites - true to draw sprite palettes; false to draw background palettes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawPalettePanel(BRect panel, const char* title, bool sprites)
{
	::DrawDebugPanel(this, panel, title);

	SetFontSize(11.0f);

	// Fixed palette-label column.
	const float labelRightX = panel.left + 52.0f;
	const float cellStartX = panel.left + 66.0f;

	const float cellW = 42.0f;
	const float cellH = 18.0f;
	const float gapX = 8.0f;
	const float rowH = 25.0f;

	const float firstRowY = panel.top + 34.0f;

	uint16 active = fEntryLocked ? fLockedAddress : fHoverAddress;

	bool activeInThisPanel = sprites
		? active >= 0x3F10 && active <= 0x3F1F
		: active >= 0x3F00 && active <= 0x3F0F;

	int32 activePalette = -1;

	if (activeInThisPanel) {
		activePalette = sprites
			? ((active - 0x3F10) / 4)
			: ((active - 0x3F00) / 4);
	}

	bool externalInThisPanel = fHasExternalHighlight
		&& fExternalHighlightSprites == sprites
		&& fExternalHighlightPalette >= 0
		&& fExternalHighlightPalette <= 3;

	BString s;

	for (int32 pal = 0; pal < 4; pal++) {
		float y = firstRowY + pal * rowH;

		BRect rowRect(
			panel.left + 8.0f,
			y - 2.0f,
			panel.right - 8.0f,
			y + cellH + 2.0f
		);

		// Blue external source highlight, usually sent from NameTableView.
		if (externalInThisPanel && pal == fExternalHighlightPalette) {
			BRect extRect = rowRect.InsetByCopy(1.0f, 1.0f);

			SetHighColor(218, 232, 255, 255);
			FillRect(extRect);

			SetHighColor(45, 110, 210, 255);
			StrokeRect(extRect);

			BRect inner = extRect.InsetByCopy(1.0f, 1.0f);
			if (inner.IsValid()) {
				SetHighColor(85, 145, 235, 255);
				StrokeRect(inner);
			}
		}

		// Yellow local hover/lock row highlight.
		if (pal == activePalette) {
			SetHighColor(238, 238, 190, 255);
			FillRect(rowRect);

			SetHighColor(190, 175, 80, 255);
			StrokeRect(rowRect);
		}

		s.SetToFormat("Pal %ld", (long)pal);

		// Right-align the row label inside the fixed label column.
		SetHighColor(70, 70, 70, 255);
		float labelW = StringWidth(s);
		DrawString(s, BPoint(labelRightX - labelW, y + 13.0f));

		for (int32 entry = 0; entry < 4; entry++) {
			uint16 address = sprites
				? static_cast<uint16>(0x3F10 + pal * 4 + entry)
				: static_cast<uint16>(0x3F00 + pal * 4 + entry);

			BRect r(
				cellStartX + entry * (cellW + gapX),
				y,
				cellStartX + entry * (cellW + gapX) + cellW - 1.0f,
				y + cellH - 1.0f
			);

			bool selected = (active == address);

			DrawPaletteEntry(r, address, selected);

			// Optional exact-entry external highlight.
			if (externalInThisPanel
				&& pal == fExternalHighlightPalette
				&& fExternalHighlightEntry == entry) {
				BRect entryRect = r.InsetByCopy(-3.0f, -3.0f);

				SetHighColor(45, 110, 210, 255);
				StrokeRect(entryRect);

				BRect inner = entryRect.InsetByCopy(1.0f, 1.0f);
				if (inner.IsValid()) {
					SetHighColor(85, 145, 235, 255);
					StrokeRect(inner);
				}

				BRect innerWhite = entryRect.InsetByCopy(2.0f, 2.0f);
				if (innerWhite.IsValid()) {
					SetHighColor(255, 255, 255, 255);
					StrokeRect(innerWhite);
				}
			}
		}
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugView::DrawPaletteEntry
//
// Draws a single NES palette RAM entry.  The entry is resolved through the
// host palette table and BScreen color map so the displayed color matches the
// emulator's indexed-color output.
//
// Mirrored entries are marked with an X, and the selected entry receives a
// magenta/white outline.
//
// Parameters:
//   r        - Rectangle to fill with the palette color.
//   address  - NES palette RAM address represented by this cell.
//   selected - true if this cell is the active hover/locked entry.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawPaletteEntry(BRect r, uint16 address, bool selected)
{
	uint16 resolved = ResolvePaletteAddress(address);
	uint8 nesColor = ReadPalette(address);

	uint8 hostIndex = nesColor;

	if (fHostPalette)
		hostIndex = fHostPalette[nesColor & 0x3F];

	const color_map* cmap = BScreen().ColorMap();
	rgb_color rgb = {0, 0, 0, 255};

	if (cmap)
		rgb = cmap->color_list[hostIndex];

	SetHighColor(rgb);
	FillRect(r);

	SetHighColor(0, 0, 0, 255);
	StrokeRect(r);

	if (resolved != address) {
		SetHighColor(0, 0, 0, 170);

		StrokeLine(
			BPoint(r.left + 3.0f, r.top + 3.0f),
			BPoint(r.right - 3.0f, r.bottom - 3.0f)
		);

		StrokeLine(
			BPoint(r.left + 3.0f, r.bottom - 3.0f),
			BPoint(r.right - 3.0f, r.top + 3.0f)
		);
	}

	if (selected) {
		SetHighColor(255, 0, 255, 255);
		StrokeRect(r.InsetByCopy(-2.0f, -2.0f));

		SetHighColor(255, 255, 255, 255);
		StrokeRect(r.InsetByCopy(-1.0f, -1.0f));
	}

	BString s;
	s.SetToFormat("%02X", nesColor & 0x3f);

	BFont oldFont;
	GetFont(&oldFont);

	BFont fixedFont(be_fixed_font);
	fixedFont.SetSize(10.0f);
	SetFont(&fixedFont);

	int32 brightness = rgb.red + rgb.green + rgb.blue;

	if (brightness < 260)
		SetHighColor(255, 255, 255, 255);
	else
		SetHighColor(0, 0, 0, 255);

	float textW = StringWidth(s);
	float textX = r.left + ((r.Width() - textW) * 0.5f);
	float textY = r.top + 13.0f;

	DrawString(s.String(), BPoint(textX, textY));

	SetFont(&oldFont);
}


// -----------------------------------------------------------------------------
// PaletteDebugView::DrawSelectedInfo
//
// Draws detailed information for the active palette entry.  The active entry is
// either the locked entry or, when unlocked, the current hover entry.
//
// The panel shows the palette address, mirror mapping, background/sprite group,
// palette row, entry number, NES color index, host color index, RGB value,
// PPUMASK value, monochrome state, color-emphasis bits, and hover/lock state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::DrawSelectedInfo()
{
	BRect panel(
		4.0f,
		408.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Selected Color");

	uint16 address = fEntryLocked ? fLockedAddress : fHoverAddress;
	uint16 resolved = ResolvePaletteAddress(address);

	uint8 nesColor = ReadPalette(address);

	uint8 hostIndex = nesColor;
	if (fHostPalette)
		hostIndex = fHostPalette[nesColor & 0x3f];

	const color_map* cmap = BScreen().ColorMap();
	rgb_color rgb = {0, 0, 0, 255};

	if (cmap)
		rgb = cmap->color_list[hostIndex];

	bool sprites = address >= 0x3f10;

	int32 palette = sprites
		? ((address - 0x3f10) / 4)
		: ((address - 0x3f00) / 4);

	int32 entry = sprites
		? ((address - 0x3f10) % 4)
		: ((address - 0x3f00) % 4);

	uint8 mask = nes::ppu::ppumask();

	bool monochrome = (mask & 0x01) != 0;
	bool emphR = (mask & 0x20) != 0;
	bool emphG = (mask & 0x40) != 0;
	bool emphB = (mask & 0x80) != 0;

	BString emphasis;

	if (!emphR && !emphG && !emphB) {
		emphasis.SetTo("none");
	} else {
		emphasis.SetTo("");

		if (emphR)
			emphasis.Append("R");

		if (emphG)
			emphasis.Append("G");

		if (emphB)
			emphasis.Append("B");
	}

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 78.0f;

	const float rightLabelX = panel.left + 205.0f;
	const float rightValueX = rightLabelX + 62.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BString s;

	auto drawLeftKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	s.SetToFormat("$%04X", address);
	drawLeftKV("Address:", s.String());

	if (resolved != address) {
		s.SetToFormat("$%04X -> $%04X", address, resolved);
		drawLeftKV("Mirror:", s.String());
	} else {
		drawLeftKV("Mirror:", "none");
	}

	drawLeftKV("Group:", sprites ? "Sprite" : "Background");

	s.SetToFormat("%ld", (long)palette);
	drawLeftKV("Palette:", s.String());

	s.SetToFormat("%ld", (long)entry);
	drawLeftKV("Entry:", s.String());

	s.SetToFormat("$%02X", nesColor & 0x3f);
	drawRightKV("NES:", s.String());

	s.SetToFormat("%u", (unsigned)hostIndex);
	drawRightKV("Host:", s.String());

	s.SetToFormat("%u,%u,%u",
		(unsigned)rgb.red,
		(unsigned)rgb.green,
		(unsigned)rgb.blue);
	drawRightKV("RGB:", s.String());

	s.SetToFormat("$%02X", mask);
	drawRightKV("PPUMASK:", s.String());

	drawRightKV("Mono:", monochrome ? "ON" : "off");
	drawRightKV("Emph:", emphasis.String());

	drawRightKV("State:", fEntryLocked ? "LOCKED" : "HOVER");

	BRect swatch(
		panel.right - 66.0f,
		panel.top + 70.0f,
		panel.right - 20.0f,
		panel.top + 116.0f
	);

	SetHighColor(rgb);
	FillRect(swatch);

	SetHighColor(0, 0, 0, 255);
	StrokeRect(swatch);
}


// -----------------------------------------------------------------------------
// PaletteDebugView::MouseMoved
//
// Updates the hover palette entry as the mouse moves over the palette panels.
// Hover tracking is disabled while the view is frozen or while an entry is
// locked.
//
// Parameters:
//   where   - Current mouse position in view coordinates.
//   transit - Pointer transit code from the BeAPI.
//   message - Optional drag/drop message associated with the movement.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::MouseMoved(BPoint where, uint32 transit, const BMessage *message)
{
	(void)where;
	(void)message;

	if (transit == B_EXITED_VIEW) {
		fMouseInside = false;

		if (!fEntryLocked && !fFreezeUpdates)
			Invalidate();

		return;
	}

	fMouseInside = true;

	// Freeze means the visible/active palette inspection stays fixed.
	// Mouse movement should not change the active entry while frozen.
	if (fFreezeUpdates)
		return;

	// When locked, mouse movement should not change the active entry.
	if (fEntryLocked)
		return;

	uint16 address = 0x3F00;

	if (PaletteEntryAt(where, address)) {
		if (fHoverAddress != address) {
			fHoverAddress = address;
			Invalidate();
		}
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugView::MouseDown
//
// Locks or unlocks a palette entry when the user clicks a palette cell.
// Clicking the currently locked entry unlocks it; clicking any other palette
// entry locks that entry.
//
// Mouse locking is disabled while palette updates are frozen.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::MouseDown(BPoint where)
{
	MakeFocus(true);

	// Freeze means the palette viewer is locked in its current state.
	// Allow Space to unfreeze, but do not allow mouse clicks to change
	// the selected/locked palette entry.
	if (fFreezeUpdates)
		return;

	uint16 address = 0x3f00;

	if (!PaletteEntryAt(where, address))
		return;

	// Clicking the already locked entry unlocks it.
	if (fEntryLocked && fLockedAddress == address) {
		fEntryLocked = false;
		fHoverAddress = address;
		Invalidate();
		return;
	}

	// Clicking any palette entry locks that entry.
	fEntryLocked = true;
	fLockedAddress = address;
	fHoverAddress = address;

	Invalidate();
}


// -----------------------------------------------------------------------------
// PaletteDebugView::KeyDown
//
// Handles keyboard shortcuts for the palette debugger.  Space toggles frozen
// palette inspection so the visible values remain stable while the emulator
// continues running.
//
// Parameters:
//   bytes    - Key bytes supplied by the BeAPI.
//   numBytes - Number of valid bytes in bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes <= 0)
		return;

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugView::PaletteEntryAt
//
// Converts a mouse position into a NES palette RAM address.  Both the
// background and sprite palette panels are checked, with a small hit-margin
// around each cell to make selection easier.
//
// Parameters:
//   where      - Mouse position in view coordinates.
//   outAddress - Receives the palette RAM address if a cell is hit.
//
// Returns:
//   true if the point is over a palette entry; false otherwise.
// -----------------------------------------------------------------------------
bool
PaletteDebugView::PaletteEntryAt(BPoint where, uint16 &outAddress) const
{
	const float cellW = 42.0f;
	const float cellH = 18.0f;
	const float gapX = 8.0f;
	const float rowH = 25.0f;

	auto checkPanel = [&](BRect panel, bool sprites) -> bool {
		const float cellStartX = panel.left + 66.0f;
		const float firstRowY = panel.top + 34.0f;

		for (int32 pal = 0; pal < 4; pal++) {
			float y = firstRowY + pal * rowH;

			for (int32 entry = 0; entry < 4; entry++) {
				uint16 address = sprites
					? static_cast<uint16>(0x3f10 + pal * 4 + entry)
					: static_cast<uint16>(0x3f00 + pal * 4 + entry);

				BRect r(
					cellStartX + entry * (cellW + gapX),
					y,
					cellStartX + entry * (cellW + gapX) + cellW - 1.0f,
					y + cellH - 1.0f
				);

				r.InsetBy(-4.0f, -4.0f);

				if (r.Contains(where)) {
					outAddress = address;
					return true;
				}
			}
		}

		return false;
	};

	BRect bgPanel(
		4.0f,
		108.0f,
		Bounds().right - 4.0f,
		248.0f
	);

	BRect spritePanel(
		4.0f,
		258.0f,
		Bounds().right - 4.0f,
		398.0f
	);

	if (checkPanel(bgPanel, false))
		return true;

	if (checkPanel(spritePanel, true))
		return true;

	return false;
}


// -----------------------------------------------------------------------------
// PaletteDebugView::ResolvePaletteAddress
//
// Resolves NES palette RAM mirroring.  Palette addresses are folded into the
// $3F00-$3F1F range, and the mirrored sprite universal-color entries are mapped
// back to their background counterparts.
//
// Parameters:
//   address - Palette address to normalize and resolve.
//
// Returns:
//   Resolved palette RAM address.
// -----------------------------------------------------------------------------
uint16
PaletteDebugView::ResolvePaletteAddress(uint16 address) const
{
	address &= 0x3fff;

	if (address >= 0x3f00)
		address = 0x3f00 | (address & 0x1f);

	switch (address) {
		case 0x3f10:
			return 0x3f00;

		case 0x3f14:
			return 0x3f04;

		case 0x3f18:
			return 0x3f08;

		case 0x3f1c:
			return 0x3f0c;

		default:
			return address;
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugView::ReadPalette
//
// Reads a NES palette RAM value through the active cartridge mapper.  The value
// is masked to the valid NES color range of 0-63.
//
// Parameters:
//   address - Palette RAM address to read.
//
// Returns:
//   NES color index stored at the requested palette address, or $0F if no
//   mapper is available.
// -----------------------------------------------------------------------------
uint8
PaletteDebugView::ReadPalette(uint16 address) const
{
	Mapper* mapper = nes::cart.mapper();

	if (!mapper)
		return 0xf;

	return mapper->read_vram(address) & 0x3f;
}


// -----------------------------------------------------------------------------
// PaletteDebugView::SetExternalHighlight
//
// Sets a palette highlight requested by another debugger view.  The highlight
// can mark an entire palette row, or a specific entry within that row.
//
// Parameters:
//   sprites - true to highlight a sprite palette row; false for background.
//   palette - Palette row index, 0-3.
//   entry   - Palette entry index, 0-3, or -1 to highlight the whole row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::SetExternalHighlight(bool sprites, int32 palette, int32 entry)
{
	if (palette < 0 || palette > 3) {
		ClearExternalHighlight();
		return;
	}

	if (entry < -1 || entry > 3)
		entry = -1;

	fHasExternalHighlight = true;
	fExternalHighlightSprites = sprites;
	fExternalHighlightPalette = palette;
	fExternalHighlightEntry = entry;

	Invalidate();
}


// -----------------------------------------------------------------------------
// PaletteDebugView::ClearExternalHighlight
//
// Clears any palette highlight requested by another debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugView::ClearExternalHighlight()
{
	if (!fHasExternalHighlight)
		return;

	fHasExternalHighlight = false;
	fExternalHighlightSprites = false;
	fExternalHighlightPalette = -1;
	fExternalHighlightEntry = -1;

	Invalidate();
}

