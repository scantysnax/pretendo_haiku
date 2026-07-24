
#include "PPUStatusView.h"

#include "Cart.h"
#include "DebugHelpers.h"
#include "Ppu.h"
#include "PretendoWindow.h"

#include <cmath>


static void
SetStateColor (BView *view, bool active)
{
	if (active) {
		view->SetHighColor(0, 120, 0);
	} else {
		view->SetHighColor(120, 120, 120);
	}
}


PPUStatusView::PPUStatusView (BRect frame, PretendoWindow *parent)
	: BView(frame, "ppu_status_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


PPUStatusView::~PPUStatusView()
{
}


void
PPUStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
}


// -----------------------------------------------------------------------------
// PPUStatusView::Pulse
//
// Refreshes the PPU status viewer while a ROM is loaded.  If no ROM is loaded,
// the empty-state view is static and does not need continuous redraws.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	Invalidate();
}


void
PPUStatusView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216, 255);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(
			4.0f,
			74.0f,
			Bounds().right - 4.0f,
			Bounds().bottom - 8.0f
		);

		::DrawDebugPanel(this, panel, "PPU State");
		DrawNoROMMessage(panel);
		return;
	}

	DrawRegisterPanel();
	DrawTimingPanel();
	DrawControlPanel();
	DrawMaskPanel();
	DrawStatusPanel();
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawHeaderPanel
//
// Draws the title/help panel for the PPU status debugger.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawHeaderPanel()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		62.0f
	);

	::DrawDebugPanel(this, panel, "PPU Status");

	SetFontSize(11.0f);

	SetHighColor(35, 35, 35);
	DrawString(
		"Live PPU register and render-state summary",
		BPoint(panel.left + 8.0f, panel.top + 36.0f)
	);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawRegisterPanel
