
#include "NameTableView.h"
#include "PretendoWindow.h"
#include "CHRExplorerView.h"

#include "Mapper.h"
#include "Ppu.h"

#include "DebugHelpers.h"

#include <cstdio>
#include <algorithm>


static inline uint32
NameTableBaseFromIndex (int32 which)
{	
	return 0x2000 + (which & 0x3) * 0x400;
}


NameTableView::NameTableView(BRect frame, PretendoWindow *mainWindow, int32 which, CHRExplorerView *explorer)
   : BView(frame, "name_table_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE),
			fMainWindow(mainWindow),
			fCHRExplorer(explorer),
			fWhichNameTable(which),
			fFollowViewport(true)
{
	fPalette = fMainWindow ? fMainWindow->Palette() : nullptr;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

    //MakeFocus(true);
}


NameTableView::~NameTableView()
{
	delete fBitmap;
	
	fBitmap = nullptr;
	fBits = nullptr;
	fRowBytes = 0;
}


void
NameTableView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// don't steal focus here; take focus on click in MouseDown().
	// MakeFocus(true);

	// allocate bitmap once
	if (!fBitmap) {
		fBitmap = new BBitmap(BRect(0, 0, WIDTH - 1, HEIGHT - 1), B_CMAP8);
		fBits = reinterpret_cast<uint8*>(fBitmap->Bits());
		fRowBytes = fBitmap->BytesPerRow();
	}

	if (fBits) {
		memset(fBits, 0x0, fBitmap->BitsLength());
	}
	
	// set default hover so explorer isn't blank
	if (fHoverTileX < 0 || fHoverTileY < 0) {
		fHoverTileX = 0;
		fHoverTileY = 0;
	}

	// update explorer state
	UpdateExplorer();
	MaybeUpdateCHRExplorer();

	// reliable hover events
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	// set host palette for the explorer
	if (fCHRExplorer && fPalette) {
		fCHRExplorer->SetHostPalette(fPalette);
	}
}


void
NameTableView::Pulse()
{
	if (fFreezeUpdates) {
		return;
	}
	
	if (!fMainWindow) {
		return;
	}

	uint32 x = 0;
	uint32 y = 0;
	uint32 frameId = 0;

	if (!fMainWindow->GetLatchedScroll(x, y, frameId)) {
		return;
	}

	if (frameId == fLastPPUFrame)
		return;

	fLastPPUFrame = frameId;
	fScrollX = x % 512;
	fScrollY = y % 480;

	Invalidate();
}


void
NameTableView::Draw (BRect updateRect)
{
	// draw things in the right order
	
	(void)updateRect;

	uint32 x = 0;
	uint32 y = 0;
	uint32 frameId = 0;

	if (fMainWindow) {
		fMainWindow->GetLatchedScroll(x, y, frameId);
	}

	fScrollX = x % 512;
	fScrollY = y % 480;

	if (fFollowViewport) {
		DrawScrolledViewport(fScrollX, fScrollY);
	} else {
		DrawNameTable(fWhichNameTable);
	}

	DrawBitmap(fBitmap, BPoint(0, 0));

	PushState();
	ConstrainClippingRegion(nullptr);

	if (fShowAttributeMap) {
		DrawAttributeQuadrantOverlay();
	}

	if (fShowAttributeGrid) {
		DrawAttributeGrid();
	}
		
	DrawMatchingTileOverlay();
	DrawMatchingTileLegend();
	DrawHelpHUD();
	DrawTileInfoHUD();
	DrawFreezeBadge();
	
	if (fFollowViewport) {
		DrawPPUViewportOverlay();
	}

	DrawOverlays();

	PopState();
}


void
NameTableView::MouseMoved (BPoint where, uint32 transit, const BMessage *msg)
{
	(void)msg;
	
	// lets get focus.
	if (transit == B_ENTERED_VIEW) {
		MakeFocus(true);
	}

	if (transit == B_EXITED_VIEW) {
		if (!fTileLocked) {
			if (fHoverTileX != -1 || fHoverTileY != -1) {
				fHoverTileX = -1;
				fHoverTileY = -1;

				if (fCHRExplorer) {
					fCHRExplorer->Clear();
				}

				Invalidate();
			}
		}
		return;
	}

	if (fTileLocked) {
		return;
	}

	int32 x, y;
	
	if (!ComputeTileFromViewPoint(where, x, y)) {
		if (fHoverTileX != -1 || fHoverTileY != -1) {
			fHoverTileX = -1;
			fHoverTileY = -1;

			if (fCHRExplorer)
				fCHRExplorer->Clear();

			Invalidate();
		}
		return;
	}

	if (x == fHoverTileX && y == fHoverTileY) {
		return;
	}

	fHoverTileX = x;
	fHoverTileY = y;

	UpdateExplorer();
	
	Invalidate();
}


