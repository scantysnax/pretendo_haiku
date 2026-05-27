#include "OAMDebugView.h"
#include "PretendoWindow.h"
#include "DebugHelpers.h"
#include "PatternTableWindow.h"
#include "PatternTableView.h"

#include "Ppu.h"

#include <cstdio>


class OAMSpriteScrollBar : public BScrollBar
{
	public:
	OAMSpriteScrollBar (BRect frame, OAMDebugView* owner)
		: BScrollBar(frame, "oam_sprite_scrollbar", nullptr, 0.0f, 56.0f, B_VERTICAL),
			fOwner(owner)
	{
		SetSteps(8.0f, 8.0f);
		SetProportion(8.0f / 64.0f);
	}

	virtual void ValueChanged(float value)
	{
		BScrollBar::ValueChanged(value);

		if (!fOwner)
			return;

		int32 firstSprite = static_cast<int32>(value);

		// Snap to 8-sprite pages so the rows stay stable.
		firstSprite = (firstSprite / 8) * 8;

		if (firstSprite < 0)
			firstSprite = 0;
		if (firstSprite > 56)
			firstSprite = 56;

		fOwner->SetFirstSpriteFromScrollBar(firstSprite);
	}

	private:
	OAMDebugView *fOwner;
};


static inline void
SetPatternWindowHighlight(PatternTableWindow *window, int32 whichPT, int32 tileIndex)
{
	if (!window)
		return;

	if (window->Lock()) {
		if (window->View())
			window->View()->SetExternalHighlight(whichPT, tileIndex);

		window->Unlock();
	}
}


static inline void
ClearPatternWindowHighlight(PatternTableWindow *window)
{
	if (!window)
		return;

	if (window->Lock()) {
		if (window->View())
			window->View()->ClearExternalHighlight();

		window->Unlock();
	}
}


OAMDebugView::OAMDebugView(BRect frame, PretendoWindow *parent)
	: BView(frame, "oam_debug_view", B_FOLLOW_ALL_SIDES,
	B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


OAMDebugView::~OAMDebugView()
{
	if (fParent)
		fParent->ClearPaletteDebuggerHighlight();

	ClearPatternTableHighlight();
}


void
OAMDebugView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	SetMouseEventMask(
		B_POINTER_EVENTS | B_MOUSE_WHEEL_CHANGED,
		B_NO_POINTER_HISTORY
	);

	if (!fSpriteScrollBar) {
		BRect listPanel(
			4.0f,
			174.0f,
			Bounds().right - 4.0f,
			432.0f
		);

		BRect scrollFrame(
			listPanel.right - 18.0f,
			listPanel.top + 26.0f,
			listPanel.right - 4.0f,
			listPanel.bottom - 22.0f
		);

		fSpriteScrollBar = new OAMSpriteScrollBar(scrollFrame, this);
		AddChild(fSpriteScrollBar);
		fSpriteScrollBar->SetValue(fFirstSprite);
	}
}

void
OAMDebugView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216, 255);
	FillRect(Bounds());

	DrawHeaderUI();
	DrawOAMSummaryPanel();
	DrawSpriteListPanel();
	DrawSelectedSpritePanel();
}


void
OAMDebugView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

	(void)width;
	(void)height;

	if (!fSpriteScrollBar)
		return;

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	BRect scrollFrame(
		listPanel.right - 18.0f,
		listPanel.top + 26.0f,
		listPanel.right - 4.0f,
		listPanel.bottom - 22.0f
	);

	fSpriteScrollBar->MoveTo(scrollFrame.LeftTop());
	fSpriteScrollBar->ResizeTo(scrollFrame.Width(), scrollFrame.Height());
}


void
OAMDebugView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes <= 0)
		return;

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			break;

		case '[':
		case ',':
			fFirstSprite -= 8;

			if (fFirstSprite < 0)
				fFirstSprite = 0;

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite
					|| fHoverSprite >= fFirstSprite + 8) {
					fHoverSprite = fFirstSprite;
				}
			}

			if (fSpriteScrollBar)
				fSpriteScrollBar->SetValue(fFirstSprite);

			UpdatePaletteDebuggerHighlight();
			UpdatePatternTableHighlight();

			Invalidate();
			break;

		case ']':
		case '.':
			fFirstSprite += 8;

			if (fFirstSprite > 56)
				fFirstSprite = 56;

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite
					|| fHoverSprite >= fFirstSprite + 8) {
					fHoverSprite = fFirstSprite;
				}
			}

			if (fSpriteScrollBar)
				fSpriteScrollBar->SetValue(fFirstSprite);

			UpdatePaletteDebuggerHighlight();
			UpdatePatternTableHighlight();

			Invalidate();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