//
// Draws raw PPU register values and low-level scroll/render address state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawRegisterPanel()
{
	BRect panel(
		4.0f,
		74.0f,
		Bounds().right - 4.0f,
		178.0f
	);

	::DrawDebugPanel(this, panel, "Registers");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(&mono);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(&mono);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	uint8 ctrl = nes::ppu::ppuctrl();
	uint8 mask = nes::ppu::ppumask();
	uint8 status = nes::ppu::ppustatus();
	uint8 oamAddr = nes::ppu::oamaddr();

	nes::ppu::scroll_state_t scroll = nes::ppu::scroll_state();
	
	BString s;

	s.SetToFormat("$%02X", ctrl);
	drawLeftKV("PPUCTRL:", s.String());

	s.SetToFormat("$%02X", mask);
	drawLeftKV("PPUMASK:", s.String());

	s.SetToFormat("$%02X", status);
	drawLeftKV("PPUSTATUS:", s.String());

	s.SetToFormat("$%02X", oamAddr);
	drawLeftKV("OAMADDR:", s.String());

	s.SetToFormat("$%04lX", static_cast<unsigned long>(scroll.v & 0x7fff));
	drawRightKV("Scroll v:", s.String());

	s.SetToFormat("$%04lX", static_cast<unsigned long>(scroll.t & 0x7fff));
	drawRightKV("Scroll t:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(scroll.x));
	drawRightKV("Fine X:", s.String());

	s.SetToFormat("$%02X", scroll.ctrl);
	drawRightKV("Ctrl sh:", s.String());

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawControlPanel
//
// Decodes PPUCTRL and the current scroll address into human-readable render
// state such as name table base, pattern table selection, sprite size, NMI
// enable state, and Loopy scroll fields.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawControlPanel()
{
	BRect panel(
		4.0f,
		270.0f,
		Bounds().right - 4.0f,
		400.0f
	);
	
	::DrawDebugPanel(this, panel, "Control / Scroll Decode");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	uint8 ctrl = nes::ppu::ppuctrl();
	nes::ppu::scroll_state_t scroll = nes::ppu::scroll_state();

	uint16 ntBase = 0x2000 + ((ctrl & 0x3) * 0x400);
	uint16 bgPT = (ctrl & 0x10) ? 0x1000 : 0x0000;
	uint16 spritePT = (ctrl & 0x8) ? 0x1000 : 0x0000;

	bool sprite8x16 = (ctrl & 0x20) != 0;
	bool nmiEnabled = (ctrl & 0x80) != 0;

	uint32 v = (scroll.v & 0x7fff);

	uint8 coarseX = v & 0x1f;
	uint8 coarseY = (v >> 5) & 0x1f;
	uint8 ntX = (v >> 10) & 0x1;
	uint8 ntY = (v >> 11) & 0x1;
	uint8 fineY = (v >> 12) & 0x7;
	
	BString s;

	s.SetToFormat("$%04X", ntBase);
	drawLeftKV("NT Base:", s.String());

	s.SetToFormat("$%04X", bgPT);
	drawLeftKV("BG PT:", s.String());

	if (sprite8x16) {
		drawLeftKV("SPR PT:", "tile bit 0");
	} else {
		s.SetToFormat("$%04X", spritePT);
		drawLeftKV("SPR PT:", s.String());
	}

	drawLeftKV("Sprite:", sprite8x16 ? "8x16" : "8x8");
	drawLeftKV("NMI:", nmiEnabled ? "on" : "off");

	s.SetToFormat("%u", static_cast<unsigned>(coarseX));
	drawRightKV("Coarse X:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(coarseY));
	drawRightKV("Coarse Y:", s.String());

	s.SetToFormat(
		"%u / %u",
		static_cast<unsigned>(ntX),
		static_cast<unsigned>(ntY)
	
	);
	drawRightKV("NT X/Y:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(fineY));
	drawRightKV("Fine Y:", s.String());
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawMaskPanel
//
// Decodes PPUMASK into rendering enable state, left-edge clipping,
// monochrome/grayscale mode, and color emphasis bits.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawMaskPanel()
{
	BRect panel(
		4.0f,
		412.0f,
		Bounds().right - 4.0f,
		520.0f
	);

	::DrawDebugPanel(this, panel, "Mask / Render State");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	uint8 mask = nes::ppu::ppumask();

	bool mono = (mask & 0x1) != 0;
	bool showBgLeft = (mask & 0x2) != 0;
	bool showSpritesLeft = (mask & 0x4) != 0;
	bool showBg = (mask & 0x8) != 0;
	bool showSprites = (mask & 0x10) != 0;

	bool emphR = (mask & 0x20) != 0;
	bool emphG = (mask & 0x40) != 0;
	bool emphB = (mask & 0x80) != 0;

	BString emphasis;

	if (!emphR && !emphG && !emphB) {
		emphasis.SetTo("none");
	} else {
		emphasis.SetTo("");

		if (emphR) {
			emphasis.Append("R");
		}

		if (emphG) {
			emphasis.Append("G");
		}

		if (emphB) {
			emphasis.Append("B");
		}
	}

	drawLeftKV("BG:", showBg ? "on" : "off");
	drawLeftKV("Sprites:", showSprites ? "on" : "off");
	drawLeftKV("Mono:", mono ? "on" : "off");
	drawLeftKV("Emph:", emphasis.String());

	drawRightKV("BG Left:", showBgLeft ? "show" : "hide");
	drawRightKV("SPR Left:", showSpritesLeft ? "show" : "hide");
	
	BString s;
	
	s.SetToFormat("$%02X", mask);
	drawRightKV("PPUMASK:", s.String());
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawStatusPanel
//
// Decodes PPUSTATUS into the three visible status flags exposed by register
// $2002: sprite overflow, sprite 0 hit, and vblank.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawStatusPanel()
{
	BRect panel(
		4.0f,
		532.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Status Flags");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftState = [&](const char *label, const char *value, bool active) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetStateColor(this, active);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	uint8 status = nes::ppu::ppustatus();

	bool overflow = (status & 0x20) != 0;
	bool sprite0 = (status & 0x40) != 0;
	bool vblank = (status & 0x80) != 0;

	drawLeftState("Overflow:", overflow ? "set" : "clear", overflow);
	drawLeftState("Sprite 0:", sprite0 ? "hit" : "clear", sprite0);
	drawLeftState("VBlank:", vblank ? "set" : "clear", vblank);
	
	BString s;

	s.SetToFormat("$%02X", status);
	drawRightKV("Raw:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>((status >> 5) & 0x1));
	drawRightKV("Bit 5:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>((status >> 6) & 0x1));
	drawRightKV("Bit 6:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>((status >> 7) & 0x1));
	drawRightKV("Bit 7:", s.String());
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawTimingPanel
//
// Draws the current PPU render position.  This includes the current dot,
// scanline, and a decoded frame-region label.  This is useful when debugging
// vblank timing, sprite 0 hits, split scrolling, and mid-frame register writes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawTimingPanel()
{
	BRect panel(
		4.0f,
		190.0f,
		Bounds().right - 4.0f,
		258.0f
	);

	::DrawDebugPanel(this, panel, "Timing / Position");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	uint16 dot = nes::ppu::ppu_dot();
	uint16 scanline = nes::ppu::ppu_scanline();

	const char *region = "unknown";
	bool activeRender = false;
	bool vblank = false;

	if (scanline <= 239) {
		region = "visible";
		activeRender = true;
	} else if (scanline == 240) {
		region = "post-render";
	} else if (scanline >= 241 && scanline <= 260) {
		region = "vblank";
		vblank = true;
	} else if (scanline == 261) {
		region = "pre-render";
	}
	
	BString s;

	s.SetToFormat("%u", static_cast<unsigned>(dot));
	drawLeftKV("Dot:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(scanline));
	drawLeftKV("Scanline:", s.String());

	SetHighColor(80, 80, 80);
	DrawString("Region:", BPoint(rightLabelX, rightY));

	if (activeRender) {
		SetHighColor(0, 120, 0);
	} else if (vblank) {
		SetHighColor(0, 80, 160);
	} else {
		SetHighColor(120, 120, 120);
	}

	DrawString(region, BPoint(rightValueX, rightY));
	rightY += lineH;
}


// -----------------------------------------------------------------------------
// PPUStatusView::HasROMLoaded
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
PPUStatusView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the PPU Status window is opened
// without a loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::DrawNoROMMessage (BRect panel)
{
	BFont oldFont;
	GetFont(&oldFont);

	BFont font = oldFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char* title = "No ROM loaded";
	const char* detail = "Load a cartridge to inspect PPU state.";

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

	SetFont(&oldFont);
}