void
NameTableView::MouseDown(BPoint where)
{
	MakeFocus(true);

	int32 x, y;
	
	if (!ComputeTileFromViewPoint(where, x, y)) {
		return;
	}

	uint32 mods = modifiers();

#if 0
	uint32 buttons;
	GetMouse(&where, &buttons, false);
#endif

	bool screenLock = (mods & B_SHIFT_KEY) != 0;

	if (fTileLocked) {
		if (fLockToScreen == screenLock) {
			if (screenLock) {
				// shift-click again: unlock screen lock
				fTileLocked = false;
				fLockToScreen = false;
				fLockedTileX = -1;
				fLockedTileY = -1;
			} else if (fLockedTileX == x && fLockedTileY == y) {
				// click same world tile again: unlock
				fTileLocked = false;
				fLockToScreen = false;
				fLockedTileX = -1;
				fLockedTileY = -1;
			} else {
				// change locked world tile
				fLockedTileX = x;
				fLockedTileY = y;
			}
		} else {
			// switch lock mode
			fLockToScreen = screenLock;
			if (screenLock) {
				fLockedViewPoint = where;
			} else {
				fLockedTileX = x;
				fLockedTileY = y;
			}
		}
	} else {
		fTileLocked = true;
		fLockToScreen = screenLock;

		if (screenLock) {
			fLockedViewPoint = where;
		} else {
			fLockedTileX = x;
			fLockedTileY = y;
		}
	}

	fHoverTileX = x;
	fHoverTileY = y;

	UpdateExplorer();
	
	Invalidate();
}


void
NameTableView::KeyDown (const char *bytes, int32 numBytes)
{
	(void)numBytes;
	
	// until we get a proper ui
	
	switch (bytes[0]) {
	case 'g':
	case 'G':
		fShowAttributeGrid = !fShowAttributeGrid;
		break;

	case 'h':
	case 'H':
		fShowAttributeMap = !fShowAttributeMap;
		break;

	case 'b':
	case 'B':
		fShowAttributeGrid = true;
		fShowAttributeMap = true;
		break;

	case 'n':
	case 'N':
		fShowAttributeGrid = false;
		fShowAttributeMap = false;
		break;

	case 'm':
	case 'M':
		fShowMatchingTiles = !fShowMatchingTiles;
		break;

	case 'i':
	case 'I':
		fShowTileInfoHUD = !fShowTileInfoHUD;
		break;
		
	case 'v':
	case 'V':
		fShowViewportBox = !fShowViewportBox;
		break;
		
	case 'p':
	case 'P':
		fFollowViewport = !fFollowViewport;
		break;
		
	case '?':
		fShowHelpHUD = !fShowHelpHUD;
		break;
		
	case ' ':
		fFreezeUpdates = !fFreezeUpdates;
		break;

	default:
		BView::KeyDown(bytes, numBytes);
		return;
	}

 	Invalidate();
}


void
NameTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	// make sure we can draw
	if (!fBits || !fBitmap) {
		return;
	}
	
	// make sure we should draw
	if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) {
		return;
	}

	*(uint8 *)(fBits+x+y*fRowBytes) = color;
}


BPoint
NameTableView::ViewToBitmap (BPoint where) const
{
	float x = where.x + static_cast<float>(fScrollX);
	float y = where.y + static_cast<float>(fScrollY);

	while (x < 0.0f) {
		x += 512.0f;
	}
	
	while (y < 0.0f) {
		y += 480.0f;
	}

	while (x >= 512.0f) {
		x -= 512.0f;
	}
	
	while (y >= 480.0f) {
		y -= 480.0f;
	}

	return BPoint(x, y);
}


bool
NameTableView::ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const
{
	if (!fBitmap) {
		return false;
	}

	// local viewport size
	if (where.x < 0.0f || where.y < 0.0f || where.x >= 256.0f || where.y >= 240.0f) {
		return false;
	}

	BPoint point = ViewToBitmap(where);

	outTX = static_cast<int32>(point.x) >> 3;   // 0-63
	outTY = static_cast<int32>(point.y) >> 3;   // 0-59

	if (outTX < 0 || outTX >= 64 || outTY < 0 || outTY >= 60) {
		return false;
	}

	return true;
}


