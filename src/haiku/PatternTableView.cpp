// -------------------------------------------------------------
// PatternTableView.cpp
//
// Implements the PatternTable debugger view. This view renders either
// NES pattern table as a 256x256 debug bitmap, supports 8x8 and 8x16
// sprite-pair layouts, tracks hover/locked tile selection, and feeds
// the active tile into CHRExplorerView.
// -------------------------------------------------------------

#include "Cart.h"
#include "CHRExplorerView.h"
#include "DebugHelpers.h"
#include "PatternTableView.h"

#include <cmath>
#include <cstring>


// -------------------------------------------------------------
// PatternTableView::PatternTableView
//
// Creates the PatternTable viewer, allocates the backing bitmap,
// records the parent/emulator context, and initializes selection state.
//
// Parameters:
//   frame      - Initial view frame in the owning window.
//   mainWindow - Parent Pretendo window; used for palette access.
//   which      - Pattern table index: 0 for $0000, 1 for $1000.
//   explorer   - Optional CHR explorer view to notify on selection changes.
//
// Returns:
//   Constructed PatternTableView object.
// -------------------------------------------------------------
PatternTableView::PatternTableView (BRect frame,
                                   PretendoWindow *mainWindow,
                                   int32 which,
                                   CHRExplorerView *explorer)
	: BView(frame, "pattern_table",  B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
{
	fMainWindow = mainWindow;
	fWhichPatternTable = which;
	fCHRExplorer = explorer;
	fHostPalette = fMainWindow ? fMainWindow->Palette() : nullptr;

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


// -------------------------------------------------------------
// PatternTableView::~PatternTableView
//
// Releases the pattern-table backing bitmap owned by this view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
PatternTableView::~PatternTableView()
{
	delete fBitmap;
}



// -------------------------------------------------------------
// PatternTableView::AttachedToWindow
//
// Performs final view setup after the view is attached to a window,
// refreshes palette/explorer wiring, and seeds persistent explore mode
// with tile 0 so the explorer is never blank.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
	MakeFocus(true);

	// Refresh palette here, not only in the constructor.
	// On first window open, fMainWindow->Palette() may not have been ready yet.
	if (!fHostPalette && fMainWindow) {
		fHostPalette = fMainWindow->Palette();
	}
	
	if (fCHRExplorer && fHostPalette) {
		fCHRExplorer->SetHostPalette(fHostPalette);
	}

		// Persistent explore mode:
	// Seed a default tile so the CHR explorer is never blank.
	if (fHoverTileIndex < 0) {
		fHoverTileIndex = 0;
	}

	fLockedTileIndex = -1;
	fTileLocked = false;

	if (HasROMLoaded()) {
		UpdateExplorer();
		NotifyCHRExplorer();
	} else if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::Draw
//
// Draws the complete PatternTable UI: background, controls panel,
// bitmap panel, pattern-table bitmap, overlays, viewport border, and
// bottom Pattern State panel. If no ROM is loaded, the normal UI frame
// remains visible and the bitmap area shows a friendly empty-state message.
//
// Parameters:
//   updateRect - Invalidated region supplied by the app_server. The
//                current implementation redraws the full view.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	BPoint origin = BitmapOrigin();

	DrawHeaderUI();
	DrawBitmapPanel(origin);

	if (!HasROMLoaded()) {
		DrawNoROMMessage();
		DrawPatternStatePanel();
		return;
	}

	if (fBitmap && fBits) {
		memset(fBits, 0x00, fBitmap->BitsLength());

		if (fViewMode == MODE_8x16) {
			DrawPatternTable8x16(fWhichPatternTable);
		} else {
			DrawPatternTable8x8(fWhichPatternTable);
		}

		DrawBitmap(fBitmap, origin);
	}

	PushState();
	TranslateBy(origin.x, origin.y);

	DrawOverlays();

	PopState();

	SetHighColor(120, 120, 120);
	StrokeRect(BRect(
		origin.x,
		origin.y,
		origin.x + 255.0f,
		origin.y + 255.0f
	));

	DrawPatternStatePanel();
}


// -------------------------------------------------------------
// PatternTableView::MouseDown
//
// Locks or unlocks the tile under the mouse. Clicking the currently
// locked tile unlocks it; clicking another tile locks that tile.
//
// Parameters:
//   where - Mouse position in this view's coordinate system.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::MouseDown (BPoint where)
{
	MakeFocus(true);
	
	if (!HasROMLoaded()) {
		return;
	}
	
	int32 index = TileIndexFromPoint(where);
	if (index < 0) {
		return;
	}

	if (Show8x16()) {
		index &= ~0x1;
	}

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


// -------------------------------------------------------------
// PatternTableView::MouseMoved
//
// Updates hover selection while the mouse moves over the pattern-table
// bitmap. Persistent explore mode keeps the last valid tile visible when
// the mouse leaves or moves outside the bitmap.
//
// Parameters:
//   where   - Mouse position in this view's coordinate system.
//   transit - Haiku mouse transit state, such as entered/exited view.
//   msg     - Optional drag message; unused here.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::MouseMoved (BPoint where, uint32 transit, const BMessage* msg)
{
	(void)msg;
	
	if (!HasROMLoaded()) {
		fMouseValid = false;
		return;
	}

	if (transit == B_ENTERED_VIEW) {
		MakeFocus(true);
	}

	if (transit == B_EXITED_VIEW) {
		fMouseValid = false;

		// Persistent explore mode:
		// Keep the last valid tile visible.
		// Do not clear fHoverTileIndex.
		// Do not clear fCHRExplorer.

		if (!fTileLocked) {
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
		// Pointer is inside the view but outside the pattern bitmap.
		// Keep the last valid tile/explorer contents visible.
		Invalidate();
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


// -------------------------------------------------------------
// PatternTableView::Pulse
//
// Refreshes the CHR explorer while a tile is locked so the explorer
// stays synchronized with live CHR/PPU data.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}
	
	if (!fTileLocked) {
		return;
	}

	UpdateExplorer();
	NotifyCHRExplorer();
	
	Invalidate();
}



// -------------------------------------------------------------
// PatternTableView::DrawPixel
//
// Writes one indexed-color pixel into the backing B_CMAP8 bitmap.
//
// Parameters:
//   x     - Destination X coordinate in bitmap space.
//   y     - Destination Y coordinate in bitmap space.
//   color - Host color-map index to write.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	if (!fBits || !fBitmap) {
		return;
	}

	*(uint8 *)(fBits+x+(y*fRowBytes)) = color;
}


// -------------------------------------------------------------
// PatternTableView::DrawTile
//
// Decodes and draws one NES 2bpp 8x8 CHR tile into the pattern-table
// backing bitmap, scaled according to fTileSize.
//
// Parameters:
//   patternTable - Pattern table index: 0 for $0000, 1 for $1000.
//   tileIndex    - Tile index within the selected pattern table.
//   tileX        - Destination tile-column in the debug grid.
//   tileY        - Destination tile-row in the debug grid.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawTile (uint32 patternTable, int32 tileIndex, int32 tileX, int32 tileY)
{
	// Rendering requires mapper data, bitmap storage, and a host palette.
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper || !fBits || !fBitmap || !fHostPalette) {
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
	pal[0] = fHostPalette[mapper->read_vram(0x3f00) & 0x3f];
	pal[1] = fHostPalette[mapper->read_vram(0x3f01) & 0x3f];
	pal[2] = fHostPalette[mapper->read_vram(0x3f02) & 0x3f];
	pal[3] = fHostPalette[mapper->read_vram(0x3f03) & 0x3f];

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


// -------------------------------------------------------------
// PatternTableView::DrawPatternTable8x8
//
// Draws the selected pattern table as a normal 16x16 grid of 8x8
// tiles, showing all 256 tiles in linear CHR order.
//
// Parameters:
//   which - Pattern table index: 0 for $0000, 1 for $1000.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
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


// -------------------------------------------------------------
// PatternTableView::DrawPatternTable8x16
//
// Draws the selected pattern table as 8x16 sprite pairs. Even tile
// indices are drawn as the top half and the following odd tile as the
// bottom half.
//
// Parameters:
//   which - Pattern table index: 0 for $0000, 1 for $1000.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
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


// -------------------------------------------------------------
// PatternTableView::NotifyCHRExplorer
//
// Sends the active tile's CHR bytes and metadata to CHRExplorerView.
// Handles both 8x8 tile mode and 8x16 paired-tile mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
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
		fCHRExplorer->SetTile8x8(
			fWhichPatternTable,
			index,
			fTileLocked,
			fCHRTileAddress,
			fCHRBytes,
			0,   // bgPalette
			-1,  // whichNT
			0,   // nameTileAddr
			0,   // attrAddr
			0,   // attrByte
			0    // attrQuadrant
		);

		if (fMainWindow) {
			fMainWindow->HighlightPaletteDebugger(
				false,
				fCHRExplorer->SelectedPalette(),
				-1
			);
		}

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

	if (fMainWindow) {
		fMainWindow->HighlightPaletteDebugger(
			false,
			fCHRExplorer->SelectedPalette(),
			-1
		);
	}
}


// -------------------------------------------------------------
// PatternTableView::TileIndexFromPoint
//
// Converts a view-space mouse point into a pattern-table tile index.
// The calculation accounts for bitmap origin and current 8x8/8x16 mode.
//
// Parameters:
//   where - Point in this view's coordinate system.
//
// Returns:
//   Tile index under the point, or -1 if the point is outside the bitmap.
// -------------------------------------------------------------
int32
PatternTableView::TileIndexFromPoint (BPoint where) const
{
	BPoint origin = BitmapOrigin();

	float localX = where.x - origin.x;
	float localY = where.y - origin.y;

	float tileSize = static_cast<float>(fTileSize);
	float tableSize = tileSize * 16.0f;

	if (localX < 0.0f || localY < 0.0f || localX >= tableSize || localY >= tableSize)
		return -1;

	if (!Show8x16()) {
		int32 tx = static_cast<int32>(localX / tileSize);
		int32 ty = static_cast<int32>(localY / tileSize);

		if (tx < 0 || tx >= 16 || ty < 0 || ty >= 16)
			return -1;

		return tx + (ty * 16);
	}

	int32 cx = static_cast<int32>(localX / tileSize);
	int32 cy = static_cast<int32>(localY / (tileSize * 2.0f));

	if (cx < 0 || cx >= 16 || cy < 0 || cy >= 8)
		return -1;

	int32 within = static_cast<int32>((localY - (cy * tileSize * 2.0f)) / tileSize);
	int32 pair = cx + cy * 16;

	if (within < 0) {
		within = 0;
	}
	
	if (within > 1) {
		within = 1;
	}

	return pair * 2 + within;
}



// -------------------------------------------------------------
// PatternTableView::CellRectForTileIndex
//
// Returns the bitmap-space rectangle for a tile or 8x16 sprite-pair cell.
//
// Parameters:
//   index - Tile index to locate. In 8x16 mode this is rounded down to
//           the even top tile of the pair.
//
// Returns:
//   BRect in bitmap-local coordinates.
// -------------------------------------------------------------
BRect
PatternTableView::CellRectForTileIndex(int32 index) const
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

// -------------------------------------------------------------
// PatternTableView::DrawOverlays
//
// Draws hover/locked selection overlays and the optional external
// highlight sent from NameTableView. This function expects bitmap-local
// drawing coordinates; the caller translates before invoking it.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawOverlays()
{
	static const rgb_color kHoverStroke = {  0, 255, 255, 255 };
	static const rgb_color kHoverFill   = {  0, 255, 255,  32 };

	static const rgb_color kLockStroke  = { 255,   0, 255, 255 };
	static const rgb_color kLockFill    = { 255,   0, 255,  32 };

	// lock highlight takes priority
	// Draw active selection first: locked selection wins over hover.
	if (fTileLocked && fLockedTileIndex >= 0) {
		BRect r = CellRectForTileIndex(fLockedTileIndex & 0xff);

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		SetHighColor(kLockFill);
		FillRect(r);

		SetHighColor(kLockStroke);
		StrokeRect(r);

		PopState();
	} else if (fHoverTileIndex >= 0) {
		BRect r = CellRectForTileIndex(fHoverTileIndex & 0xff);

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		SetHighColor(kHoverFill);
		FillRect(r);

		SetHighColor(kHoverStroke);
		StrokeRect(r);

		PopState();
	}

	// external highlight from NameTableView
	// Draw NameTable-driven tile highlight, if one has been supplied.
	if (fHasExternalHighlight && fExternalWhichPT == fWhichPatternTable && fExternalTileIndex >= 0) {
		BRect r;
		float tileSize = static_cast<float>(fTileSize);
		int32 index = fExternalTileIndex & 0xff;

		if (Show8x16()) {
			// Match the layout used by DrawPatternTable8x16():
			// even tile = top half of pair, odd tile = bottom half
			int32 topIndex = index & ~0x1;
			int32 pair = topIndex / 2;
			int32 tx = pair % 16;
			int32 ty = (pair / 16) * 2;

			if (index & 0x1)
				ty += 1;

			r = BRect(
				tx * tileSize,
				ty * tileSize,
				((tx + 1) * tileSize) - 1.0f,
				((ty + 1) * tileSize) - 1.0f
			);
		} else {
			r = CellRectForTileIndex(index);
		}

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		rgb_color fill   = {255, 255,   0, 40};
		rgb_color stroke = {255, 255,   0, 255};

		SetHighColor(fill);
		FillRect(r);

		SetHighColor(stroke);
		StrokeRect(r);
		StrokeRect(r.InsetByCopy(1, 1));

		PopState();
	}
}


void
PatternTableView::SetViewMode(view_mode vm)
{
	if (fViewMode == vm) {
		return;
	}

	fViewMode = vm;

	// In 8x16 mode the top tile must be even.
	if (Show8x16()) {
		if (fHoverTileIndex >= 0) {
			fHoverTileIndex &= ~0x1;
		}

		if (fLockedTileIndex >= 0) {
			fLockedTileIndex &= ~0x1;
		}
	}

	// Prevent stale explorer data when switching modes.
	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	if (HasROMLoaded()) {
		UpdateExplorer();
		NotifyCHRExplorer();
	}

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::ActiveTileIndex
//
// Chooses the tile that should drive overlays and CHR explorer updates.
// Locked selection takes priority over hover selection.
//
// Parameters:
//   None.
//
// Returns:
//   Locked tile index if locked, otherwise hover tile index. May return -1.
// -------------------------------------------------------------
int32
PatternTableView::ActiveTileIndex() const
{
	if (fTileLocked) {
		return fLockedTileIndex;
	}

	return fHoverTileIndex;
}


// -------------------------------------------------------------
// PatternTableView::UpdateExplorer
//
// Reads CHR bytes for the active tile from PPU/mapper memory and updates
// cached tile address/bytes before notifying CHRExplorerView.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
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

	// CHR tiles are 16 bytes each: 8 bytes plane 0, 8 bytes plane 1.
	uint32 base = fWhichPatternTable ? 0x1000 : 0x0000;
	uint32 addr = base + (index * 16);

	fCHRTileAddress = addr;

	for (int32 i = 0; i < 16; i++) {
		fCHRBytes[i] = mapper->read_vram(addr + i);
	}

	NotifyCHRExplorer();
}


// -------------------------------------------------------------
// PatternTableView::SetExplorer
//
// Connects or replaces the CHR explorer used by this PatternTableView,
// passes it the host palette, and seeds it with the current tile.
//
// Parameters:
//   explorer - CHRExplorerView to receive active tile data. May be null.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::SetExplorer (CHRExplorerView *explorer)
{
	fCHRExplorer = explorer;

	if (fCHRExplorer)
		fCHRExplorer->SetPaletteHighlightTarget(fMainWindow, false);

	if (fCHRExplorer && fHostPalette)
		fCHRExplorer->SetHostPalette(fHostPalette);

	NotifyCHRExplorer();
}



// -------------------------------------------------------------
// PatternTableView::BitmapOrigin
//
// Computes where the 256x256 pattern-table bitmap should be drawn. The
// bitmap is horizontally inset and vertically centered between the top
// Controls panel and bottom Pattern State panel.
//
// Parameters:
//   None.
//
// Returns:
//   Top-left point for drawing the pattern-table bitmap.
// -------------------------------------------------------------
BPoint
PatternTableView::BitmapOrigin() const
{
	const float bitmapH = 256.0f;
	const float originX = 12.0f;

	// Must match DrawHeaderUI().
	const float controlsBottom = 82.0f;

	// Pattern State panel starts below the bitmap and extends
	// to the same bottom edge as the right-side panels.
	const float bottomReserve = 132.0f;

	const float topGap = 12.0f;
	const float bottomGap = 10.0f;

	const float availableTop = controlsBottom + topGap;
	const float availableBottom = Bounds().bottom - bottomReserve - bottomGap;

	float originY = availableTop + ((availableBottom - availableTop - bitmapH) * 0.5f);

	if (originY < availableTop) {
		originY = availableTop;
	}

	return BPoint(originX, originY);
}


// -------------------------------------------------------------
// PatternTableView::SetExternalHighlight
//
// Enables a highlight for a tile referenced by another view, usually a
// selected tile from NameTableView.
//
// Parameters:
//   whichPT   - Pattern table index that owns the highlighted tile.
//   tileIndex - Tile index to highlight.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::SetExternalHighlight (int32 whichPT, int32 tileIndex)
{
	fHasExternalHighlight = true;
	fExternalWhichPT = whichPT;
	fExternalTileIndex = tileIndex & 0xff;

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::ClearExternalHighlight
//
// Disables the external NameTable-driven tile highlight.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::ClearExternalHighlight()
{
	fHasExternalHighlight = false;
	fExternalTileIndex = -1;

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::DrawPatternStatePanel
//
// Draws the bottom Pattern State panel with aligned pattern table, base
// address, mode, tile, CHR address, and lock/hover state fields.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawPatternStatePanel()
{
	BPoint origin = BitmapOrigin();

	const float panelLeft = 4.0f;
	const float panelRight = Bounds().right - 8.0f;

	const float panelTop = origin.y + 256.0f + 10.0f;
	const float panelBottom = Bounds().bottom - 8.0f;

	BRect panel(panelLeft, panelTop, panelRight, panelBottom);

	::DrawDebugPanel(this, panel, "Pattern State");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 44.0f;

	float textY = panel.top + 36.0f;

	uint32 base = fWhichPatternTable ? 0x1000 : 0x0000;
	int32 index = ActiveTileIndex();

	BString s;
	
	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, textY));

		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(valueX, textY));

		textY += lineH;
	};

	s.SetToFormat("%ld", static_cast<long>(fWhichPatternTable));
	drawKV("PT:", s.String());

	s.SetToFormat("$%04lX", static_cast<unsigned long>(base));
	drawKV("Base:", s.String());

	drawKV("Mode:", Show8x16() ? "8x16" : "8x8");

	if (index >= 0) {
		uint32 addr = base + ((index & 0xff) * 16);
		s.SetToFormat("$%02lX", static_cast<unsigned long>(index & 0xff));
		drawKV("Tile:", s.String());

		s.SetToFormat("$%04lX", static_cast<unsigned long>(addr));
		drawKV("CHR:", s.String());

		drawKV("State:", fTileLocked ? "LOCKED" : "HOVER");
	} else {
		drawKV("Tile:", "--");
		drawKV("CHR:", "----");
		drawKV("State:", "--");
	}
}


