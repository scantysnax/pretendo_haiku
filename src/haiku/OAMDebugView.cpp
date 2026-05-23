#include "OAMDebugView.h"
#include "PretendoWindow.h"
#include "DebugHelpers.h"

#include "Ppu.h"

#include <stdio.h>


OAMDebugView::OAMDebugView(BRect frame, PretendoWindow* parent)
	:
	BView(frame, "oam_debug_view", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


OAMDebugView::~OAMDebugView()
{
}


void
OAMDebugView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
OAMDebugView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}


void
OAMDebugView::Pulse()
{
	if (fFreezeUpdates)
		return;

	Invalidate();
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
	drawKV("Enter:", "open sprite details later");
}


void
OAMDebugView::DrawOAMSummaryPanel()
{
	BRect panel(
		4.0f,
		88.0f,
		Bounds().right - 4.0f,
		158.0f
	);

	::DrawDebugPanel(this, panel, "OAM Summary");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 86.0f;

	float y = panel.top + 36.0f;

	char s[128];

	auto drawKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawKV("Sprites:", "64");

	snprintf(s, sizeof(s), "$%02X", nes::ppu::ppuctrl());
	drawKV("PPUCTRL:", s);

	drawKV("Mode:", (nes::ppu::ppuctrl() & 0x20) ? "8x16 sprites" : "8x8 sprites");
}


void
OAMDebugView::DrawSpriteListPanel()
{
	BRect panel(
		4.0f,
		168.0f,
		Bounds().right - 4.0f,
		404.0f
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

	y += 14.0f;

	SetHighColor(150, 150, 150, 255);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f),
		BPoint(panel.right - 8.0f, y - 8.0f));

	const float rowH = 17.0f;

	// Placeholder rows for now. Real OAM reads come next.
	for (int32 i = 0; i < 8; i++) {
		char s[64];

		SetHighColor(0, 0, 0, 255);

		snprintf(s, sizeof(s), "%02ld", (long)i);
		DrawString(s, BPoint(xIndex, y));

		DrawString("--", BPoint(xY, y));
		DrawString("--", BPoint(xTile, y));
		DrawString("--", BPoint(xAttr, y));
		DrawString("--", BPoint(xX, y));
		DrawString("placeholder", BPoint(xInfo, y));

		y += rowH;
	}

	SetHighColor(90, 90, 90, 255);
	DrawString("Real OAM rows will replace these placeholders.",
		BPoint(panel.left + 10.0f, panel.bottom - 16.0f));
}


void
OAMDebugView::DrawSelectedSpritePanel()
{
	BRect panel(
		4.0f,
		414.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Selected Sprite");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 72.0f;

	const float rightLabelX = panel.left + 210.0f;
	const float rightValueX = rightLabelX + 74.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

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

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	char s[64];

	if (active >= 0) {
		snprintf(s, sizeof(s), "%02ld", (long)active);
		drawLeftKV("Sprite:", s);
	} else {
		drawLeftKV("Sprite:", "--");
	}

	drawLeftKV("Y:", "--");
	drawLeftKV("Tile:", "--");
	drawLeftKV("Attr:", "--");
	drawLeftKV("X:", "--");

	drawRightKV("Palette:", "--");
	drawRightKV("Priority:", "--");
	drawRightKV("Flip H:", "--");
	drawRightKV("Flip V:", "--");
	drawRightKV("State:", fSpriteLocked ? "LOCKED" : "HOVER");
}


void
OAMDebugView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	(void)message;

	if (transit == B_EXITED_VIEW) {
		fMouseInside = false;
		Invalidate();
		return;
	}

	fMouseInside = true;

	if (fSpriteLocked)
		return;

	// Placeholder hit testing:
	// Real sprite-row hit testing comes when we draw all 64 rows.
	BRect listPanel(
		4.0f,
		164.0f,
		Bounds().right - 4.0f,
		404.0f
	);

	if (!listPanel.Contains(where))
		return;

	float firstRowY = listPanel.top + 50.0f;
	float rowH = 18.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8)
		return;

	if (fHoverSprite != row) {
		fHoverSprite = row;
		Invalidate();
	}
}


void
OAMDebugView::MouseDown(BPoint where)
{
	MakeFocus(true);

	BRect listPanel(
		4.0f,
		164.0f,
		Bounds().right - 4.0f,
		404.0f
	);

	if (!listPanel.Contains(where))
		return;

	float firstRowY = listPanel.top + 50.0f;
	float rowH = 18.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8)
		return;

	if (fSpriteLocked && fLockedSprite == row) {
		fSpriteLocked = false;
		fHoverSprite = row;
	} else {
		fSpriteLocked = true;
		fLockedSprite = row;
		fHoverSprite = row;
	}

	Invalidate();
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

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}

