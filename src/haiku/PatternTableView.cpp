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
PatternTableView::PatternTableView (BRect frame, PretendoWindow *mainWindow, 
									int32 which, CHRExplorerView *explorer)
	: BView(frame, "pattern_table",  B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
{
	fMainWindow = mainWindow;
	fWhichPatternTable = which;
	fCHRExplorer = explorer;
	fHostPalette = fMainWindow ? fMainWindow->Palette() : nullptr;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

	fTileSize = 16;

	float w = static_cast<float>(fTileSize * 16) - 1.0f;
	float h = static_cast<float>(fTileSize * 16) - 1.0f;

	fBitmap = new BBitmap(BRect(0.0f, 0.0f, w, h), B_CMAP8);
	fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
	fRowBytes = fBitmap->BytesPerRow();

	fHoverTileIndex = -1;
	fLockedTileIndex = -1;
	fTileLocked = false;
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

	/*
	 * Refresh the palette here, not only in the constructor.
	 * On first window open, fMainWindow->Palette() may not have been ready yet.
	 */
	if (!fHostPalette && fMainWindow) {
		fHostPalette = fMainWindow->Palette();
	}

	if (fCHRExplorer && fHostPalette) {
		fCHRExplorer->SetHostPalette(fHostPalette);
	}

	/*
	 * Persistent explore mode:
	 * Seed a default tile so the CHR Explorer is never blank.
	 */
	if (fHoverTileIndex < 0) {
		fHoverTileIndex = 0;
	}

	fLockedTileIndex = -1;
	fTileLocked = false;

	if (HasROMLoaded()) {
		UpdateExplorer();
	} else if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// PatternTableView::Draw
//
// Draws the complete pattern-table debugger view.
//
// The header and bitmap panel are drawn first, followed by either the no-ROM
// state or the current pattern-table contents.  When a ROM is loaded, the debug
// palette and CHR snapshot are refreshed once per redraw so all rendered tiles
// use one coherent source image.  Overlays and the pattern-state panel are then
// drawn on top.
//
// Parameters:
//   updateRect - Region of the view that Haiku requested to redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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
		memset(fBits, 0x0, fBitmap->BitsLength());

		/*
		 * Resolve the four debug colors once for this redraw.
		 */
		RefreshDebugPalette();

		/*
		 * Capture the full CHR address space once so every tile in this
		 * Pattern Table bitmap is decoded from one coherent CHR image.
		 */
		CaptureCHRRenderSnapshot();

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
	StrokeRect(BRect(origin.x, origin.y, origin.x + 255.0f, origin.y + 255.0f));

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
PatternTableView::MouseMoved (BPoint where, uint32 transit, const BMessage *msg)
{
	(void)msg;

	if (!HasROMLoaded()) {
		return;
	}

	if (transit == B_ENTERED_VIEW) {
		MakeFocus(true);
	}

	if (transit == B_EXITED_VIEW) {
		/*
		 * Persistent explore mode:
		 * Keep the last valid tile and CHR Explorer contents visible.
		 */
		if (!fTileLocked) {
			Invalidate();
		}

		return;
	}

	if (fTileLocked) {
		return;
	}

	int32 index = TileIndexFromPoint(where);

	if (index < 0) {
		/*
		 * Pointer is inside the view but outside the pattern bitmap.
		 * Keep the last valid tile/explorer contents visible.
		 */
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


// -----------------------------------------------------------------------------
// PatternTableView::Pulse
//
// Refreshes the PatternTable display and CHR Explorer while a ROM is loaded.
//
// The PatternTable bitmap represents live CHR/PPU data, so the view is
// invalidated on every pulse.  The active tile's CHR data is also refreshed so
// the connected CHR Explorer, including its tile preview, palette previews, and
// pixel inspector, is populated immediately without requiring mouse movement.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	UpdateExplorer();

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
// Decodes and draws one NES 2bpp 8x8 CHR tile into the Pattern Table backing
// bitmap, scaled according to fTileSize.
//
// Most tiles are decoded from the stable CHR render snapshot captured at the
// beginning of the current Draw() call.
//
// If this tile is the externally highlighted tile and the source debugger
// supplied an exact 16-byte CHR snapshot, those bytes are used instead.  This
// keeps the highlighted Pattern Table tile synchronized with the source view
// even on bank-switched cartridges.
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
	if (!fBits || !fBitmap || !fHostPalette) {
		return;
	}

	tileIndex &= 0xff;

	int32 scale = (fTileSize > 0) ? (fTileSize / 8) : 1;

	if (scale < 1) {
		return;
	}

	uint32 tileAddr = ((patternTable & 1) << 12) + (tileIndex * 16);

	const bool useExternalCHR = fHasExternalHighlight && fHaveExternalCHRBytes
								&& fExternalWhichPT == static_cast<int32>(patternTable)
								&& fExternalTileIndex == tileIndex;

	int32 dstX0 = tileX * 8 * scale;
	int32 dstY0 = tileY * 8 * scale;

	int32 bmpW = static_cast<int32>(fBitmap->Bounds().Width()) + 1;
	int32 bmpH = static_cast<int32>(fBitmap->Bounds().Height()) + 1;

	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane;
		uint8 secondPlane;

		if (useExternalCHR) {
			firstPlane = fExternalCHRBytes[y];
			secondPlane = fExternalCHRBytes[y + 8];
		} else {
			firstPlane = fCHRRenderSnapshot[(tileAddr + y) & 0x1fff];
			secondPlane = fCHRRenderSnapshot[(tileAddr + y + 8) & 0x1fff];
		}

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			uint8 color = fDebugPalette[pixel];

			int32 px = dstX0 + x * scale;
			int32 py = dstY0 + y * scale;

			if (px < 0 || py < 0 || (px + scale - 1) >= bmpW || (py + scale - 1) >= bmpH) {
				continue;
			}

			for (int32 dy = 0; dy < scale; dy++) {
				for (int32 dx = 0; dx < scale; dx++) {
					DrawPixel(px + dx, py + dy, color);
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
// When an external highlight is active for this Pattern Table, notification
// from the local Pattern Table selection is suppressed. The external debugger
// selection owns the CHR Explorer until that external highlight is cleared.
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

	/*
	 * Preserve externally supplied CHR Explorer state while an external
	 * highlight is active for this Pattern Table.
	 *
	 * This includes suppressing a stale local Pattern Table lock.
	 */
	if (fHasExternalHighlight && fExternalWhichPT == fWhichPatternTable && fExternalTileIndex >= 0) {
		return;
	}

	int32 index = ActiveTileIndex();

	if (index < 0) {
		fCHRExplorer->Clear();
		return;
	}

	if (!Show8x16()) {
		fCHRExplorer->SetTile8x8(fWhichPatternTable, index, fTileLocked, fCHRTileAddress, fCHRBytes,
									0, -1, 0, 0, 0, 0);

		if (fMainWindow) {
			fMainWindow->HighlightPaletteDebugger(false, fCHRExplorer->SelectedPalette(), -1);
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

	fCHRExplorer->SetTile8x16(fWhichPatternTable, topIndex, fTileLocked, topAddr, topBytes,
								bottomAddr, bottomBytes, 0);

	if (fMainWindow) {
		fMainWindow->HighlightPaletteDebugger(false, fCHRExplorer->SelectedPalette(), -1);
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

		if (tx < 0 || tx >= 16 || ty < 0 || ty >= 16) {
			return -1;
		}

		return tx + (ty * 16);
	}

	int32 cx = static_cast<int32>(localX / tileSize);
	int32 cy = static_cast<int32>(localY / (tileSize * 2.0f));

	if (cx < 0 || cx >= 16 || cy < 0 || cy >= 8) {
		return -1;
	}

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
PatternTableView::CellRectForTileIndex (int32 index) const
{
	float tileSize = static_cast<float>(fTileSize);

	index &= 0xff;

	if (!Show8x16()) {
		int32 tx = index % 16;
		int32 ty = index / 16;
		
		return BRect(tx * tileSize, ty * tileSize, ((tx + 1) * tileSize) - 1.0f, 
					((ty + 1) * tileSize) - 1.0f);
	}

	index &= ~0x1;

	int32 pair = index / 2;
	int32 tx = pair % 16;
	int32 ty = pair / 16;

	return BRect(tx * tileSize, ty * (tileSize * 2.0f), 
				((tx + 1) * tileSize) - 1.0f, ((ty + 1) * (tileSize * 2.0f)) - 1.0f);
}


// -------------------------------------------------------------
// PatternTableView::DrawOverlays
//
// Draws hover/locked selection overlays and the optional external
// highlight supplied by another debugger view.
//
// External highlights may represent either a single 8x8 CHR tile
// or a complete 8x16 sprite pair. The highlight geometry follows
// the Pattern Table's current display mode.
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
	static const rgb_color kHoverStroke = { 0, 255, 255, 255 };
	static const rgb_color kHoverFill   = { 0, 255, 255, 32 };

	static const rgb_color kLockStroke  = { 255, 0, 255, 255 };
	static const rgb_color kLockFill    = { 255, 0, 255, 32 };

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

	// Draw externally supplied Pattern Table highlight.
	if (fHasExternalHighlight && fExternalWhichPT == fWhichPatternTable && fExternalTileIndex >= 0) {
		const float tileSize = static_cast<float>(fTileSize);
		const int32 index = fExternalTileIndex & 0xff;

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		const rgb_color fill   = { 255, 255, 0, 40 };
		const rgb_color stroke = { 255, 255, 0, 255 };

		/*
		 * A requested 8x16 sprite highlight represents the even-numbered
		 * top tile and the following odd-numbered bottom tile.
		 */
		if (fExternalHighlight8x16Pair) {
			const int32 topIndex = index & ~0x1;

			if (Show8x16()) {
				/*
				 * In 8x16 Pattern Table display mode, each even/odd tile
				 * pair is shown vertically as one 8x16 sprite cell.
				 */
				const int32 pair = topIndex / 2;
				const int32 tx = pair % 16;
				const int32 ty = (pair / 16) * 2;

				BRect r(tx * tileSize, ty * tileSize, ((tx + 1) * tileSize) - 1.0f, ((ty + 2) * tileSize) - 1.0f);
				SetHighColor(fill);
				FillRect(r);

				SetHighColor(stroke);
				StrokeRect(r);
				StrokeRect(r.InsetByCopy(1, 1));
			} else {
				/*
				 * In ordinary 8x8 Pattern Table display mode, the two
				 * CHR tiles retain their normal independent locations,
				 * so highlight both cells separately.
				 */
				BRect topRect = CellRectForTileIndex(topIndex);
				BRect bottomRect = CellRectForTileIndex((topIndex + 1) & 0xff);

				SetHighColor(fill);
				FillRect(topRect);
				FillRect(bottomRect);

				SetHighColor(stroke);

				StrokeRect(topRect);
				StrokeRect(topRect.InsetByCopy(1, 1));

				StrokeRect(bottomRect);
				StrokeRect(bottomRect.InsetByCopy(1, 1));
			}
		} else {
			/*
			 * Ordinary single-tile external highlight.
			 */
			BRect r;

			if (Show8x16()) {
				/*
				 * Match DrawPatternTable8x16() placement for one
				 * individual tile within an even/odd pair.
				 */
				const int32 topIndex = index & ~0x1;
				const int32 pair = topIndex / 2;
				const int32 tx = pair % 16;
				int32 ty = (pair / 16) * 2;

				if (index & 0x1) {
					ty += 1;
				}

				r = BRect(tx * tileSize, ty * tileSize, ((tx + 1) * tileSize) - 1.0f, 
						 ((ty + 1) * tileSize) - 1.0f);
			} else {
				r = CellRectForTileIndex(index);
			}

			SetHighColor(fill);
			FillRect(r);

			SetHighColor(stroke);
			StrokeRect(r);
			StrokeRect(r.InsetByCopy(1, 1));
		}

		PopState();
	}
}

// -------------------------------------------------------------
// PatternTableView::DrawPatternStatePanel
//
// Draws the bottom Pattern State panel with aligned Pattern Table, base
// address, display mode, tile/CHR selection information, and a compact
// one-line highlight legend.
//
// External selections take priority when reporting the current Pattern State.
//
// Highlight colors:
//
//   Cyan   - local hover
//   Pink   - local Pattern Table lock
//   Yellow - external debugger selection, such as OAM
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
	const float panelTop = origin.y + 256.0f - 2.0f;
	const float panelBottom = Bounds().bottom - 8.0f;

	BRect panel(panelLeft, panelTop, panelRight, panelBottom);
	::DrawDebugPanel(this, panel, "Pattern State");

	SetFontSize(11.0f);

	BFont uiFont;
	GetFont(&uiFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(uiFont.Size());

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;
	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 52.0f;
	float textY = panel.top + 36.0f;

	const uint32 base = fWhichPatternTable ? 0x1000 : 0x0000;

	bool external = false;
	int32 index = -1;

	if (fHasExternalHighlight && fExternalWhichPT == fWhichPatternTable && fExternalTileIndex >= 0) {
		external = true;
		index = fExternalTileIndex & 0xff;
	} else {
		index = ActiveTileIndex();
	}

	BString s;
	auto drawLV = [&](const char *label, const char *value, bool fixedValue) {

		SetFont(&uiFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, textY));

		SetFont(fixedValue ? &fixed : &uiFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(valueX, textY));

		textY += lineH;
	};

	s.SetToFormat("%ld", static_cast<long>(fWhichPatternTable));
	drawLV("PT:", s.String(), false);

	s.SetToFormat("$%04lX", static_cast<unsigned long>(base));
	drawLV("Base:", s.String(), true);
	drawLV("Mode:", Show8x16() ? "8x16" : "8x8", false);

	if (index >= 0) {
		if (external && fExternalHighlight8x16Pair) {
			const int32 topIndex = index & ~0x1;
			const uint32 topAddr = base + (static_cast<uint32>(topIndex) * 16);
			const uint32 bottomAddr = topAddr + 16;

			s.SetToFormat("$%02lX/$%02lX", static_cast<unsigned long>(topIndex),
											static_cast<unsigned long>((topIndex + 1) & 0xff));
			drawLV("Tiles:", s.String(), true);

			s.SetToFormat("$%04lX/$%04lX", static_cast<unsigned long>(topAddr),
											static_cast<unsigned long>(bottomAddr));
			drawLV("CHR:", s.String(), true);
		} else {
			const uint32 addr = base + (static_cast<uint32>(index & 0xff) * 16);
			
			s.SetToFormat("$%02lX", static_cast<unsigned long>(index & 0xff));
			drawLV("Tile:", s.String(), true);

			s.SetToFormat("$%04lX", static_cast<unsigned long>(addr));
			drawLV("CHR:", s.String(), true);
		}

		if (external) {
			drawLV("State:", "EXTERNAL", false);
		} else if (fTileLocked) {
			drawLV("State:", "LOCAL LOCK", false);
		} else {
			drawLV("State:", "HOVER", false);
		}
	} else {
		drawLV("Tile:", "--", false);
		drawLV("CHR:", "----", false);
		drawLV("State:", "--", false);
	}

	/*
	 * Compact one-line highlight legend.
	 */
	textY += 4.0f;

	SetFont(&uiFont);

	SetHighColor(60, 60, 60);
	DrawString("Colors:", BPoint(labelX, textY));

	float 
	legendX = valueX;

	auto drawLegendItem = [&](const rgb_color &color, const char *text) {
		const float boxSize = 7.0f;
		BRect swatch(legendX, textY - 7.0f, legendX + boxSize, textY);

		SetHighColor(color);
		FillRect(swatch);

		SetHighColor(90, 90, 90);
		StrokeRect(swatch);

		legendX += 12.0f;

		SetHighColor(0, 0, 0);
		DrawString(text, BPoint(legendX, textY));

		legendX += StringWidth(text) + 12.0f;
	};

	drawLegendItem(rgb_color { 0, 255, 255, 255 }, "Hover");
	drawLegendItem(rgb_color { 255, 0, 255, 255 }, "Lock");
	drawLegendItem(rgb_color { 255, 255, 0, 255 }, "External");

	SetFont(&uiFont);
}


// -------------------------------------------------------------
// PatternTableView::SetViewMode
//
// Changes the Pattern Table display between normal 8x8 tile mode and 8x16
// sprite-pair mode.
//
// When switching to 8x16 mode, active selections are normalized to the even
// top tile of each pair.  The CHR Explorer is cleared and then repopulated from
// the current active tile using the new interpretation.
//
// Parameters:
//   vm - New Pattern Table display mode.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::SetViewMode (view_mode vm)
{
	if (fViewMode == vm) {
		return;
	}

	fViewMode = vm;

	/*
	 * In 8x16 mode the active sprite pair is represented by its even
	 * top-tile index.
	 */
	if (Show8x16()) {
		if (fHoverTileIndex >= 0) {
			fHoverTileIndex &= ~0x1;
		}

		if (fLockedTileIndex >= 0) {
			fLockedTileIndex &= ~0x1;
		}
	}

	/*
	 * Prevent stale explorer data while changing tile interpretation.
	 */
	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	if (HasROMLoaded()) {
		UpdateExplorer();
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
// Refreshes the active Pattern Table selection and sends it to the connected
// CHR Explorer.
//
// When an external highlight is active for this Pattern Table, that external
// debugger selection owns the CHR Explorer. Local Pattern Table hover or lock
// state must not overwrite the externally supplied tile information.
//
// In normal 8x8 mode, the selected tile's 16 CHR bytes are cached locally
// before notification. In 8x16 mode, NotifyCHRExplorer() reads both halves of
// the sprite pair directly, so the redundant single-tile cache read is skipped.
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

	/*
	 * An externally supplied selection, such as an OAM sprite, owns the
	 * CHR Explorer while it is active for this Pattern Table.
	 *
	 * Do not allow either local hover or a local Pattern Table lock to
	 * overwrite that external selection.
	 */
	if (fHasExternalHighlight && fExternalWhichPT == fWhichPatternTable && fExternalTileIndex >= 0) {
		return;
	}

	int32 index = ActiveTileIndex();

	if (index < 0) {
		fCHRExplorer->Clear();
		return;
	}

	uint32 base = fWhichPatternTable ? 0x1000 : 0x0000;

	/*
	 * NotifyCHRExplorer() handles both halves directly in 8x16 mode.
	 */
	if (Show8x16()) {
		NotifyCHRExplorer();
		return;
	}

	/*
	 * CHR tiles are 16 bytes each:
	 * 8 bytes plane 0 followed by 8 bytes plane 1.
	 */
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
// Connects or replaces the CHR Explorer used by this PatternTableView.
//
// The explorer is configured with the current Palette debugger target and host
// palette.  If a ROM is loaded, the active tile's CHR data is refreshed before
// notifying the new explorer so it cannot receive stale cached tile bytes.
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

	if (!fCHRExplorer) {
		return;
	}

	fCHRExplorer->SetPaletteHighlightTarget(fMainWindow, false);

	if (fHostPalette) {
		fCHRExplorer->SetHostPalette(fHostPalette);
	}

	if (HasROMLoaded()) {
		UpdateExplorer();
	} else {
		fCHRExplorer->Clear();
	}
}


// -----------------------------------------------------------------------------
// PatternTableView::Clear
//
// Clears all ROM-specific Pattern Table selection, cached CHR, external
// highlight, and linked debugger state.
//
// This is used when the current ROM is unloaded so an open Pattern Table window
// cannot continue displaying selections, CHR Explorer information, Palette
// debugger highlighting, or externally supplied CHR data belonging to the
// previous cartridge.
//
// The Pattern Table identity and display mode are preserved because they are
// properties of the debugger window rather than of the loaded ROM.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::Clear()
{
	fHoverTileIndex = -1;
	fLockedTileIndex = -1;
	fTileLocked = false;

	fCHRTileAddress = 0;
	memset(fCHRBytes, 0, sizeof(fCHRBytes));

	fHasExternalHighlight = false;
	fExternalWhichPT = 0;
	fExternalTileIndex = -1;

	fHaveExternalCHRBytes = false;

	memset(fExternalCHRBytes, 0, sizeof(fExternalCHRBytes));

	if (fBitmap && fBits) {
		memset(fBits, 0, fBitmap->BitsLength());
	}

	if (fMainWindow) {
		fMainWindow->ClearPaletteDebuggerHighlight();
	}

	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::RefreshDebugPalette
//
// Refreshes the four host color-map indices used to render Pattern Table
// pixels.
//
// PatternTableView uses background palette 0 as a stable four-color debugging
// palette.  The palette is read once per Pattern Table redraw rather than once
// for every CHR tile.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::RefreshDebugPalette()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper || !fHostPalette) {
		memset(fDebugPalette, 0, sizeof(fDebugPalette));
		return;
	}

	for (int32 i = 0; i < 4; i++) {
		uint8 nesColor = mapper->read_vram(0x3f00 + i) & 0x3f;
		fDebugPalette[i] = fHostPalette[nesColor];
	}
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


// -----------------------------------------------------------------------------
// PatternTableView::SetExternalHighlight
//
// Sets an externally requested single-tile Pattern Table highlight.
//
// This overload does not supply a CHR snapshot. Any previously retained CHR
// snapshot or 8x16 sprite-pair state is cleared so the new highlight represents
// only the requested live 8x8 CHR tile.
//
// Parameters:
//   whichPT   - Pattern-table index containing the tile.
//   tileIndex - CHR tile index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::SetExternalHighlight (int32 whichPT, int32 tileIndex)
{
	fHasExternalHighlight = true;
	fExternalWhichPT = whichPT;
	fExternalTileIndex = tileIndex;
	fExternalHighlight8x16Pair = false;
	fHaveExternalCHRBytes = false;
	
	memset(fExternalCHRBytes, 0, sizeof(fExternalCHRBytes));

	Invalidate();
}


// -----------------------------------------------------------------------------
// PatternTableView::SetExternalHighlight
//
// Sets an externally requested single-tile Pattern Table highlight together
// with a retained 16-byte CHR snapshot.
//
// The supplied CHR bytes represent the highlighted 8x8 tile at the time the
// external debugger captured it. Any previous 8x16 sprite-pair state is cleared.
//
// Parameters:
//   whichPT   - Pattern-table index containing the tile.
//   tileIndex - CHR tile index.
//   chrBytes  - Pointer to the 16 CHR bytes describing the tile.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::SetExternalHighlight(int32 whichPT, int32 tileIndex, const uint8 *chrBytes)
{
	fHasExternalHighlight = true;
	fExternalWhichPT = whichPT;
	fExternalTileIndex = tileIndex;
	fExternalHighlight8x16Pair = false;
	fHaveExternalCHRBytes = false;
	
	memset(fExternalCHRBytes, 0, sizeof(fExternalCHRBytes));

	if (chrBytes) {
		for (int32 i = 0; i < 16; i++) {
			fExternalCHRBytes[i] = chrBytes[i];
		}

		fHaveExternalCHRBytes = true;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// PatternTableView::SetExternalHighlight
//
// Sets an externally requested Pattern Table highlight for a sprite-oriented
// debugger source such as OAMDebugView.
//
// The external selection records both whether it represents a complete 8x16
// tile pair and whether the originating OAM selection is currently locked.
// The connected CHR Explorer is synchronized with the same tile data and state.
//
// Parameters:
//   whichPT           - Pattern-table index containing the tile.
//   tileIndex         - CHR tile index.
//   highlight8x16Pair - true for a complete even/odd 8x16 sprite pair.
//   externalLocked    - true when the originating external selection is locked.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableView::SetExternalHighlight (int32 whichPT, int32 tileIndex, bool highlight8x16Pair, bool externalLocked)
{
	fHasExternalHighlight = true;
	fExternalWhichPT = whichPT;
	fExternalTileIndex = tileIndex;
	fExternalHighlight8x16Pair = highlight8x16Pair;
	fExternalHighlightLocked = externalLocked;
	fHaveExternalCHRBytes = false;
	
	memset(fExternalCHRBytes, 0, sizeof(fExternalCHRBytes));

	if (fCHRExplorer && whichPT == fWhichPatternTable && tileIndex >= 0) {
		Mapper *mapper = nes::cart.mapper();

		if (mapper) {
			const uint32 base = whichPT ? 0x1000 : 0x0000;

			if (highlight8x16Pair) {
				const int32 topIndex = (tileIndex & 0xff) & ~0x1;
				const uint32 topAddr = base + (static_cast<uint32>(topIndex) * 16);
				const uint32 bottomAddr = topAddr + 16;

				uint8 topBytes[16];
				uint8 bottomBytes[16];

				for (int32 i = 0; i < 16; i++) {
					topBytes[i] = mapper->read_vram(topAddr + i);
					bottomBytes[i] = mapper->read_vram(bottomAddr + i);
				}

				fCHRExplorer->SetTile8x16(whichPT, topIndex, externalLocked, topAddr, topBytes,
										  bottomAddr, bottomBytes, 0);
			} else {
				const int32 index = tileIndex & 0xff;
				const uint32 addr = base + (static_cast<uint32>(index) * 16);

				uint8 chrBytes[16];

				for (int32 i = 0; i < 16; i++) {
					chrBytes[i] = mapper->read_vram(addr + i);
				}

				fCHRExplorer->SetTile8x8(whichPT, index, externalLocked, addr, chrBytes,
										 0, -1, 0, 0, 0, 0);
			}
		}
	}

	Invalidate();
}


// -------------------------------------------------------------
// PatternTableView::ClearExternalHighlight
//
// Disables the external debugger-driven Pattern Table tile highlight and clears
// any CHR snapshot associated with it.
//
// Any retained 8x16 sprite-pair highlight state is also cleared so a later
// external highlight cannot inherit stale sprite-mode information.
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
	if (!fHasExternalHighlight && fExternalTileIndex < 0 && !fHaveExternalCHRBytes && !fExternalHighlight8x16Pair) {
		return;
	}

	fHasExternalHighlight = false;
	fExternalWhichPT = 0;
	fExternalTileIndex = -1;
	fExternalHighlight8x16Pair = false;
	fExternalHighlightLocked = false;
	fHaveExternalCHRBytes = false;
	
	memset(fExternalCHRBytes, 0, sizeof(fExternalCHRBytes));

	Invalidate();
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
	BRect panel(origin.x - 5.0f, origin.y - 5.0f, origin.x + 255.0f + 5.0f, origin.y + 255.0f + 5.0f);

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

	auto drawLV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawLV("Zoom:",  "toggle 8x16 mode");
	drawLV("Mouse:", "move explore / click lock");
	drawLV("CHR:",   "1-4 palette / 0 source");
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
// Draws the PatternTable empty-state message inside the normal 256x256 bitmap
// area when no ROM is loaded.
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
	BRect panel(origin.x, origin.y, origin.x + 255.0f, origin.y + 255.0f);

	SetHighColor(230, 230, 230);
	FillRect(panel);

	SetHighColor(160, 160, 160);
	StrokeRect(panel);

	BFont previousFont;
	GetFont(&previousFont);

	BFont font = previousFont;
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
	DrawString(title, BPoint(centerX - (titleWidth * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - (detailWidth * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&previousFont);
}


// -------------------------------------------------------------
// PatternTableView::CaptureCHRRenderSnapshot
//
// Captures the complete 8 KB CHR address space used by the Pattern Table
// renderer.
//
// PatternTableView builds an entire bitmap during one Draw() call.  Reading CHR
// directly from the mapper for every tile can mix data from different emulator
// states if mapper/PPU state changes while the debugger window is redrawing.
//
// Capturing CHR once at the beginning of the redraw guarantees that every tile
// in that bitmap is decoded from one stable CHR image.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------
void
PatternTableView::CaptureCHRRenderSnapshot()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		memset(fCHRRenderSnapshot, 0, sizeof(fCHRRenderSnapshot));

		return;
	}

	for (uint32 address = 0; address < 0x2000; address++) {
		fCHRRenderSnapshot[address] = mapper->read_vram(address);
	}
}