uint32
NameTableView::PatternBase() const
{
    // background pattern table select is PPUCTRL bit 4:
    // 	0: $0000
    //	1: $1000

	uint8 const ppuctrl = nes::ppu::ppuctrl();

	return (ppuctrl & 0x10) << 8; // ? 0x1000 : 0x0000;
}


void
NameTableView::DrawNameTable(int32 which)
{
	Mapper* mapper = nes::cart.mapper();
	if (!mapper)
		return;

	// keep selection in sync for explorer
	fWhichNameTable = which;

	// address of nametable being viewed ($2000/$2400/$2800/$2C00)
	uint32 const baseAddr = 0x2000 + (which * 0x400);
	fCurrentNameTableBase = baseAddr;

	// background pattern table address ($0000 or $1000), from PPUCTRL bit 4
	uint32 const patternBase = PatternBase();
	
	// draw tiles
	for (int32 tileY = 0; tileY < 30; tileY++) {
		for (int32 tileX = 0; tileX < 32; tileX++) {
			uint32 ntAddr = baseAddr + tileX + (tileY * 32);
			uint8  tileIndex = mapper->read_vram(ntAddr);
			uint8 palette = PaletteForAttribute(baseAddr, tileX, tileY);

			DrawTile(patternBase, tileIndex, tileX, tileY, palette);
		}
	}

	// overlays once
	if (fShowAttributeMap) {
		DrawAttributeMap(baseAddr);
	}

	if (fShowAttributeGrid) {
		DrawAttributeGrid();
	}

    // update explorer fields for the currently selected tile
    // NOTE: Do not overwrite fHoverTileX/Y here every frame.
    // MouseMoved/MouseDown should own hover/lock selection.

	int32 x = fTileLocked ? fLockedTileX : fHoverTileX;
	int32 y = fTileLocked ? fLockedTileY : fHoverTileY;

	// if nothing valid yet, pick a stable default, but don't keep forcing it.
	if (x < 0 || y < 0) {
		x = 0;
		y = 0;
	}

    // temporarily point UpdateExplorer at selected tile without changing "hover"
	int32 savedHX = fHoverTileX;
	int32 savedHY = fHoverTileY;

	fHoverTileX = x;
	fHoverTileY = y;
	UpdateExplorer();

	fHoverTileX = savedHX;
	fHoverTileY = savedHY;

	if (fShowExplorer) {
		MaybeUpdateCHRExplorer();
	}
}


uint8
NameTableView::ColorForPixel (uint8 palette, uint8 pixel)
{
    Mapper *mapper = nes::cart.mapper();
    
	if (!mapper || !fPalette) {
		return 0;
	}

	// pixel 0 always uses universal background color ($3f00)
	if (pixel == 0) {
		uint8 bg = mapper->read_vram(0x3f00) & 0x3f;
		return fPalette[bg];
	}

	// bg palettes: $3f01..$3f0f (mirrors handled by mapper)
	uint32 addr = 0x3f00 + 1 + (palette * 4) + (pixel - 1);
	uint8 index = mapper->read_vram(addr) & 0x3f;
	
	return fPalette[index];
}


uint8
NameTableView::PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY)
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper)
		return 0;

	// attribute byte address: base + 0x3C0 + (tileY/4)*8 + (tileX/4)
	uint32 attrAddr = nameTableBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
	uint8 attrByte = mapper->read_vram(attrAddr);

    // quadrant inside 4x4 tile region
	uint8 qx = ((tileX % 4) / 2); // 0 or 1
	uint8 qy = ((tileY % 4) / 2); // 0 or 1
	uint8 quadrant = ((qy << 1) | qx); // 0-3
	int32 shift = quadrant * 2;

	return (attrByte >> shift) & 0x3;
}


void
NameTableView::DrawTile (uint32 patternBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette)
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return;
	}

	if (!fBits || !fBitmap) {
		return;
	}

	uint32 tileAddr = patternBase + tileIndex * 16;

	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = mapper->read_vram(tileAddr + y);
		uint8 secondPlane = mapper->read_vram(tileAddr + y + 8);

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			uint8 color = ColorForPixel(palette, pixel);
			
			DrawPixel(x + tileX * 8, y + tileY * 8, color);
		}
	}
}


