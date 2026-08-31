
#include "PPUStatusView.h"


// -----------------------------------------------------------------------------
// SetStateColor
//
// Selects the text color used for boolean/status values in the PPU Status view.
//
// Active states are drawn in green so enabled/set conditions stand out clearly.
// Inactive states use a neutral gray.
//
// Parameters:
//   view   - View whose high color should be changed.
//   active - true for the active-state color; false for the inactive color.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
SetStateColor (BView *view, bool active)
{
	if (active) {
		view->SetHighColor(0, 120, 0);
	} else {
		view->SetHighColor(120, 120, 120);
	}
}


// -----------------------------------------------------------------------------
// PPUStatusView::PPUStatusView
//
// Constructs the PPU Status debugger view.
//
// The view continuously presents live PPU register, timing, rendering, and
// scroll state.  The parent argument is retained for consistency with debugger
// view construction but is not currently required by this view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow; currently unused.
//
// Returns:
//   Constructed PPUStatusView instance.
// -----------------------------------------------------------------------------
PPUStatusView::PPUStatusView(BRect frame, PretendoWindow *parent)
	: BView(frame, "ppu_status_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
{
	(void)parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// PPUStatusView::~PPUStatusView
//
// Destroys the PPU Status debugger view.
//
// No additional cleanup is currently required because the view does not own any
// dynamically allocated debugger resources.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PPUStatusView::~PPUStatusView()
{
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


// -----------------------------------------------------------------------------
// PPUStatusView::CaptureSnapshot
//
// Captures the live PPU state represented by one PPU Status redraw.
//
// Registers and scroll state are sampled once so all panels decode the same
// values.  Scanline/dot timing is read with a small stability check so the
// captured dot cannot accidentally belong to the next scanline while the
// captured scanline still belongs to the previous one.
//
// This snapshot is not a freeze feature.  A new snapshot is captured on every
// redraw while a ROM is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::CaptureSnapshot()
{
	fSnapshot.ctrl = nes::ppu::ppuctrl();
	fSnapshot.mask = nes::ppu::ppumask();
	fSnapshot.status = nes::ppu::ppustatus();
	fSnapshot.oamAddr = nes::ppu::oamaddr();
	fSnapshot.scroll = nes::ppu::scroll_state();

	/*
	 * Capture dot/scanline as one stable pair.
	 *
	 * If the PPU crosses a scanline boundary between the first scanline
	 * read and the dot read, repeat until both surrounding scanline reads
	 * agree.
	 */
	uint16 scanlineBefore = 0;
	uint16 scanlineAfter = 0;
	uint16 dot = 0;

	do {
		scanlineBefore = nes::ppu::ppu_scanline();
		dot = nes::ppu::ppu_dot();
		scanlineAfter = nes::ppu::ppu_scanline();
	} while (scanlineBefore != scanlineAfter);

	fSnapshot.scanline = scanlineAfter;
	fSnapshot.dot = dot;
}


// -----------------------------------------------------------------------------
// PPUStatusView::Draw
//
// Draws the complete PPU Status debugger.
//
// The background and header are drawn first.  If no ROM is loaded, a friendly
// empty-state panel is shown.
//
// When a ROM is available, one coherent live PPU snapshot is captured and all
// register, timing, control, mask, and status panels decode that same sample.
//
// Parameters:
//   updateRect - Area of the view requested for redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUStatusView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216, 255);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(4.0f, 74.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
		::DrawDebugPanel(this, panel, "PPU State");
		DrawNoROMMessage(panel);
		return;
	}

	/*
	 * Take exactly one live PPU sample for this redraw.  Every panel below
	 * decodes this same state.
	 */
	CaptureSnapshot();

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
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 62.0f);
	::DrawDebugPanel(this, panel, "PPU Status");

	SetFontSize(11.0f);
	SetHighColor(35, 35, 35);
	DrawString("Live PPU register and render-state summary", BPoint(panel.left + 8.0f, panel.top + 36.0f));
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawRegisterPanel
//
// Draws the raw PPU register and internal scroll-state panel.
//
// PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, Loopy v/t, fine X, and the control
// shadow value are displayed from the snapshot captured for the current redraw.
//
// Register and hexadecimal address values are rendered with a fixed-width font
// for easier visual comparison.
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
	BRect panel(4.0f, 74.0f, Bounds().right - 4.0f, 178.0f);
	::DrawDebugPanel(this, panel, "Registers");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;
	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftLV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(&fixed);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;		
	};

	auto drawRightLV = [&](const char *label, const char *value) {
			SetHighColor(80, 80, 80);
			SetFont(&prevFont);
			DrawString(label, BPoint(rightLabelX, rightY));

			SetHighColor(0, 0, 0);
			SetFont(&fixed);
			DrawString(value, BPoint(rightValueX, rightY));

			rightY += lineH;
		};

	BString s;

	s.SetToFormat("$%02X", fSnapshot.ctrl);
	drawLeftLV("PPUCTRL:", s.String());

	s.SetToFormat("$%02X", fSnapshot.mask);
	drawLeftLV("PPUMASK:", s.String());

	s.SetToFormat("$%02X", fSnapshot.status);
	drawLeftLV("PPUSTATUS:", s.String());

	s.SetToFormat("$%02X", fSnapshot.oamAddr);
	drawLeftLV("OAMADDR:", s.String());

	s.SetToFormat("$%04lX", static_cast<unsigned long>(fSnapshot.scroll.v & 0x7fff));
	drawRightLV("Scroll v:", s.String());

	s.SetToFormat("$%04lX", static_cast<unsigned long>(fSnapshot.scroll.t & 0x7fff));
	drawRightLV("Scroll t:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(fSnapshot.scroll.x));
	drawRightLV("Fine X:", s.String());

	s.SetToFormat("$%02X", fSnapshot.scroll.ctrl);
	drawRightLV("Ctrl sh:", s.String());

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawControlPanel
//
// Decodes PPUCTRL and the current scroll address into human-readable render
// state.
//
// The panel shows the control-selected name table, background and sprite pattern
// table selection, sprite size, NMI enable state, VRAM address increment, and
// the decoded Loopy scroll fields from the snapshot captured for this redraw.
//
// Hexadecimal addresses are displayed using a fixed-width font for easier
// comparison.
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
	BRect panel(4.0f, 270.0f, Bounds().right - 4.0f, 400.0f);
	::DrawDebugPanel(this, panel, "Control / Scroll Decode");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont normal;
	GetFont(&normal);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftLV = [&](const char *label, const char *value, bool fixedValue = false) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetFont(fixedValue ? &fixed : &normal);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};	

	auto drawRightLV = [&](const char *label, const char *value, bool fixedValue = false) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetFont(fixedValue ? &fixed : &normal);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	const uint8 ctrl = fSnapshot.ctrl;
	const uint32 v = fSnapshot.scroll.v & 0x7fff;

	const uint16 ntBase = 0x2000 + ((ctrl & 0x3) * 0x400);
	const uint16 bgPT = (ctrl & 0x10) ? 0x1000 : 0x0000;
	const uint16 spritePT = (ctrl & 0x8) ? 0x1000 : 0x0000;
	
	const bool sprite8x16 = (ctrl & 0x20) != 0;
	const bool nmiEnabled = (ctrl & 0x80) != 0;

	const uint16 vramIncrement = (ctrl & 0x4) ? 32 : 1;

	const uint8 coarseX = v & 0x1f;
	const uint8 coarseY = (v >> 5) & 0x1f;
	const uint8 ntX = (v >> 10) & 0x1;
	const uint8 ntY = (v >> 11) & 0x1;
	const uint8 fineY = (v >> 12) & 0x7;

	BString s;

	s.SetToFormat("$%04X", ntBase);
	drawLeftLV("CTRL NT:", s.String(), true);

	s.SetToFormat("$%04X", bgPT);

	drawLeftLV("BG PT:", s.String(), true);

	if (sprite8x16) {
		drawLeftLV("SPR PT:", "tile bit 0");
	} else {
		s.SetToFormat("$%04X", spritePT);
		drawLeftLV("SPR PT:", s.String(), true);
	}

	drawLeftLV("Sprite:", sprite8x16 ? "8x16" : "8x8");
	drawLeftLV("NMI:", nmiEnabled ? "on" : "off");

	s.SetToFormat("%u", static_cast<unsigned>(coarseX));
	drawRightLV("Coarse X:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(coarseY));
	drawRightLV("Coarse Y:", s.String());

	s.SetToFormat("%u / %u", static_cast<unsigned>(ntX), static_cast<unsigned>(ntY));
	drawRightLV("NT X/Y:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(fineY));
	drawRightLV("Fine Y:", s.String());

	s.SetToFormat("+%u", static_cast<unsigned>(vramIncrement));
	drawRightLV("VRAM Inc:", s.String());

	SetFont(&normal);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawMaskPanel
//
// Decodes PPUMASK into the current rendering state.
//
// The panel shows background and sprite enable state, left-edge rendering,
// grayscale mode, color-emphasis bits, and the raw PPUMASK value captured for
// the current redraw.
//
// The raw hexadecimal register value is displayed using a fixed-width font.
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
	BRect panel(4.0f, 412.0f, Bounds().right - 4.0f, 520.0f);
	::DrawDebugPanel(this, panel, "Mask / Render State");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont normal;
	GetFont(&normal);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftLV = [&](const char *label, const char *value, bool fixedValue = false) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetFont(fixedValue ? &fixed : &normal);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightLV = [&](const char *label, const char *value, bool fixedValue = false) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetFont(fixedValue ? &fixed : &normal);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));
		
		rightY += lineH;
	};

	const uint8 mask = fSnapshot.mask;
	const bool grayscale = (mask & 0x1) != 0;
	const bool showBgLeft = (mask & 0x2) != 0;
	const bool showSpritesLeft = (mask & 0x4) != 0;
	const bool showBg = (mask & 0x8) != 0;
	const bool showSprites = (mask & 0x10) != 0;

	const bool emphR = (mask & 0x20) != 0;
	const bool emphG = (mask & 0x40) != 0;
	const bool emphB = (mask & 0x80) != 0;

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

	drawLeftLV("BG:", showBg ? "on" : "off");
	drawLeftLV("Sprites:", showSprites ? "on" : "off");
	drawLeftLV("Gray:", grayscale ? "on" : "off");
	drawLeftLV("Emph:", emphasis.String());

	drawRightLV("BG Left:", showBgLeft ? "show" : "hide");
	drawRightLV("SPR Left:", showSpritesLeft ? "show" : "hide");

	BString s;

	s.SetToFormat("$%02X", mask);
	drawRightLV("PPUMASK:", s.String(), true);

	SetFont(&normal);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawStatusPanel
