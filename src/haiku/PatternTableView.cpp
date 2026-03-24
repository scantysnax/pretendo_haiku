
#include "PatternTableView.h"
#include "CHRExplorerView.h"
#include "DebugHelpers.h"


PatternTableView::PatternTableView (BRect frame,
                                   PretendoWindow *mainWindow,
                                   int32 which,
                                   CHRExplorerView *explorer)
	: BView(frame, "pattern_table",  B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
{
	fMainWindow = mainWindow;
	fWhichPatternTable = which;
	fCHRExplorer = explorer;
	fPalette = fMainWindow ? fMainWindow->Palette() : nullptr;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

	fTileSize = 16;

	int32 w = fTileSize * 16;
	int32 h = fTileSize * 16;

	fBitmap = new BBitmap(BRect(0, 0, w - 1, h - 1), B_CMAP8);
	fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
	fRowBytes = fBitmap->BytesPerRow();

	fHoverTileIndex = -1;
	fLockedTileIndex = -1;
	fTileLocked = false;
	fMouseValid = false;
}


PatternTableView::~PatternTableView()
{
	delete fBitmap;
}


void
PatternTableView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
	MakeFocus(true);

	if (fCHRExplorer && fPalette) {
		fCHRExplorer->SetHostPalette(fPalette);
	}
}


void
PatternTableView::Draw(BRect updateRect)
{
	(void)updateRect;

	if (Show8x16()) {
		DrawPatternTable8x16(fWhichPatternTable);
	} else {
		DrawPatternTable8x8(fWhichPatternTable);
	}

	DrawBitmap(fBitmap, BPoint(0, 0));

	PushState();
	ConstrainClippingRegion(nullptr);
	DrawOverlays();
	PopState();
}


void
PatternTableView::Pulse()
{
	if (!fTileLocked) {
		return;
	}

	UpdateExplorer();
	NotifyCHRExplorer();
	Invalidate();
}


void
PatternTableView::MouseMoved(BPoint where, uint32 transit, const BMessage *msg)
{
	(void)msg;

	if (transit == B_EXITED_VIEW) {
		fMouseValid = false;

		if (!fTileLocked && fHoverTileIndex != -1) {
			fHoverTileIndex = -1;

			if (fCHRExplorer) {
				fCHRExplorer->Clear();
			}

			Invalidate();
		}
		return;
	}

	fLastMouse = where;
	fMouseValid = true;

	if (fTileLocked) {
		return;
	}

	int32 index = TileIndexFromPoint(where);

	if (index < 0) {
		if (fHoverTileIndex != -1) {
			fHoverTileIndex = -1;

			if (fCHRExplorer) {
				fCHRExplorer->Clear();
			}

			Invalidate();
		}
		return;
	}

	if (Show8x16()) {
		index &= ~0x1;
	}

	if (index == fHoverTileIndex) {
		return;
	}

	fHoverTileIndex = index;

	UpdateExplorer();
	Invalidate();
}


void
PatternTableView::MouseDown(BPoint where)
{
	MakeFocus(true);
	
	int32 index = TileIndexFromPoint(where);
	if (index < 0)
		return;

	if (Show8x16())
		index &= ~0x1;

	if (fTileLocked && fLockedTileIndex == index) {
		fTileLocked = false;
		fLockedTileIndex = -1;
		fHoverTileIndex = index;
	} else {
		fTileLocked = true;
		fLockedTileIndex = index;
		fHoverTileIndex = index;
	}

	UpdateExplorer();
	Invalidate();
}


void
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	// make sure we can draw
	if (!fBits || !fBitmap) {
		return;
	}

	*(uint8 *)(fBits+x+(y*fRowBytes)) = color;
}



// draw a single tile from a pattern table into the pattern-table debug view
//
// patternTable = 0 or 1 selecting:
//      0: pattern table at $0000
//      1: pattern table at $1000
//
// tileIndex = which 8x8 tile inside that table (0-255 per table)
// tileX/Y   = where to place it in the debug grid (in tiles)