void
NameTableView::DrawAttributeMap (uint32 nameTableBase)
{
	if (!fBits || !fBitmap)
		return;

	// loud debug colors
	static const uint8 debugColors[4] = { 0xff, 0x14, 0x65, 0x96 };

	for (int32 tileY = 0; tileY < 30; tileY++) {
		for (int tileX = 0; tileX < 32; tileX++) {
			uint8 palette = PaletteForAttribute(nameTableBase, tileX, tileY);
			uint8 color = debugColors[palette % 4];

			int32 tx = tileX * 8;
			int32 ty = tileY * 8;

			for (int32 y = 0; y < 8; y++) {
				for (int32 x = 0; x < 8; x++) {
					DrawPixel(tx + x, ty + y, color);
				}
			}
        }
    }
}


void
NameTableView::DrawAttributeGrid()
{
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	const float width  = 256.0f;
	const float height = 240.0f;

	const float quadStep = 16.0f;
	const float attrStep = 32.0f;

	rgb_color quadColor = {  0, 255, 255, 80 };
	rgb_color attrColor = { 255, 255,   0, 180 };

	// 16x16 quadrant lines
	SetHighColor(quadColor);

	for (float x = quadStep; x < width; x += quadStep) {
		StrokeLine(BPoint(x, 0), BPoint(x, height - 1));
	}

	for (float y = quadStep; y < height; y += quadStep) {
		StrokeLine(BPoint(0, y), BPoint(width - 1, y));
	}

	// 32x32 attribute-byte lines
	SetHighColor(attrColor);

	for (float x = attrStep; x < width; x += attrStep) {
		StrokeLine(BPoint(x, 0), BPoint(x, height - 1));
		StrokeLine(BPoint(x + 1, 0), BPoint(x + 1, height - 1));
	}

	for (float y = attrStep; y < height; y += attrStep) {
		StrokeLine(BPoint(0, y), BPoint(width - 1, y));
		StrokeLine(BPoint(0, y + 1), BPoint(width - 1, y + 1));
	}

	PopState();
}


void
NameTableView::UpdateExplorer()
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper || !fCHRExplorer) {
		return;
	}

	int32 worldTX, worldTY;
	
	if (!ActiveTile(worldTX, worldTY)) {
		fCHRExplorer->Clear();
		return;
	}

	// figure out which nametable the world tile belongs to.
	int32 ntX = worldTX / 32;   // 0-1
	int32 ntY = worldTY / 30;   // 0-1

	int32 tileX = worldTX % 32; // 0-31
	int32 tileY = worldTY % 30; // 0-29

	uint32 nameBase = 0x2000 + (ntY * 2 + ntX) * 0x400;
	fCurrentNameTableBase = nameBase;
	fHoverWhichNameTable = ntY * 2 + ntX;

	// nametable byte
	uint32 tileAddr = nameBase + (tileY * 32) + tileX;
	fHoverTileIndex = mapper->read_vram(tileAddr);

	// attribute byte
	uint32 attrAddr = nameBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
	fHoverAttrAddr = attrAddr;
	fHoverAttrByte = mapper->read_vram(attrAddr);

	uint8 qx = (tileX % 4) / 2;
	uint8 qy = (tileY % 4) / 2;
	uint8 quadrant = (qy << 1) | qx;
	int32 shift = quadrant * 2;

	fHoverPalette = (fHoverAttrByte >> shift) & 0x3;
	fCHRTileAddress = PatternBase() + fHoverTileIndex * 16;

	for (int32 i = 0; i < 16; i++) {
		fCHRBytes[i] = mapper->read_vram(fCHRTileAddress + i);
	}

	NotifyCHRExplorer();
}


void
NameTableView::MaybeUpdateCHRExplorer()
{
	if (!fCHRExplorer) {
		return;
	}

	UpdateExplorer();
}