//
// Draws and decodes the visible status flags from PPUSTATUS.
//
// Sprite overflow, sprite-0 hit, and VBlank are shown as named states with
// active values highlighted in green.  The complete raw PPUSTATUS value is
// retained on the right for low-level inspection without redundantly repeating
// bits 5-7 as anonymous bit values.
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
	BRect panel(4.0f, 532.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Status Flags");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont normal;
	GetFont(&normal);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 82.0f;

	const float rightLabelX = panel.left + 196.0f;
	const float rightValueX = rightLabelX + 76.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftState = [&](const char *label, const char *value, bool active) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetStateColor(this, active);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightLV = [&](const char *label, const char *value, bool fixedValue = false) {
		SetFont(&normal);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetFont(fixedValue ? &fixed : &normal);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));
		
		rightY += lineH;
	};

	const uint8 status = fSnapshot.status;
	const bool overflow = (status & 0x20) != 0;
	const bool sprite0 = (status & 0x40) != 0;
	const bool vblank = (status & 0x80) != 0;

	drawLeftState("Overflow:", overflow ? "set" : "clear", overflow);
	drawLeftState("Sprite 0:", sprite0 ? "hit" : "clear", sprite0);
	drawLeftState("VBlank:", vblank ? "set" : "clear", vblank);

	BString s;

	s.SetToFormat("$%02X", status);
	drawRightLV("Raw:", s.String(), true);

	SetFont(&normal);
}


