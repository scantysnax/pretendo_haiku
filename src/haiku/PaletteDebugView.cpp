
#include "PaletteDebugView.h"
#include "PretendoWindow.h"
#include "DebugHelpers.h"

#include "Cart.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"

#include <cstdio>
#include <cstring>


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


PaletteDebugView::~PaletteDebugView()
{
}


void
PaletteDebugView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	// Make sure the view continues receiving mouse motion events
	// cleanly while the pointer is inside the palette viewer.
	SetEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void
PaletteDebugView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}


void
PaletteDebugView::Pulse()
{
	if (fFreezeUpdates)
		return;

	Invalidate();
}


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
	drawKV("Blue:", "source palette from NameTable");
}


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

	char s[64];

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

		snprintf(s, sizeof(s), "Pal %ld", (long)pal);

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

	char s[16];
	snprintf(s, sizeof(s), "%02X", nesColor & 0x3F);

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

	DrawString(s, BPoint(textX, textY));

	SetFont(&oldFont);
}


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
		hostIndex = fHostPalette[nesColor & 0x3F];

	const color_map* cmap = BScreen().ColorMap();
	rgb_color rgb = {0, 0, 0, 255};

	if (cmap)
		rgb = cmap->color_list[hostIndex];

	bool sprites = address >= 0x3F10;

	int32 palette = sprites
		? ((address - 0x3F10) / 4)
		: ((address - 0x3F00) / 4);

	int32 entry = sprites
		? ((address - 0x3F10) % 4)
		: ((address - 0x3F00) % 4);

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

	char s[128];

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

	snprintf(s, sizeof(s), "$%04X", address);
	drawLeftKV("Address:", s);

	if (resolved != address) {
		snprintf(s, sizeof(s), "$%04X -> $%04X", address, resolved);
		drawLeftKV("Mirror:", s);
	} else {
		drawLeftKV("Mirror:", "none");
	}

	drawLeftKV("Group:", sprites ? "Sprite" : "Background");

	snprintf(s, sizeof(s), "%ld", (long)palette);
	drawLeftKV("Palette:", s);

	snprintf(s, sizeof(s), "%ld", (long)entry);
	drawLeftKV("Entry:", s);

	snprintf(s, sizeof(s), "$%02X", nesColor & 0x3F);
	drawRightKV("NES:", s);

	snprintf(s, sizeof(s), "%u", (unsigned)hostIndex);
	drawRightKV("Host:", s);

	snprintf(s, sizeof(s), "%u,%u,%u",
		(unsigned)rgb.red,
		(unsigned)rgb.green,
		(unsigned)rgb.blue);
	drawRightKV("RGB:", s);

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


void
PaletteDebugView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
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


bool
PaletteDebugView::PaletteEntryAt(BPoint where, uint16& outAddress) const
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
					? static_cast<uint16>(0x3F10 + pal * 4 + entry)
					: static_cast<uint16>(0x3F00 + pal * 4 + entry);

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


uint8
PaletteDebugView::ReadPalette(uint16 address) const
{
	Mapper* mapper = nes::cart.mapper();

	if (!mapper)
		return 0xf;

	return mapper->read_vram(address) & 0x3f;
}

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