void
NameTableView::DrawScrolledViewport (int32 scrollX, int32 scrollY)
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper || !fBits) {
		return;
	}

	uint32 patternBase = PatternBase();

	auto wrap480 = [](int y) -> int {
		y %= 480;
		
		if (y < 0) {
			y += 480;
		}
		
		return y;
	};

	for (int32 screenY = 0; screenY < 240; ++screenY) {
		for (int32 screenX = 0; screenX < 256; ++screenX) {

			int32 bgX = (screenX + scrollX) % 512;   // 0-511
			int32 bgY = wrap480(screenY + scrollY);    // 0-479

			int32 ntX = bgX / 256;                     // 0-1
			int32 ntY = bgY / 240;                     // 0-1
			int32 whichNT = ntX + ntY * 2;             // 0-3

			int32 xInNT = bgX % 256;                  // 0-255
			int32 yInNT = bgY % 240;                   // 0-239

			int32 tileX = xInNT / 8;                   // 0-31
			int32 tileY = yInNT / 8;                   // 0-29

			int32 fineX = xInNT % 8;                   // 0-7
			int32 fineY = yInNT % 8;                   // 0-7

			uint32 ntBase = NameTableBaseFromIndex(whichNT);
			uint32 nameAddr = ntBase + tileY * 32 + tileX;
			uint8 tileIndex = mapper->read_vram(nameAddr);

			uint32 attrAddr = ntBase + 0x3c0 + (tileY / 4) * 8 + (tileX / 4);
			uint8 attrByte = mapper->read_vram(attrAddr);

			int32 shift = ((tileY & 2) << 1) | (tileX & 2);
			uint8 palette = (attrByte >> shift) & 0x3;

			uint32 chrAddr = patternBase + (tileIndex * 16);
			uint8 firstPlane = mapper->read_vram(chrAddr + fineY);
			uint8 secondPlane = mapper->read_vram(chrAddr + fineY + 8);

			shift = 7 - fineX;
			uint8 pixel = (firstPlane >> shift) & 0x1; 
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			uint8 color = ColorForPixel(palette, pixel);
			
			DrawPixel(screenX, screenY, color);
		}
	}
}


void
NameTableView::DrawPPUViewportOverlay()
{
	if (!fShowViewportBox) {
		return;
	}

	// 1: ghosted "full viewport" box, always drawn in local space
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(255, 255, 0, 60);

	BRect ghost(0.0f, 0.0f, 255.0f, 239.0f);
	StrokeRect(ghost);

	PopState();

	
	// 2: real wrapped viewport pieces for this nametable window
	int32 worldL = fScrollX;
	int32 worldT = fScrollY;
	int32 worldR = fScrollX + 256;
	int32 worldB = fScrollY + 240;

	for (int32 wrapY = 0; wrapY < 2; wrapY++) {
		for (int32 wrapX = 0; wrapX < 2; wrapX++) {

			int32 pieceL = worldL - wrapX * 512;
			int32 pieceT = worldT - wrapY * 480;
			int32 pieceR = worldR - wrapX * 512;
			int32 pieceB = worldB - wrapY * 480;
			
			// keep things 32-bit friendly
			int32 visL = std::max(pieceL, static_cast<int32>(0));
			int32 visT = std::max(pieceT, static_cast<int32>(0));
			int32 visR = std::min(pieceR, static_cast<int32>(256));
			int32 visB = std::min(pieceB, static_cast<int32>(240));
			
			if (visL >= visR || visT >= visB) {
				continue;
			}

			BRect r(visL, visT,(visR - 1), (visB - 1));

			StrokeRectTriple(this, r, (rgb_color){255, 255, 0, 255});
		}
	}

	// 3: mark viewport origin inside the local wrapped view
	float ox = (fScrollX % 512);
	float oy = (fScrollY % 480);

	while (ox >= 256.0f) {
		ox -= 512.0f;
	}
	
	while (oy >= 240.0f) {
		oy -= 480.0f;
	}

	SetHighColor(255, 255, 0, 255);
	StrokeLine(BPoint(ox - 4, oy), BPoint(ox + 4, oy));
	StrokeLine(BPoint(ox, oy - 4), BPoint(ox, oy + 4));
}


bool
NameTableView::ActiveTile (int32 &outTX, int32 &outTY) const
{
	if (fTileLocked) {
		if (fLockToScreen) {
			return ComputeTileFromViewPoint(fLockedViewPoint, outTX, outTY);
		}

		if (fLockedTileX < 0 || fLockedTileY < 0) {
			return false;
		}

		outTX = fLockedTileX;
		outTY = fLockedTileY;
		return true;
	}

	if (fHoverTileX < 0 || fHoverTileY < 0) {
		return false;
	}

	outTX = fHoverTileX;
	outTY = fHoverTileY;
	
	return true;
}


void
NameTableView::DrawOverlays()
{
	static const rgb_color kHoverStroke      = {  0,255,255,255 };
	static const rgb_color kHoverFill        = {  0,255,255, 48 };

	static const rgb_color kWorldLockStroke  = {255,  0,255,255 };
	static const rgb_color kWorldLockFill    = {255,  0,255, 48 };

	static const rgb_color kScreenLockStroke = {255,255,  0,255 };
	static const rgb_color kScreenLockFill   = {255,255,  0, 48 };

	if (fTileLocked) {
		if (fLockToScreen) {
			BRect r = CellRectForViewPoint(fLockedViewPoint);
			::FillAndStrokeRectTriple(this, r, kScreenLockFill, kScreenLockStroke);
			return;
		}

		if (fLockedTileX >= 0 && fLockedTileY >= 0) {
			BRect r = CellRectForTile(fLockedTileX, fLockedTileY);
			::FillAndStrokeRectTriple(this, r, kWorldLockFill, kWorldLockStroke);
			return;
		}
	}

	if (fHoverTileX >= 0 && fHoverTileY >= 0) {
		BRect r = CellRectForTile(fHoverTileX, fHoverTileY);
		::FillAndStrokeRectTriple(this, r, kHoverFill, kHoverStroke);
	}
}