// -----------------------------------------------------------------------------
// PPUStatusView::DrawTimingPanel
//
// Draws the current PPU render position from the snapshot captured for this
// redraw.
//
// The panel shows the raw dot and scanline together with decoded vertical and
// horizontal render phases.  The vertical region identifies visible,
// post-render, VBlank, and pre-render scanlines, while the horizontal phase
// identifies visible-pixel, sprite-fetch, background-prefetch, and trailing
// fetch periods within the scanline.
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
	BRect panel(4.0f, 190.0f, Bounds().right - 4.0f, 258.0f);
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

	auto drawLeftLV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightLV = [&](const char *label, const char *value, rgb_color color) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(color);
		DrawString(value, BPoint(rightValueX, rightY));
		
		rightY += lineH;
	};

	const uint16 dot = fSnapshot.dot;
	const uint16 scanline = fSnapshot.scanline;
	const char *region = "unknown";
	rgb_color regionColor = { 120, 120, 120, 255 };

	if (scanline <= 239) {
		region = "visible";
		regionColor = { 0, 120, 0, 255 };
	} else if (scanline == 240) {
		region = "post-render";
	} else if (scanline >= 241 && scanline <= 260) {
		region = "vblank";
		regionColor = { 0, 80, 160, 255 };
	} else if (scanline == 261) {
		region = "pre-render";
	}

	const char *phase = "unknown";

	if (dot == 0) {
		phase = "start";
	} else if (dot <= 256) {
		phase = "visible";
	} else if (dot <= 320) {
		phase = "sprite fetch";
	} else if (dot <= 336) {
		phase = "BG prefetch";
	} else if (dot <= 340) {
		phase = "fetch tail";
	}

	BString s;
	s.SetToFormat("%u", static_cast<unsigned>(dot));
	drawLeftLV("Dot:", s.String());

	s.SetToFormat("%u", static_cast<unsigned>(scanline));
	drawLeftLV("Scanline:", s.String());
	drawRightLV("Region:", region, regionColor);
	drawRightLV("Phase:", phase, rgb_color { 0, 0, 0, 255 });
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

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect PPU state.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(centerX - (StringWidth(title) * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - (StringWidth(detail) * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&oldFont);
}