void
OAMDebugView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY = 0.0f;

			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK) {
				BView::MessageReceived(message);
				return;
			}

			int32 oldFirstSprite = fFirstSprite;

			if (deltaY > 0.0f) {
				fFirstSprite += 8;
			} else if (deltaY < 0.0f) {
				fFirstSprite -= 8;
			}

			if (fFirstSprite < 0)
				fFirstSprite = 0;

			if (fFirstSprite > 56)
				fFirstSprite = 56;

			if (fFirstSprite != oldFirstSprite) {
				if (fSpriteScrollBar)
					fSpriteScrollBar->SetValue(fFirstSprite);

				if (!fSpriteLocked) {
					if (fHoverSprite < fFirstSprite
						|| fHoverSprite >= fFirstSprite + 8) {
						fHoverSprite = fFirstSprite;
					}
				}

				UpdatePaletteDebuggerHighlight();
				UpdatePatternTableHighlight();

				Invalidate();
			}

			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
OAMDebugView::MouseDown(BPoint where)
{
	MakeFocus(true);

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	if (!listPanel.Contains(where))
		return;

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8)
		return;

	int32 spriteIndex = fFirstSprite + row;

	if (spriteIndex < 0 || spriteIndex >= 64)
		return;

	if (fSpriteLocked && fLockedSprite == spriteIndex) {
		fSpriteLocked = false;
		fHoverSprite = spriteIndex;
	} else {
		fSpriteLocked = true;
		fLockedSprite = spriteIndex;
		fHoverSprite = spriteIndex;
	}

	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();

	Invalidate();
}


void
OAMDebugView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	(void)message;

	if (transit == B_EXITED_VIEW) {
		fMouseInside = false;

		if (!fSpriteLocked) {
			fHoverSprite = -1;

			if (fParent)
				fParent->ClearPaletteDebuggerHighlight();

			ClearPatternTableHighlight();
		}

		Invalidate();
		return;
	}

	fMouseInside = true;

	if (fSpriteLocked)
		return;

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	if (!listPanel.Contains(where))
		return;

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8)
		return;

	int32 spriteIndex = fFirstSprite + row;

	if (spriteIndex < 0 || spriteIndex >= 64)
		return;

	if (fHoverSprite != spriteIndex) {
		fHoverSprite = spriteIndex;

		UpdatePaletteDebuggerHighlight();
		UpdatePatternTableHighlight();

		Invalidate();
	}
}


void
OAMDebugView::Pulse()
{
	if (fFreezeUpdates)
		return;

	Invalidate();
}


void
OAMDebugView::DrawHeaderUI()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		76.0f
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
		? "unfreeze OAM updates"
		: "freeze OAM updates");
	drawKV("[ / ]:", "previous / next 8 sprites");
}