void
PatternTableView::DrawTile (uint32 patternTable, int32 tileIndex, int32 tileX, int32 tileY)
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper || !fBits || !fBitmap || !fPalette) {
		return;
	}

	tileIndex &= 0xff;

	// scale based on view tile size (8->1x, 16->2x, etc.)
	int32 scale = (fTileSize > 0) ? (fTileSize / 8) : 1;
	if (scale < 1) {
		return;
	}

	// build a stable 4-color debug palette from BG palette 0
	uint8 pal[4];
	pal[0] = fPalette[mapper->read_vram(0x3f00) & 0x3f];
	pal[1] = fPalette[mapper->read_vram(0x3f01) & 0x3f];
	pal[2] = fPalette[mapper->read_vram(0x3f02) & 0x3f];
	pal[3] = fPalette[mapper->read_vram(0x3f03) & 0x3f];

	// tile address in chr ($0000 or $1000)
	uint32 const tileAddr = ((patternTable & 1) << 12) + (tileIndex * 16);

	// Destination top-left in bitmap space
	int32 dstX0 = tileX * 8 * scale;
	int32 dstY0 = tileY * 8 * scale;

	int32 bmpW = fBitmap->Bounds().Width() + 1;
	int32 bmpH = fBitmap->Bounds().Height() + 1;

	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = mapper->read_vram(tileAddr + y + 0);
		uint8 secondPlane = mapper->read_vram(tileAddr + y + 8);

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 1;
			pixel |= ((secondPlane >> shift) & 1) << 1;
			uint8 const color = pal[pixel];

			const int32 px = dstX0 + x * scale;
			const int32 py = dstY0 + y * scale;

			// Draw ×scale block (fast path for scale==2 is optional)
			if (px >= 0 && py >= 0 && (px + scale - 1) < bmpW && (py + scale - 1) < bmpH) {
				for (int32 dy = 0; dy < scale; dy++) {
					for (int32 dx = 0; dx < scale; dx++) {
						DrawPixel(px + dx, py + dy, color);
					}
				}
			}
		}
	}
}


// draw an entire pattern table as a 16x16 grid of 8x8 tiles.
//
// which = 0 or 1 selecting:
//   0: pattern table at $0000
//   1: pattern table at $1000
//
// each pattern table contains 256 tiles total
// we visualize them in a 16x16 grid purely for readability
void
PatternTableView::DrawPatternTable8x8 (int32 which)
{	
    // loop over the rows of tiles in the viewer grids
    // 16 rows * 16 columns = 256 tiles (entire pattern table)
	for (int32 y = 0; y < 16; y++) {
        // loop over columns in this row
		for (int32 x = 0; x < 16; x++) {
			// compute which tile index we are drawing
            //
            // pattern tables are stored linearly in chr rom:
            //   tile 0, tile 1, tile 2, ... tile 255
            //
            // we remap that linear order into a 16x16 2D grid:
            //
            //   index = x + (y * 16)
			DrawTile(which, x + (y * 16), x, y);
		}
	}
}


// draw the pattern table arranged the way the ppu uses it in 8x16 sprite mode
//
// in 8x16 mode, each sprite is made of 2 stacked tiles:
//   top tile = even index
//   bottom tile = next odd index
//
// the nes does not treat these as independent tiles. they are always paired
// this function rearranges the linear chr to reflect that pairing
void
PatternTableView::DrawPatternTable8x16 (int32 which)
{
    // x,y here are viewer coordinates (tile grid position),
    // not chr rom coordinates.
	int32 x = 0;
	int32 y = 0;

    // there are 256 tiles total
    // in 8x16 mode they form 128 sprite pairs
    //
    // we draw them as 8 rows of 32 tiles (16 per row)
	for (int32 i = 0; i < 8; i++) {
        // each row processes 32 tiles = 16 pairs
        //
        // (i * 32) selects the start of this row in chr rom
        // we advance by 2 because each sprite consumes two tiles
		for (int32 t = (i * 32); t < ((i * 32) + 32); t += 2) {

            // draw the top half of the sprite (even tile index)
			DrawTile(which, t, x, y);

            // draw the bottom half (odd tile index) directly beneath it
            // in 8x16 mode, the ppu *always* fetches tiles this way
			DrawTile(which, (t + 1), x, (y + 1));

            // Move to the next sprite column.
			x++;
		}

        // after finishing a row of sprites:
        // advance down by 2 because each sprite is 16 pixels tall (2 tiles)
		y += 2;

        // reset to the left side for the next row.
		x = 0;
	}
}


void
PatternTableView::DrawExplorer (BView *dest)
{
	if (!fExploreEnabled) {
		return;
	}

	dest->SetHighColor(255,255,255);
	dest->SetLowColor(0,0,0);

	char line[256];

	sprintf(line, "CHR $%04X", (unsigned)fCHRTileAddress);
	dest->DrawString(line, BPoint(8, 140));

	// raw bytes
	for (int32 i = 0; i < 16; i++) {
		sprintf(line, "%02X", fCHRBytes[i]);
		dest->DrawString(line, BPoint(8 + (i % 8) * 20, 160 + (i / 8) * 14));
	}
}


BPoint
PatternTableView::ViewToBitmap (BPoint where) const
{
	return where;
}


bool
PatternTableView::ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const
{
	if (!fBitmap) {
		return false;
	}

	// vsible viewport size
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


void
PatternTableView::NotifyCHRExplorer()
{
	if (!fCHRExplorer) {
		return;
	}

	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		fCHRExplorer->Clear();
		return;
	}

	int32 index = ActiveTileIndex();
	if (index < 0) {
		fCHRExplorer->Clear();
		return;
	}

	if (!Show8x16()) {
		fCHRExplorer->SetTile(
			fWhichPatternTable,
			index,
			fTileLocked,
			fCHRTileAddress,
			fCHRBytes,
			0,
			-1,
			0,
			0,
			0
		);
		return;
	}

	int32 topIndex = index & ~0x1;
	uint32 topAddr = (fWhichPatternTable ? 0x1000 : 0x0000) + (topIndex * 16);
	uint32 bottomAddr = topAddr + 16;

	uint8 topBytes[16];
	uint8 bottomBytes[16];

	for (int32 i = 0; i < 16; i++) {
		topBytes[i] = mapper->read_vram(topAddr + i);
		bottomBytes[i] = mapper->read_vram(bottomAddr + i);
	}

	fCHRExplorer->SetTile8x16(
		fWhichPatternTable,
		topIndex,
		fTileLocked,
		topAddr,
		topBytes,
		bottomAddr,
		bottomBytes,
		0
	);
}