// -------------------------------------------------------------
// PatternTableView::DrawBitmapPanel
//
// Draws the light panel frame behind the 256x256 pattern-table bitmap.
//
// Parameters:
//   origin - Top-left bitmap origin in view coordinates.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawBitmapPanel(BPoint origin)
{
	BRect panel(
		origin.x - 5.0f,
		origin.y - 5.0f,
		origin.x + 255.0f + 5.0f,
		origin.y + 255.0f + 5.0f
	);

	SetHighColor(228, 228, 228);
	FillRect(panel);

	SetHighColor(150, 150, 150);
	StrokeRect(panel);
}


// -------------------------------------------------------------
// PatternTableView::DrawHeaderUI
//
// Draws the top Controls panel describing PatternTable interactions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::DrawHeaderUI()
{
	const float panelLeft = 4.0f;
	const float panelTop = 4.0f;
	const float panelRight = Bounds().right - 8.0f;
	const float panelBottom = 82.0f;

	BRect panel(panelLeft, panelTop, panelRight, panelBottom);

	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 52.0f;

	float y = panel.top + 36.0f;

	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawKV("Zoom:",  "toggle 8x16 mode");
	drawKV("Mouse:", "move explore / click lock");
	drawKV("CHR:",   "1-4 palette / 0 source");
}


// -----------------------------------------------------------------------------
// PatternTableView::HasROMLoaded
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
PatternTableView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// PatternTableView::DrawNoROMMessage
//
// Draws a friendly empty-state message in the pattern-table bitmap area when
// the pattern table window is opened without a loaded ROM.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::DrawNoROMMessage()
{
	BPoint origin = BitmapOrigin();

	BRect panel(
		origin.x,
		origin.y,
		origin.x + 255.0f,
		origin.y + 255.0f
	);

	SetHighColor(230, 230, 230);
	FillRect(panel);

	SetHighColor(160, 160, 160);
	StrokeRect(panel);

	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to view pattern tables.";

	font_height fh;
	GetFontHeight(&fh);

	const float titleWidth = StringWidth(title);
	const float detailWidth = StringWidth(detail);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80);
	DrawString(
		title,
		BPoint(centerX - (titleWidth * 0.5f), centerY - 8.0f)
	);

	SetHighColor(120, 120, 120);
	DrawString(
		detail,
		BPoint(centerX - (detailWidth * 0.5f), centerY + fh.ascent + 8.0f)
	);

	SetFont(&prevFont);
}