BRect
NameTableView::CellRectForTile (int32 tileX, int32 tileY) const
{
	float x = (tileX * 8 - fScrollX);
	float y = (tileY * 8 - fScrollY);

	while (x < 0.0f) {
		x += 512.0f;
	}
	
	while (y < 0.0f) {
		y += 480.0f;
	}

	while (x >= 256.0f) {
		x -= 512.0f;
	}
	
	while (y >= 240.0f) {
		y -= 480.0f;
	}

	return BRect(x, y, x + 7.0f, y + 7.0f);
}


BRect
NameTableView::CellRectForViewPoint (BPoint where) const
{
	float x = floorf(where.x / 8.0f) * 8.0f;
	float y = floorf(where.y / 8.0f) * 8.0f;
	
	return BRect(x, y, x + 7.0f, y + 7.0f);
}


void
NameTableView::DrawAttributeQuadrantOverlay()
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper)
		return;

	static const rgb_color kPaletteTint[4] = {
		{255, 128, 128, 72},
		{128, 255, 128, 72},
		{128, 128, 255, 72},
		{255, 255, 128, 72}
	};

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	for (int32 screenY = 0; screenY < 240; screenY += 16) {
		for (int32 screenX = 0; screenX < 256; screenX += 16) {

			int32 bgX = (screenX + fScrollX) % 512;
			int32 bgY = (screenY + fScrollY) % 480;
			
			if (bgY < 0) {
				bgY += 480;
			}

			int32 ntX = bgX / 256;
			int32 ntY = bgY / 240;
			int32 whichNT = ntX + ntY * 2;

			int32 xInNT = bgX % 256;
			int32 yInNT = bgY % 240;

			int32 tileX = xInNT / 8;
			int32 tileY = yInNT / 8;

			uint32 ntBase = NameTableBaseFromIndex(whichNT);
			uint32 attrAddr = ntBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
			uint8 attrByte = mapper->read_vram(attrAddr);

			uint8 qx = (tileX % 4) / 2;
			uint8 qy = (tileY % 4) / 2;
			uint8 quadrant = (qy << 1) | qx;
			int32 shift = quadrant * 2;

			uint8 palette = (attrByte >> shift) & 0x3;

			BRect r(screenX, screenY, screenX + 15.0f, screenY + 15.0f);

			SetHighColor(kPaletteTint[palette]);
			FillRect(r);
		}
	}

	PopState();
}