void
OAMDebugView::DrawOAMSummaryPanel()
{
	BRect panel(
		4.0f,
		88.0f,
		Bounds().right - 4.0f,
		164.0f
	);

	::DrawDebugPanel(this, panel, "OAM Summary");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont oldFont;
	GetFont(&oldFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	int32 usedSprites = 0;
	int32 hiddenSprites = 0;

	for (int32 i = 0; i < 64; i++) {
		uint8 spriteY = nes::ppu::oam_ram((i * 4) + 0);

		if (spriteY >= 0xef)
			hiddenSprites++;
		else
			usedSprites++;
	}

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 70.0f;

	const float rightLabelX = panel.left + 210.0f;
	const float rightValueX = rightLabelX + 72.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BString s;

	auto drawLeftKV = [&](const char* label, const char* value,
		bool monoValue) {
		SetHighColor(80, 80, 80, 255);
		SetFont(&oldFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0, 255);
		SetFont(monoValue ? &mono : &oldFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char* label, const char* value,
		bool monoValue) {
		SetHighColor(80, 80, 80, 255);
		SetFont(&oldFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0, 255);
		SetFont(monoValue ? &mono : &oldFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	s.SetToFormat("%ld / 64", (long)usedSprites);
	drawLeftKV("OAM Used:", s.String(), false);

	s.SetToFormat("%ld", (long)hiddenSprites);
	drawLeftKV("Hidden:", s.String(), false);

	drawRightKV("Mode:", (nes::ppu::ppuctrl() & 0x20)
		? "8x16 sprites"
		: "8x8 sprites", false);

	s.SetToFormat("$%02X", nes::ppu::ppuctrl());
	drawRightKV("PPUCTRL:", s.String(), true);

	SetFont(&oldFont);
}

void
OAMDebugView::DrawSpriteListPanel()
{
	BRect panel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	::DrawDebugPanel(this, panel, "Sprite List");

	SetFontSize(11.0f);

	const float xIndex = panel.left + 10.0f;
	const float xY = panel.left + 50.0f;
	const float xTile = panel.left + 92.0f;
	const float xAttr = panel.left + 144.0f;
	const float xX = panel.left + 198.0f;
	const float xInfo = panel.left + 240.0f;

	float y = panel.top + 36.0f;

	SetHighColor(80, 80, 80, 255);
	DrawString("#", BPoint(xIndex, y));
	DrawString("Y", BPoint(xY, y));
	DrawString("Tile", BPoint(xTile, y));
	DrawString("Attr", BPoint(xAttr, y));
	DrawString("X", BPoint(xX, y));
	DrawString("Info", BPoint(xInfo, y));

	y += 22.0f;

	SetHighColor(150, 150, 150, 255);
	StrokeLine(
		BPoint(panel.left + 8.0f, y - 13.0f),
		BPoint(panel.right - 24.0f, y - 13.0f)
	);

	const float firstRowY = y;
	const float rowH = 17.0f;

	BFont oldFont;
	GetFont(&oldFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	for (int32 row = 0; row < 8; row++) {
		int32 spriteIndex = fFirstSprite + row;

		if (spriteIndex < 0 || spriteIndex >= 64)
			continue;

		uint32 base = spriteIndex * 4;

		uint8 spriteY = nes::ppu::oam_ram(base + 0);
		uint8 tile = nes::ppu::oam_ram(base + 1);
		uint8 attr = nes::ppu::oam_ram(base + 2);
		uint8 spriteX = nes::ppu::oam_ram(base + 3);

		uint8 pal = attr & 0x03;
		bool priority = (attr & 0x20) != 0;
		bool flipH = (attr & 0x40) != 0;
		bool flipV = (attr & 0x80) != 0;

		float rowY = firstRowY + (row * rowH);

		BRect rowRect(
			panel.left + 7.0f,
			rowY - 12.0f,
			panel.right - 24.0f,
			rowY + 4.0f
		); 
		
		if (spriteIndex == active) {
			if (fSpriteLocked) {
				SetHighColor(255, 230, 245, 255);
				FillRect(rowRect);

				SetHighColor(210, 80, 170, 255);
				StrokeRect(rowRect);
			} else {
				SetHighColor(238, 238, 190, 255);
				FillRect(rowRect);

				SetHighColor(190, 175, 80, 255);
				StrokeRect(rowRect);
			}
		}

		BString s;

		SetHighColor(0, 0, 0, 255);

		SetFont(&mono);

		s.SetToFormat("%02ld", (long)spriteIndex);
		DrawString(s, BPoint(xIndex, rowY));

		s.SetToFormat("$%02X", spriteY);
		DrawString(s.String(), BPoint(xY, rowY));

		s.SetToFormat("$%02X", tile);
		DrawString(s.String(), BPoint(xTile, rowY));

		s.SetToFormat("$%02X", attr);
		DrawString(s.String(), BPoint(xAttr, rowY));

		s.SetToFormat("$%02X", spriteX);
		DrawString(s.String(), BPoint(xX, rowY));

		SetFont(&oldFont);
		
		s.SetToFormat("P%u %s%s%s",
			(unsigned)pal,
			priority ? "B" : "F",
			flipH ? " H" : "",
			flipV ? " V" : "");
		DrawString(s.String(), BPoint(xInfo, rowY));
	}

	SetFont(&oldFont);

	BString footer;
	footer.SetToFormat("Showing OAM sprites %02ld-%02ld of 64.",
		(long)fFirstSprite,
		(long)(fFirstSprite + 7)
	);

	SetHighColor(90, 90, 90, 255);
	DrawString(
		footer.String(),
		BPoint(panel.left + 10.0f, panel.bottom - 14.0f)
	);
}


void
OAMDebugView::DrawSelectedSpritePanel()
{
	BRect panel(
		4.0f,
		442.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Selected Sprite");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 78.0f;

	const float rightLabelX = panel.left + 210.0f;
	const float rightValueX = rightLabelX + 72.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BFont oldFont;
	GetFont(&oldFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	auto drawLeftKV = [&](const char* label, const char* value, bool monoValue) {
		SetHighColor(80, 80, 80, 255);
		SetFont(&oldFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0, 255);
		SetFont(monoValue ? &mono : &oldFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char* label, const char* value, bool monoValue) {
		SetHighColor(80, 80, 80, 255);
		SetFont(&oldFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0, 255);
		SetFont(monoValue ? &mono : &oldFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	BRect previewRect(
		panel.right - 58.0f,
		panel.top + 34.0f,
		panel.right - 12.0f,
		panel.bottom - 12.0f
	);

	DrawSpritePreview(previewRect, active);

	BString s;

	if (active < 0) {
		drawLeftKV("Sprite:", "--", true);
		drawLeftKV("Raw Y:", "--", true);
		drawLeftKV("Screen Y:", "--", true);
		drawLeftKV("X:", "--", true);
		drawLeftKV("Tile:", "--", true);

		drawRightKV("Attr:", "--", true);
		drawRightKV("CHR:", "--", true);
		drawRightKV("Palette:", "--", false);
		drawRightKV("Priority:", "--", false);
		drawRightKV("Flip:", "--", false);
		drawRightKV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

		SetFont(&oldFont);
		return;
	}

	uint32 base = active * 4;

	uint8 spriteY = nes::ppu::oam_ram(base + 0);
	uint8 tile = nes::ppu::oam_ram(base + 1);
	uint8 attr = nes::ppu::oam_ram(base + 2);
	uint8 spriteX = nes::ppu::oam_ram(base + 3);

	uint8 pal = attr & 0x03;
	bool priority = (attr & 0x20) != 0;
	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	uint32 chrAddr = 0;
	uint32 chrAddrBottom = 0;

	if (largeSprites) {
		uint32 whichPT = tile & 0x01;
		uint32 topTile = tile & 0xfe;

		chrAddr = (whichPT ? 0x1000 : 0x0000) + (topTile * 16);
		chrAddrBottom = chrAddr + 16;
	} else {
		uint32 spritePatternBase = (nes::ppu::ppuctrl() & 0x08)
			? 0x1000
			: 0x0000;

		chrAddr = spritePatternBase + (tile * 16);
	}

	s.SetToFormat("%02ld", (long)active);
	drawLeftKV("Sprite:", s.String(), true);

	s.SetToFormat("$%02X", spriteY);
	drawLeftKV("Raw Y:", s.String(), true);

	s.SetToFormat("%u", (unsigned)((uint16)spriteY + 1));
	drawLeftKV("Screen Y:", s.String(), true);
	drawLeftKV("Visible:", spriteY < 0xef ? "yes" : "offscreen", false);

	s.SetToFormat("$%02X", spriteX);
	drawLeftKV("X:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%02X/$%02X",
			tile & 0xfe,
			(tile & 0xfe) + 1);
		drawLeftKV("Tiles:", s.String(), true);
	} else {
		s.SetToFormat("$%02X", tile);
		drawLeftKV("Tile:", s.String(), true);
	}

	s.SetToFormat("$%02X", attr);
	drawRightKV("Attr:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%04lX/$%04lX",
			(unsigned long)chrAddr,
			(unsigned long)chrAddrBottom);
		drawRightKV("CHR:", s.String(), true);
	} else {
		s.SetToFormat("$%04lX", (unsigned long)chrAddr);
		drawRightKV("CHR:", s.String(), true);
	}

	s.SetToFormat("%u", (unsigned)pal);
	drawRightKV("Palette:", s.String(), false);

	drawRightKV("Priority:", priority ? "behind BG" : "in front", false);

	if (flipH && flipV)
		drawRightKV("Flip:", "H + V", false);
	else if (flipH)
		drawRightKV("Flip:", "H", false);
	else if (flipV)
		drawRightKV("Flip:", "V", false);
	else
		drawRightKV("Flip:", "none", false);

	drawRightKV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

	SetFont(&oldFont);
}




void
OAMDebugView::SetFirstSpriteFromScrollBar(int32 firstSprite)
{
	firstSprite = (firstSprite / 8) * 8;

	if (firstSprite < 0)
		firstSprite = 0;
	if (firstSprite > 56)
		firstSprite = 56;

	if (fFirstSprite == firstSprite)
		return;

	fFirstSprite = firstSprite;

	if (!fSpriteLocked) {
		if (fHoverSprite < fFirstSprite || fHoverSprite >= fFirstSprite + 8)
			fHoverSprite = fFirstSprite;
	}
	
	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();

	Invalidate();
}

void
OAMDebugView::SetHostPalette(uint8 *palette)
{
	fHostPalette = palette;
	Invalidate();
}


rgb_color
OAMDebugView::SpritePreviewColor(uint8 spritePalette, uint8 pixel) const
{
	uint32 paletteAddress;

	if (pixel == 0) {
		// Sprite pixel 0 is transparent, but for this preview we draw it
		// using the universal background color so the sprite preview matches
		// the current PPU palette.
		paletteAddress = 0x3f00;
	} else {
		// Sprite palettes live at $3F10-$3F1F.
		// Entries 1-3 are visible sprite colors.
		paletteAddress = 0x3f10 + (spritePalette * 4) + pixel;
	}

	uint8 nesColor = nes::ppu::palette_ram(paletteAddress) & 0x3f;

	if (!fHostPalette || !Window())
		return rgb_color{0, 0, 0, 255};

	uint8 hostIndex = fHostPalette[nesColor];

	BScreen screen(Window());
	return screen.ColorForIndex(hostIndex);
}


void
OAMDebugView::DrawSpritePreview(BRect previewRect, int32 spriteIndex)
{
	// Small shadow so the preview box stands off the gray panel.
	BRect shadowRect = previewRect;
	shadowRect.OffsetBy(2.0f, 2.0f);

	SetHighColor(190, 190, 190, 255);
	FillRect(shadowRect);

	// Outer preview area: light gray debugger background.
	SetHighColor(236, 236, 236, 255);
	FillRect(previewRect);

	SetHighColor(135, 135, 135, 255);
	StrokeRect(previewRect);

	// Inner highlight edge.
	SetHighColor(255, 255, 255, 255);
	StrokeLine(
		BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
		BPoint(previewRect.right - 1.0f, previewRect.top + 1.0f)
	);
	StrokeLine(
		BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
		BPoint(previewRect.left + 1.0f, previewRect.bottom - 1.0f)
	);

	if (spriteIndex < 0 || spriteIndex >= 64 || !nes::cart.mapper()) {
		SetHighColor(90, 90, 90, 255);
		DrawString("--", BPoint(previewRect.left + 12.0f,
			previewRect.top + 24.0f));
		return;
	}

	uint32 base = spriteIndex * 4;

	uint8 spriteY = nes::ppu::oam_ram(base + 0);
	uint8 tile = nes::ppu::oam_ram(base + 1);
	uint8 attr = nes::ppu::oam_ram(base + 2);

	uint8 palette = attr & 0x03;

	if (spriteY >= 0xef) {
		SetHighColor(90, 90, 90, 255);
		DrawString("OFF", BPoint(previewRect.left + 11.0f,
			previewRect.top + 27.0f));
		DrawString("SCR", BPoint(previewRect.left + 11.0f,
			previewRect.top + 42.0f));
		return;
	}

	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	const int spriteW = 8;
	const int spriteH = largeSprites ? 16 : 8;

	const float scale = largeSprites ? 3.0f : 4.0f;

	const float drawW = spriteW * scale;
	const float drawH = spriteH * scale;

	const float startX = previewRect.left
		+ floorf((previewRect.Width() + 1.0f - drawW) / 2.0f);
	const float startY = previewRect.top
		+ floorf((previewRect.Height() + 1.0f - drawH) / 2.0f);

	BRect spriteRect(
		startX,
		startY,
		startX + drawW - 1.0f,
		startY + drawH - 1.0f
	);

	// Inner sprite area: live universal background color from $3F00.
	// Transparent sprite pixels show through this color.
	SetHighColor(SpritePreviewColor(palette, 0));
	FillRect(spriteRect);

	for (int py = 0; py < spriteH; py++) {
		int srcY = flipV ? (spriteH - 1 - py) : py;

		uint32 chrAddr;

		if (largeSprites) {
			uint32 whichPT = tile & 0x01;
			uint32 topTile = tile & 0xfe;

			chrAddr = (whichPT ? 0x1000 : 0x0000)
				+ (topTile * 16)
				+ ((srcY / 8) * 16)
				+ (srcY & 0x07);
		} else {
			uint32 spritePatternBase = (nes::ppu::ppuctrl() & 0x08)
				? 0x1000
				: 0x0000;

			chrAddr = spritePatternBase + (tile * 16) + srcY;
		}

		uint8 plane0 = nes::cart.mapper()->read_vram(chrAddr);
		uint8 plane1 = nes::cart.mapper()->read_vram(chrAddr + 8);

		for (int px = 0; px < spriteW; px++) {
			int srcX = flipH ? px : (7 - px);

			uint8 pixel = ((plane0 >> srcX) & 0x01)
				| (((plane1 >> srcX) & 0x01) << 1);

			if (pixel == 0)
				continue;

			BRect r(
				startX + (px * scale),
				startY + (py * scale),
				startX + ((px + 1) * scale) - 1.0f,
				startY + ((py + 1) * scale) - 1.0f
			);

			SetHighColor(SpritePreviewColor(palette, pixel));
			FillRect(r);
		}
	}

	// Inner sprite canvas outline.
	SetHighColor(40, 40, 40, 255);
	StrokeRect(spriteRect);

	SetHighColor(255, 255, 255, 255);
	StrokeLine(
		BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
		BPoint(spriteRect.right - 1.0f, spriteRect.top + 1.0f)
	);
	StrokeLine(
		BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
		BPoint(spriteRect.left + 1.0f, spriteRect.bottom - 1.0f)
	);

	// Outer preview border, redrawn after all contents.
	SetHighColor(135, 135, 135, 255);
	StrokeRect(previewRect);
}

void
OAMDebugView::UpdatePaletteDebuggerHighlight()
{
	if (!fParent)
		return;

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= 64) {
		fParent->ClearPaletteDebuggerHighlight();
		return;
	}

	uint32 base = active * 4;

	uint8 attr = nes::ppu::oam_ram(base + 2);
	uint8 spritePalette = attr & 0x03;

	// true = sprite palette area, palette = 0..3, entry -1 = whole row.
	fParent->HighlightPaletteDebugger(true, spritePalette, -1);
}


void
OAMDebugView::SetPatternTables(PatternTableWindow *pt0, PatternTableWindow *pt1)
{
	fPatternTable0 = pt0;
	fPatternTable1 = pt1;

	UpdatePatternTableHighlight();
}


void
OAMDebugView::ClearPatternTableHighlight()
{
	ClearPatternWindowHighlight(fPatternTable0);
	ClearPatternWindowHighlight(fPatternTable1);
}


void
OAMDebugView::UpdatePatternTableHighlight()
{
	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= 64) {
		ClearPatternTableHighlight();
		return;
	}

	uint32 base = active * 4;

	uint8 tile = nes::ppu::oam_ram(base + 1);
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	int32 whichPT;
	int32 tileIndex;

	if (largeSprites) {
		whichPT = tile & 0x01;
		tileIndex = tile & 0xfe;
	} else {
		whichPT = (nes::ppu::ppuctrl() & 0x08) ? 1 : 0;
		tileIndex = tile;
	}

	if (whichPT == 0) {
		SetPatternWindowHighlight(fPatternTable0, whichPT, tileIndex);
		ClearPatternWindowHighlight(fPatternTable1);
	} else {
		ClearPatternWindowHighlight(fPatternTable0);
		SetPatternWindowHighlight(fPatternTable1, whichPT, tileIndex);
	}
}