int32
PatternTableView::TileIndexFromPoint (BPoint where) const
{
	float tileSize = static_cast<float>(fTileSize);
	float tableSize = tileSize * 16.0f;

	if (where.x < 0.0f || where.y < 0.0f || where.x >= tableSize || where.y >= tableSize) {
		return -1;
	}

	if (!Show8x16()) {
		int32 tx = static_cast<int32>(where.x / tileSize);
		int32 ty = static_cast<int32>(where.y / tileSize);

		if (tx < 0 || tx >= 16 || ty < 0 || ty >= 16)
			return -1;

		return tx + (ty * 16);
	}

	int32 cx = static_cast<int32>(where.x / tileSize);
	int32 cy = static_cast<int32>(where.y / (tileSize * 2.0f));

	if (cx < 0 || cx >= 16 || cy < 0 || cy >= 8) {
		return -1;
	}

	int32 within = static_cast<int32>((where.y - (cy * tileSize * 2.0f)) / tileSize);
	int32 pair = cx + cy * 16;

	if (within < 0) {
		within = 0;
	}
	
	if (within > 1) {
		within = 1;
	}

	return pair * 2 + within;
}


BRect
PatternTableView::CellRectForTileIndex (int32 index) const
{
	float tileSize = static_cast<float>(fTileSize);

	index &= 0xff;

	if (!Show8x16()) {
		int32 tx = index % 16;
		int32 ty = index / 16;

		return BRect(
			tx * tileSize,
			ty * tileSize,
			((tx + 1) * tileSize) - 1.0f,
			((ty + 1) * tileSize) - 1.0f
		);
	}

	index &= ~0x1;

	int32 pair = index / 2;
	int32 tx = pair % 16;
	int32 ty = pair / 16;

	return BRect(
		tx * tileSize,
		ty * (tileSize * 2.0f),
		((tx + 1) * tileSize) - 1.0f,
		((ty + 1) * (tileSize * 2.0f)) - 1.0f
	);
}


void
PatternTableView::DrawOverlays()
{
	static const rgb_color kHoverStroke = {  0, 255, 255, 255 };
	static const rgb_color kHoverFill   = {  0, 255, 255,  48 };

	static const rgb_color kLockStroke  = { 255,   0, 255, 255 };
	static const rgb_color kLockFill    = { 255,   0, 255,  48 };

	if (fTileLocked && fLockedTileIndex >= 0) {
		BRect r = CellRectForTileIndex(fLockedTileIndex & 0xff);
		::FillAndStrokeRectTriple(this, r, kLockFill, kLockStroke);
		return;
	}

	if (fHoverTileIndex >= 0) {
		BRect r = CellRectForTileIndex(fHoverTileIndex & 0xff);
		::FillAndStrokeRectTriple(this, r, kHoverFill, kHoverStroke);
	}
}


void
PatternTableView::SetViewMode(view_mode vm)
{
	if (fViewMode == vm) {
		return;
	}

	fViewMode = vm;

	// in 8x16 mode the top tile must be even
	if (Show8x16()) {
		if (fHoverTileIndex >= 0)
			fHoverTileIndex &= ~0x1;

		if (fLockedTileIndex >= 0) {
			fLockedTileIndex &= ~0x1;
		}
	}

	// prevent stale explorer data when switching modes
	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	UpdateExplorer();
	NotifyCHRExplorer();
	
	Invalidate();
}


int32
PatternTableView::ActiveTileIndex() const
{
	if (fTileLocked) {
		return fLockedTileIndex;
	}

	return fHoverTileIndex;
}


void
PatternTableView::UpdateExplorer()
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return;
	}

	if (!fCHRExplorer) {
		return;
	}

	int32 index = ActiveTileIndex();
	
	if (index < 0) {
		fCHRExplorer->Clear();
		return;
	}

	uint32 base = fWhichPatternTable ? 0x1000 : 0x0000;
	uint32 addr = base + (index * 16);

	fCHRTileAddress = addr;

	for (int32 i = 0; i < 16; i++) {
		fCHRBytes[i] = mapper->read_vram(addr + i);
	}

	NotifyCHRExplorer();
}


void
PatternTableView::DrawHoverBox (int32 tileIndex)
{
	if (tileIndex < 0) {
		return;
	}

	BRect r = CellRectForTileIndex(tileIndex);

	SetHighColor(255, 0, 0);
	StrokeRect(r);
}