void
NameTableView::DrawMatchingTileOverlay()
{
	if (!fShowMatchingTiles) {
		return;
	}

	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return;
	}

	int32 worldTX, worldTY;
	
	if (!ActiveTile(worldTX, worldTY)) {
		return;
	}

	int32 selTileX = worldTX % 32;
	int32 selTileY = worldTY % 30;
	int32 selNTX = worldTX / 32;
	int32 selNTY = worldTY / 30;

	uint32 selNameBase = 0x2000 + (selNTY * 2 + selNTX) * 0x400;
	uint32 selAddr = selNameBase + (selTileY * 32) + selTileX;
	uint8 selectedTileIndex = mapper->read_vram(selAddr);

	uint32 selAttrAddr = selNameBase + 0x3c0 + ((selTileY / 4) * 8) + (selTileX / 4);
	uint8 selAttrByte = mapper->read_vram(selAttrAddr);
	uint8 selQX = (selTileX % 4) / 2;
	uint8 selQY = (selTileY % 4) / 2;
	uint8 selQuadrant = (selQY << 1) | selQX;
	int32 shift = (selQuadrant * 2);
	uint8 selectedPalette = (selAttrByte >> shift) & 0x3;

	static const rgb_color kSameTileSamePal = {255, 255, 255, 90};
	static const rgb_color kSameTileDiffPal = {255, 180, 100, 90};

	PushState();
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	for (int32 screenTileY = 0; screenTileY < 30; ++screenTileY) {
		for (int32 screenTileX = 0; screenTileX < 32; ++screenTileX) {

			int32 screenX = screenTileX * 8;
			int32 screenY = screenTileY * 8;

			int32 bgX = (screenX + fScrollX) % 512;
			int32 bgY = (screenY + fScrollY) % 480;
			
			if (bgY < 0) {
				bgY += 480;
			}

			int32 ntX = bgX / 256;
			int32 ntY = bgY / 240;

			int32 xInNT = bgX % 256;
			int32 yInNT = bgY % 240;

			int32 tileX = xInNT / 8;
			int32 tileY = yInNT / 8;

			uint32 ntBase = NameTableBaseFromIndex(ntX + ntY * 2);
			uint32 nameAddr = ntBase + tileY * 32 + tileX;
			uint8 tileIndex = mapper->read_vram(nameAddr);

			if (tileIndex != selectedTileIndex)
				continue;

			// Skip the actively selected tile itself
			if ((worldTX == (bgX >> 3)) && (worldTY == (bgY >> 3)))
				continue;

			uint32 attrAddr = ntBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);

			uint8 attrByte = mapper->read_vram(attrAddr);
			uint8 qx = (tileX % 4) / 2;
			uint8 qy = (tileY % 4) / 2;
			uint8 quadrant = (qy << 1) | qx;
			int32 shift = (quadrant * 2);
			uint8 palette = (attrByte >> shift) & 0x3;

			BRect r(screenX, screenY, screenX + 7.0f, screenY + 7.0f);

			if (palette == selectedPalette) {
				SetHighColor(kSameTileSamePal);
			} else {
				SetHighColor(kSameTileDiffPal);
			}

			StrokeRect(r);
		}
	}

	PopState();
}


void
NameTableView::DrawMatchingTileLegend()
{
	if (!fShowMatchingTiles) {
		return;
	}

	float const x = 6.0f;
	float const y = 190.0f;

	static rgb_color const kSameTileSamePalStroke = {255,255,255,200};
	static rgb_color const kSameTileSamePalFill   = {255,255,255,48};

	static rgb_color const kSameTileDiffPalStroke = {255,128,0,200};
	static rgb_color const kSameTileDiffPalFill   = {255,128,0,48};

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	BRect r1(x, y, x + 8, y + 8);
	::FillAndStrokeRectTriple(this, r1,
		kSameTileSamePalFill,
		kSameTileSamePalStroke);

	SetHighColor(255,255,255,255);
	DrawString("same tile / same palette", BPoint(x + 14, y + 8));

	BRect r2(x, y + 14, x + 8, y + 22);
	::FillAndStrokeRectTriple(this, r2,
		kSameTileDiffPalFill,
		kSameTileDiffPalStroke);

	SetHighColor(255,255,255,255);
	DrawString("same tile / different palette", BPoint(x + 14, y + 22));

	PopState();
}


void
NameTableView::DrawTileInfoHUD()
{
	if (!fShowTileInfoHUD) {
		return;
	}

	int32 worldTX, worldTY;
	
	if (!ActiveTile(worldTX, worldTY)) {
		return;
	}

	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return;
	}

	int32 ntX = worldTX / 32;
	int32 ntY = worldTY / 30;
	int32 whichNT = ntX + ntY * 2;

	int32 tileX = worldTX % 32;
	int32 tileY = worldTY % 30;

	uint32 nameBase = 0x2000 + (whichNT * 0x400);
	uint32 tileAddr = nameBase + (tileY * 32) + tileX;
	uint8 tileIndex = mapper->read_vram(tileAddr);

	uint32 attrAddr = nameBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
	uint8 attrByte = mapper->read_vram(attrAddr);

	uint8 qx = (tileX % 4) / 2;
	uint8 qy = (tileY % 4) / 2;
	uint8 quadrant = (qy << 1) | qx;
	int32 shift = (quadrant * 2);
	uint8 palette = (attrByte >> shift) & 0x03;

	uint32 chrAddr = PatternBase() + tileIndex * 16;

	const char* lockText = "HOVER";
	
	if (fTileLocked) {
		lockText = fLockToScreen ? "SCREEN LOCK" : "LOCK";
	}

	const char *modeText;
	
	if (fFreezeUpdates) {
		modeText = fFollowViewport ? "VIEW FREEZE" : "RAW FREEZE";
	} else {
		modeText = fFollowViewport ? "VIEW" : "RAW";
	}

	static const char *kQuadrantNames[4] = {
		"TL", "TR", "BL", "BR"
	};

	char line1[256];
	char line2[256];
	char line3[256];

	snprintf(line1, sizeof(line1),
		"%s  %s  W:(%ld,%ld)  NT:%ld  Tile:(%ld,%ld)",
		lockText,
		modeText,
		(long)worldTX, (long)worldTY,
		(long)whichNT,
		(long)tileX, (long)tileY);

	snprintf(line2, sizeof(line2),
		"Index:$%02X  Attr:$%04lX=%02X  Q:%s  Pal:%u",
		(unsigned)tileIndex,
		(unsigned long)attrAddr,
		(unsigned)attrByte,
		kQuadrantNames[quadrant & 3],
		(unsigned)palette);

	snprintf(line3, sizeof(line3),
		"CHR:$%04lX  Scroll:(%ld,%ld)",
		(unsigned long)chrAddr,
		(long)fScrollX, (long)fScrollY);

	const float x = 6.0f;
	const float y = 160.0f;
	const float pad = 4.0f;
	const float lineH = 13.0f;

	float w1 = StringWidth(line1);
	float w2 = StringWidth(line2);
	float w3 = StringWidth(line3);
	float boxW = std::max(w1, std::max(w2, w3)) + pad * 2.0f;
	float boxH = lineH * 3.0f + pad * 2.0f;

	BRect box(x, y, x + boxW, y + boxH);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(0, 0, 0, 160);
	FillRect(box);

	SetDrawingMode(B_OP_COPY);
	SetHighColor(255, 255, 255, 255);
	StrokeRect(box);

	DrawString(line1, BPoint(x + pad, y + pad + 10.0f));
	DrawString(line2, BPoint(x + pad, y + pad + 10.0f + lineH));
	DrawString(line3, BPoint(x + pad, y + pad + 10.0f + lineH * 2.0f));

	PopState();
}


void
NameTableView::NotifyCHRExplorer()
{
	if (!fCHRExplorer) {
		return;
	}

	int32 worldTX, worldTY;
	
	if (!ActiveTile(worldTX, worldTY)) {
		fCHRExplorer->Clear();
		return;
	}

	int32 tileX = worldTX % 32;
	int32 tileY = worldTY % 30;

	uint8 qx = (tileX % 4) / 2;
	uint8 qy = (tileY % 4) / 2;
	uint8 quadrant = (qy << 1) | qx;

	int32 whichPT = (PatternBase() != 0) ? 1 : 0;

	fCHRExplorer->SetTile(
		whichPT,
		fHoverTileIndex,
		fTileLocked,
		fCHRTileAddress,
		fCHRBytes,
		fHoverPalette,
		fHoverWhichNameTable,
		fHoverAttrAddr,
		fHoverAttrByte,
		quadrant
	);
}


void
NameTableView::DrawHelpHUD()
{
	if (!fShowHelpHUD) {
		return;
	}

	char const *line1 = "g grid   h tint   b both   n none";
	char const *line2 = "m matches   i info   v viewport   p raw/view";
	char const *line3 = "space freeze click lock   shift-click screen lock   / help";

	float x = 6.0f;
	float y = 6.0f;
	float pad = 4.0f;
	float lineH = 13.0f;

	float w1 = StringWidth(line1);
	float w2 = StringWidth(line2);
	float w3 = StringWidth(line3);
	float boxW = std::max(w1, std::max(w2, w3)) + pad * 2.0f;
	float boxH = lineH * 3.0f + pad * 2.0f;

	BRect box(x, y, x + boxW, y + boxH);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(0, 0, 0, 140);
	FillRect(box);

	SetDrawingMode(B_OP_COPY);
	SetHighColor(255, 255, 255, 255);
	StrokeRect(box);

	DrawString(line1, BPoint(x + pad, y + pad + 10.0f));
	DrawString(line2, BPoint(x + pad, y + pad + 10.0f + lineH));
	DrawString(line3, BPoint(x + pad, y + pad + 10.0f + lineH * 2.0f));

	PopState();
}


void
NameTableView::DrawFreezeBadge()
{
	if (!fFreezeUpdates) {
		return;
	}

	char const *text = "FROZEN";

	float x = 6.0f;
	float y = 34.0f;
	float pad = 4.0f;

	float textW = StringWidth(text);
	float boxW = textW + pad * 2.0f;
	float boxH = 16.0f;

	BRect box(x, y, x + boxW, y + boxH);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(180, 0, 0, 180);
	FillRect(box);

	SetDrawingMode(B_OP_COPY);
	SetHighColor(255, 255, 255, 255);
	StrokeRect(box);
	DrawString(text, BPoint(x + pad, y + 12.0f));

	PopState();
}


