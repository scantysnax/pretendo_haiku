// -------------------------------------------------------------
// NameTableView.cpp
//
// Implements the NameTable debugger view. This view renders one NES
// NameTable, shows palette/attribute/viewport overlays, tracks hover
// and locked tiles, and forwards selected tile information to the CHR
// explorer and PatternTable windows.
// -------------------------------------------------------------

#include "NameTableView.h"
#include "PretendoWindow.h"
#include "CHRExplorerView.h"
#include "PatternTableWindow.h"
#include "PatternTableView.h"

#include "Cart.h"
#include "Mapper.h"
#include "Ppu.h"

#include "DebugHelpers.h"

#include <String.h>

#include <algorithm>
#include <cmath>
#include <cstring>



// -------------------------------------------------------------
// NameTableBaseFromIndex
//
// Converts a NameTable index into its PPU NameTable base address.
//
// Parameters:
//   which - NameTable index. Only the low two bits are used.
//
// Returns:
//   Base PPU address for the selected NameTable ($2000-$2C00).
// -------------------------------------------------------------
static inline uint32
NameTableBaseFromIndex (int32 which)
{	
	return 0x2000 + ((which % 4) * 0x400);
}


// Forward declarations for local helper functions.
//
// These are needed because NameTableView::~NameTableView() uses
// ClearPatternWindowHighlight(), but the helper's full definition
// appears later in this file.
static inline void SetPatternWindowHighlight(PatternTableWindow *window, int32 whichPT, 
											 int32 tileIndex, const uint8 *chrBytes = nullptr);
static inline void ClearPatternWindowHighlight(PatternTableWindow *window);


// -----------------------------------------------------------------------------
// NameTableView::NameTableView
//
// Creates the NameTable debugger view and stores the owning window,
// NameTable index, and optional CHR Explorer connection.
//
// Parameters:
//   frame      - View frame rectangle.
//   mainWindow - Parent Pretendo window used for palette/PPU state.
//   which      - NameTable index, 0-3.
//   explorer   - Optional CHRExplorerView to update from this view.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
NameTableView::NameTableView(BRect frame, PretendoWindow *mainWindow, int32 which, CHRExplorerView *explorer)
	: BView(frame, "name_table_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE),
	
	fMainWindow(mainWindow),
	fCHRExplorer(explorer),
	fWhichNameTable(which)
{
	fPalette = fMainWindow ? fMainWindow->Palette() : nullptr;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -------------------------------------------------------------
// NameTableView::~NameTableView
//
// Releases the backing bitmap used to render the 256x240 NameTable.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -------------------------------------------------------------
NameTableView::~NameTableView()
{
	// Defensive cleanup. DetachedFromWindow() should normally clear
	// external highlights first, but this keeps destruction safe even
	// if the view is deleted through another path.
	ClearPatternWindowHighlight(fPatternTable0);
	ClearPatternWindowHighlight(fPatternTable1);

	if (fMainWindow) {
		fMainWindow->ClearPaletteDebuggerHighlight();
	}

	delete fBitmap;

	fBitmap = nullptr;
	fBits = nullptr;
	fRowBytes = 0;
}


// -----------------------------------------------------------------------------
// SetPatternWindowHighlight
//
// Sets the external tile highlight in a Pattern Table debugger window.
//
// The caller may optionally provide the exact 16 CHR bytes used by the source
// debugger view.  Passing those bytes keeps the externally highlighted tile
// synchronized with the NameTable view on cartridges that switch CHR banks.
//
// Parameters:
//   window    - Pattern Table window to update.
//   whichPT   - Pattern table index: 0 or 1.
//   tileIndex - Tile index to highlight.
//   chrBytes  - Optional 16-byte CHR image for the highlighted tile.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static inline void
SetPatternWindowHighlight (PatternTableWindow *window, int32 whichPT, int32 tileIndex, const uint8 *chrBytes)
{
	if (!window) {
		return;
	}

	if (!window->Lock()) {
		return;
	}

	PatternTableView *view = window->View();

	if (view) {
		if (chrBytes) {
			view->SetExternalHighlight(whichPT, tileIndex, chrBytes);
		} else {
			view->SetExternalHighlight(whichPT, tileIndex);
		}
	}

	window->Unlock();
}


// -------------------------------------------------------------
// ClearPatternWindowHighlight
//
// Clears the external NameTable-driven highlight from a PatternTable
// window, if it exists and can be locked safely.
//
// Parameters:
//   window - PatternTableWindow to update.
//
// Returns:
//   None.
// -------------------------------------------------------------
static inline void
ClearPatternWindowHighlight (PatternTableWindow *window)
{
	if (!window) {
		return;
	}

	if (window->Lock()) {
		PatternTableView* view = window->View();
		
		if (view) {
			view->ClearExternalHighlight();
		}

		window->Unlock();
	}
}


// -------------------------------------------------------------
// NameTableTileOrigin
//
// Converts a NameTable index into its world tile origin inside the
// 2x2 NameTable layout.
//
// Parameters:
//   whichNameTable - NameTable index, 0-3.
//   originTX       - Receives the world tile X origin.
//   originTY       - Receives the world tile Y origin.
//
// Returns:
//   None.
// -------------------------------------------------------------
static inline void
NameTableTileOrigin (int32 whichNameTable, int32 &originTX, int32 &originTY)
{
	switch (whichNameTable % 4) {
		default:
		case 0:
			originTX = 0;
			originTY = 0;
			break;

		case 1:
			originTX = 32;
			originTY = 0;
			break;

		case 2:
			originTX = 0;
			originTY = 30;
			break;

		case 3:
			originTX = 32;
			originTY = 30;
			break;
	}
}


// -----------------------------------------------------------------------------
// NameTableView::AttachedToWindow
//
// Performs final setup after the NameTable view is attached to its window.
//
// The backing bitmap is allocated and cleared, the CHR Explorer receives the
// host palette, and a default active tile is established at the upper-left
// corner of this specific NameTable.
//
// Hover coordinates are stored in world-tile space, so the default tile must use
// the world origin of the selected NameTable rather than always using (0, 0).
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::AttachedToWindow()
{
	BView::AttachedToWindow();

	/*
	 * Allocate the backing bitmap once.
	 */
	if (!fBitmap) {
		fBitmap = new BBitmap(BRect(0, 0, WIDTH - 1, HEIGHT - 1), B_CMAP8);
		fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
		fRowBytes = fBitmap->BytesPerRow();
	}

	if (fBits) {
		memset(fBits, 0x0, fBitmap->BitsLength());
	}

	fBitmapDirty = true;

	/*
	 * Give the explorer its host palette before sending tile data.
	 */
	if (fCHRExplorer && fPalette) {
		fCHRExplorer->SetHostPalette(fPalette);
	}

	/*
	 * Persistent explore mode needs a valid default tile.
	 *
	 * fHoverTileX/Y are WORLD coordinates, not local NameTable
	 * coordinates.  Seed them with the upper-left world tile of
	 * this particular NameTable.
	 */
	if (fHoverTileX < 0 || fHoverTileY < 0) {
		int32 originTX = 0;
		int32 originTY = 0;

		NameTableTileOrigin(fWhichNameTable, originTX, originTY);

		fHoverTileX = originTX;
		fHoverTileY = originTY;
	}

	/*
	 * Populate tile, attribute, palette, CHR, and cross-view state
	 * immediately so no mouse movement is required.
	 */
	UpdateCHRExplorer();

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	Invalidate();
}


// -----------------------------------------------------------------------------
// NameTableView::DetachedFromWindow
//
// Cleans up cross-view debugger state when the NameTable view is removed from
// its window.
//
// Any PatternTable highlight and Palette debugger highlight owned by this
// NameTable view are cleared so other debugger windows do not retain stale
// selection state after the NameTable window is closed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::DetachedFromWindow()
{
	ClearPatternWindowHighlight(fPatternTable0);
	ClearPatternWindowHighlight(fPatternTable1);

	if (fMainWindow) {
		fMainWindow->ClearPaletteDebuggerHighlight();
	}

	BView::DetachedFromWindow();
}


// -----------------------------------------------------------------------------
// NameTableView::Draw
//
// Draws the complete NameTable debugger view.
//
// The function clears the view background, draws the NameTable bitmap and its
// surrounding panel, renders all enabled bitmap-space overlays, and then draws
// the controls and debugger state panels in normal view coordinates.
//
// When no ROM is loaded, the bitmap area is replaced with the "No ROM loaded"
// message while the rest of the debugger UI remains visible.
//
// Parameters:
//   updateRect - Invalidated region supplied by the app_server.  The current
//                implementation redraws the complete view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	BPoint origin = BitmapOrigin();
	DrawHeaderUI();
	DrawBitmapPanel(origin);

	if (!nes::cart.mapper()) {
		DrawNoROMMessage();
		DrawDebugPanel();
		return;
	}

	if (fBitmap && fBits) {
		if (fBitmapDirty) {
			memset(fBits, 0x0, fBitmap->BitsLength());
			DrawNameTable(fWhichNameTable);
			fBitmapDirty = false;
		}

		DrawBitmap(fBitmap, origin);
	}

	PushState();
	 TranslateBy(origin.x, origin.y);

	if (fShowAttributeMap) {
		DrawAttributeMapOverlay();
	}

	if (fShowAttributeBlocks) {
		DrawAttributeBlockOverlay();
	}

	DrawOverlays();
	DrawActiveAttributeCellOverlay();
	DrawActiveAttributeBlockOverlay();

	if (fShowAttributeGrid) {
		DrawAttributeGrid();
	}

	if (fShowViewportBox) {
		DrawPPUViewportOverlay();
	}

	if (fShowMatchingTiles) {
		DrawMatchingTileOverlay();
	}

	PopState();

	SetHighColor(120, 120, 120);
	StrokeRect(BRect(origin.x, origin.y, origin.x + 255.0f, origin.y + 239.0f));

	DrawDebugPanel();
}


// -----------------------------------------------------------------------------
// NameTableView::KeyDown
//
// Handles debugger keyboard shortcuts for the attribute grid and map,
// attribute blocks, matching tiles, PPU viewport overlay, and freeze mode.
//
// Parameters:
//   bytes    - Pressed key bytes.
//   numBytes - Number of bytes in the key event.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

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

		case 'v':
		case 'V':
			fShowViewportBox = !fShowViewportBox;
			break;

		case ' ':
			if (!fFreezeUpdates) {
				/*
		 		* Take a fresh coherent snapshot at the moment freeze is requested.
		 		*/
				CapturePPUSnapshot();

				fFreezeUpdates = true;
				fBitmapDirty = true;

				UpdateCHRExplorer();
			} else {
				/*
		 		* Leaving freeze does not discard the current snapshot immediately.
		 		* The next Pulse() replaces it atomically with a new live snapshot.
				*/
				fFreezeUpdates = false;

				fLastPPUFrame = static_cast<uint64>(-1);
			}
			break;

		case 'f':
		case 'F':
			fShowAttributeBlocks = !fShowAttributeBlocks;
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			return;
	}

	Invalidate();
}


// -------------------------------------------------------------
// NameTableView::MouseDown
//
// Locks or unlocks the active tile. A normal click locks a world tile;
// Shift-click locks to a screen position that follows scrolling.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::MouseDown (BPoint where)
{
	MakeFocus(true);
	
	 if (!nes::cart.mapper()) {
	 	return;
	 }

	int32 x;
	int32 y;

	if (!ComputeTileFromViewPoint(where, x, y)) {
		return;
	}

	uint32 mods = modifiers();
	bool screenLock = (mods & B_SHIFT_KEY) != 0;

	if (fTileLocked) {
		if (fLockToScreen == screenLock) {
			if (screenLock) {
				fTileLocked = false;
				fLockToScreen = false;
				fLockedTileX = -1;
				fLockedTileY = -1;
			} else if (fLockedTileX == x && fLockedTileY == y) {
				fTileLocked = false;
				fLockToScreen = false;
				fLockedTileX = -1;
				fLockedTileY = -1;
			} else {
				fLockedTileX = x;
				fLockedTileY = y;
			}
		} else {
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

	UpdateCHRExplorer();

	Invalidate();
}


// -------------------------------------------------------------
// NameTableView::MouseMoved
//
// Updates the active hover tile while preserving the last useful
// explorer state when the pointer leaves the bitmap or view.
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   transit - BView mouse transit state.
//   msg     - Optional drag message; currently unused.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::MouseMoved (BPoint where, uint32 transit, const BMessage *msg)
{
	(void)msg;
	
	if (!nes::cart.mapper()) {
		return;
	}

	if (transit == B_ENTERED_VIEW) {
		MakeFocus(true);
	}

	if (transit == B_EXITED_VIEW) {
		if (!fTileLocked) {
			// Persistent explore mode:
			//
			// Keep the last valid hovered tile, CHR explorer contents,
			// PatternTable highlight, and Palette Viewer highlight.
			//
			// Do not clear fHoverTileX/Y.
			// Do not call fCHRExplorer->Clear().
			// Do not call ClearPatternWindowHighlight().
			// Do not call ClearPaletteDebuggerHighlight().
			Invalidate();
		}

		return;
	}

	// Locked tile mode:
	//
	// Mouse movement should not change the active NameTable tile,
	// CHR explorer contents, PatternTable highlight, or Palette Viewer
	// highlight.
	if (fTileLocked) {
		return;
	}

	int32 x = -1;
	int32 y = -1;

	if (!ComputeTileFromViewPoint(where, x, y)) {
		// Pointer is outside the bitmap but still inside the view
		// header/bottom panel area.
		//
		// Persistent explore mode keeps the last valid tile visible.
		// Do not clear the CHR explorer, PatternTable highlight, or
		// Palette Viewer highlight here.
		Invalidate();
		return;
	}

	if (x == fHoverTileX && y == fHoverTileY) {
		return;
	}

	fHoverTileX = x;
	fHoverTileY = y;

	UpdateCHRExplorer();

	Invalidate();
}


// -----------------------------------------------------------------------------
// NameTableView::Pulse
//
// Refreshes the live NameTable display whenever a new PPU frame becomes
// available.
//
// Cross-view debugger selection is intentionally not recomputed on every frame
// once a valid active tile already exists.  Hover/lock selection changes are
// handled by the mouse/keyboard selection paths themselves.
//
// This prevents a stationary NameTable selection from oscillating between
// transient tile values while the game updates PPU memory.
//
// When the view has just been initialized or a new ROM has been loaded, one
// initial CHR Explorer / PatternTable synchronization is performed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::Pulse()
{
	if (!fMainWindow) {
		return;
	}

	uint32 x = 0;
	uint32 y = 0;
	uint32 frameId = 0;

	if (!fMainWindow->GetLatchedScroll(x, y, frameId)) {
		return;
	}

	const bool needsInitialization = !fHavePPUSnapshot || (!fTileLocked && (fHoverTileX < 0 || fHoverTileY < 0));

	if (fFreezeUpdates && !needsInitialization) {
		return;
	}

	if (frameId == fLastPPUFrame && !needsInitialization) {
		return;
	}

	fLastPPUFrame = frameId;

	fScrollX = x % 512;
	fScrollY = y % 480;

	CapturePPUSnapshot();

	fBitmapDirty = true;

	if (!fTileLocked && (fHoverTileX < 0 || fHoverTileY < 0)) {
		int32 originTX = 0;
		int32 originTY = 0;

		NameTableTileOrigin(fWhichNameTable, originTX, originTY);
		fHoverTileX = originTX;
		fHoverTileY = originTY;

		/*
		 * Initial connection only. Once a valid active tile exists,
		 * mouse/lock selection changes own cross-view synchronization.
		 */
		UpdateCHRExplorer();
	}

	Invalidate();
}


// -------------------------------------------------------------
// NameTableView::DrawPixel
//
// Writes one indexed-color pixel into the NameTable backing bitmap.
//
// Parameters:
//   x     - Bitmap-space X coordinate.
//   y     - Bitmap-space Y coordinate.
//   color - Host palette color index to write.
//
// Returns:
//   None.
// -------------------------------------------------------------
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


// -------------------------------------------------------------
// NameTableView::ComputeTileFromViewPoint
//
// Converts a view-space point into a world-space NameTable tile.
//
// Parameters:
//   where - Point in view coordinates.
//   outTX - Receives world tile X coordinate.
//   outTY - Receives world tile Y coordinate.
//
// Returns:
//   true if the point is over the NameTable bitmap; false otherwise.
// -------------------------------------------------------------
bool
NameTableView::ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const
{
	if (!fBitmap) {
		return false;
	}

	BPoint origin = BitmapOrigin();

	float localX = where.x - origin.x;
	float localY = where.y - origin.y;

	const float drawW = 256.0f;
	const float drawH = 240.0f;

	if (localX < 0.0f || localY < 0.0f || localX >= drawW || localY >= drawH) {
		return false;
	}

	int32 localTileX = static_cast<int32>(localX) >> 3;
	int32 localTileY = static_cast<int32>(localY) >> 3;

	if (localTileX < 0 || localTileX >= 32 || localTileY < 0 || localTileY >= 30) {
		return false;
	}

	int32 originTX;
	int32 originTY;
	NameTableTileOrigin(fWhichNameTable, originTX, originTY);

	outTX = originTX + localTileX;
	outTY = originTY + localTileY;

	return true;
}


// -------------------------------------------------------------
// NameTableView::PatternBase
//
// Returns the background pattern table base represented by the current
// NameTable display snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   $0000 or $1000, depending on PPUCTRL bit 4.
// -------------------------------------------------------------
uint32
NameTableView::PatternBase() const
{
	const uint8 ppuctrl = DisplayPPUCTRL();

	return (ppuctrl & 0x10) << 8;
}


// -----------------------------------------------------------------------------
// NameTableView::DrawNameTable
//
// Renders a complete 32x30 NameTable into the backing bitmap using the current
// background pattern table and palette/attribute information.
//
// This function is responsible only for bitmap rendering.  CHR Explorer and
// cross-view debugger synchronization are handled by the view's input,
// attachment, connection, and Pulse paths so drawing does not create duplicate
// debugger updates or window-locking side effects.
//
// Parameters:
//   which - NameTable index, 0-3.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::DrawNameTable (int32 which)
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		return;
	}

	fWhichNameTable = which;

	/*
	 * NameTable bases:
	 *
	 *   0 -> $2000
	 *   1 -> $2400
	 *   2 -> $2800
	 *   3 -> $2C00
	 */
	const uint32 baseAddr = NameTableBaseFromIndex(which);
	fCurrentNameTableBase = baseAddr;

	const uint32 patternBase = PatternBase();

	/*
	 * Render the complete 32x30 NameTable.
	 */
	for (int32 tileY = 0; tileY < 30; tileY++) {
		for (int32 tileX = 0; tileX < 32; tileX++) {
			const uint32 ntAddr = baseAddr + tileX + (tileY * 32);
			const uint8 tileIndex = DisplayVRAM(ntAddr);
			const uint8 palette = PaletteForAttribute(baseAddr, tileX, tileY);

			DrawTile(patternBase, tileIndex, tileX, tileY, palette);
		}
	}
}


// -------------------------------------------------------------
// NameTableView::ColorForPixel
//
// Converts a decoded 2bpp tile pixel and background palette number
// into a host CMAP8 palette index.
//
// Parameters:
//   palette - Background palette number, 0-3.
//   pixel   - Decoded tile pixel value, 0-3.
//
// Returns:
//   Host palette index suitable for writing into the BBitmap.
// -------------------------------------------------------------
uint8
NameTableView::ColorForPixel (uint8 palette, uint8 pixel)
{
    Mapper *mapper = nes::cart.mapper();
    
	if (!mapper || !fPalette) {
		return 0;
	}

	// pixel 0 always uses universal background color ($3f00)
	if (pixel == 0) {
		uint8 bg = DisplayVRAM(0x3f00) & 0x3f;
		return fPalette[bg];
	}

	// bg palettes: $3f01..$3f0f (mirrors handled by mapper)
	uint32 addr = 0x3f00 + 1 + (palette * 4) + (pixel - 1);
	uint8 index = DisplayVRAM(addr) & 0x3f;
	
	return fPalette[index];
}


// -------------------------------------------------------------
// NameTableView::PaletteForAttribute
//
// Reads the attribute table and extracts the palette for a tile.
//
// Parameters:
//   nameTableBase - Base PPU address of the NameTable.
//   tileX         - Local tile X coordinate, 0-31.
//   tileY         - Local tile Y coordinate, 0-29.
//
// Returns:
//   Background palette number, 0-3.
// -------------------------------------------------------------
uint8
NameTableView::PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY)
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return 0;
	}

	// attribute byte address: base + 0x3C0 + (tileY/4)*8 + (tileX/4)
	uint32 attrAddr = nameTableBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
	uint8 attrByte = DisplayVRAM(attrAddr);

    // quadrant inside 4x4 tile region
	uint8 qx = ((tileX % 4) / 2); // 0 or 1
	uint8 qy = ((tileY % 4) / 2); // 0 or 1
	uint8 quadrant = ((qy << 1) | qx); // 0-3
	int32 shift = quadrant * 2;

	return (attrByte >> shift) & 0x3;
}


// -------------------------------------------------------------
// NameTableView::DrawTile
//
// Decodes and draws one 8x8 tile into the NameTable backing bitmap.
//
// Parameters:
//   patternBase - Background pattern table base address.
//   tileIndex   - Tile index from the NameTable byte.
//   tileX       - Local tile X position in the NameTable.
//   tileY       - Local tile Y position in the NameTable.
//   palette     - Background palette number for this tile.
//
// Returns:
//   None.
// -------------------------------------------------------------
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
		uint8 firstPlane = DisplayVRAM(tileAddr + y);
		uint8 secondPlane = DisplayVRAM(tileAddr + y + 8);

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			uint8 color = ColorForPixel(palette, pixel);
			
			DrawPixel(x + tileX * 8, y + tileY * 8, color);
		}
	}
}


// -------------------------------------------------------------
// NameTableView::DrawAttributeGrid
//
// Draws 16x16 attribute quadrant guide lines and 32x32 attribute-byte
// boundary lines over the NameTable bitmap.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
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

	rgb_color quadColor = { 0, 255, 255, 80 };
	rgb_color attrColor = { 255, 255, 0, 180 };

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


// -----------------------------------------------------------------------------
// NameTableView::UpdateCHRExplorer
//
// Updates the CHR Explorer, Palette debugger highlight, and Pattern Table
// external highlight from the currently active NameTable tile.
//
// Tile, attribute, and CHR data are read through DisplayVRAM() so all debugger
// views use the same live/frozen NameTable state.
//
// The exact 16 CHR bytes used by this NameTable tile are also supplied to the
// corresponding PatternTableView.  This keeps the externally highlighted tile
// synchronized on cartridges that switch CHR banks while rendering.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::UpdateCHRExplorer()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper || !fCHRExplorer) {
		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	int32 worldTX = -1;
	int32 worldTY = -1;

	if (!ActiveTile(worldTX, worldTY)) {
		fCHRExplorer->Clear();

		ClearPatternWindowHighlight(fPatternTable0);
		ClearPatternWindowHighlight(fPatternTable1);

		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	int32 tileX = -1;
	int32 tileY = -1;

	if (!WorldTileToLocalTile(worldTX, worldTY, tileX, tileY)) {
		fCHRExplorer->Clear();
		
		ClearPatternWindowHighlight(fPatternTable0);
		ClearPatternWindowHighlight(fPatternTable1);

		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	uint32 nameBase = 0x2000 + (fWhichNameTable * 0x400);

	fCurrentNameTableBase = nameBase;
	fHoverWhichNameTable = fWhichNameTable;

	uint32 tileAddr = nameBase + (tileY * 32) + tileX;
	fHoverTileIndex = DisplayVRAM(tileAddr);
	uint32 attrAddr = nameBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);
	fHoverAttrAddr = attrAddr;
	fHoverAttrByte = DisplayVRAM(attrAddr);

	uint8 qx = (tileX % 4) / 2;
	uint8 qy = (tileY % 4) / 2;
	uint8 quadrant = (qy << 1) | qx;
	int32 shift = quadrant * 2;

	fHoverPalette = (fHoverAttrByte >> shift) & 0x3;

	if (fMainWindow) {
		fMainWindow->HighlightPaletteDebugger(false, fHoverPalette, -1);
	}

	uint32 patternBase = PatternBase();
	fCHRTileAddress = patternBase + (fHoverTileIndex * 16);

	for (int32 i = 0; i < 16; i++) {
		fCHRBytes[i] = DisplayVRAM((fCHRTileAddress + i) & 0x3fff);
	}

	int32 whichPT = (patternBase != 0) ? 1 : 0;

	if (whichPT == 0) {
		SetPatternWindowHighlight(fPatternTable0, 0, fHoverTileIndex, fCHRBytes);
		ClearPatternWindowHighlight(fPatternTable1);
	} else {
		SetPatternWindowHighlight(fPatternTable1, 1, fHoverTileIndex, fCHRBytes);
		ClearPatternWindowHighlight(fPatternTable0);
	}

	NotifyCHRExplorer();
}


// -------------------------------------------------------------
// NameTableView::DrawScrolledViewport
//
// Renders the PPU-scrolled 256x240 viewport into the backing bitmap.
// This helper is retained for viewport-style rendering paths.
//
// Parameters:
//   scrollX - PPU scroll X position, 0-511.
//   scrollY - PPU scroll Y position, 0-479.
//
// Returns:
//   None.
// -------------------------------------------------------------
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
			uint8 tileIndex = DisplayVRAM(nameAddr);

			uint32 attrAddr = ntBase + 0x3c0 + (tileY / 4) * 8 + (tileX / 4);
			uint8 attrByte = DisplayVRAM(attrAddr);

			int32 shift = ((tileY & 0x2) << 1) | (tileX & 0x2);
			uint8 palette = (attrByte >> shift) & 0x3;

			uint32 chrAddr = patternBase + (tileIndex * 16);
			uint8 firstPlane = DisplayVRAM(chrAddr + fineY);
			uint8 secondPlane = DisplayVRAM(chrAddr + fineY + 8);

			shift = 7 - fineX;
			uint8 pixel = (firstPlane >> shift) & 0x1; 
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			uint8 color = ColorForPixel(palette, pixel);
			
			DrawPixel(screenX, screenY, color);
		}
	}
}


// -------------------------------------------------------------
// NameTableView::DrawPPUViewportOverlay
//
// Draws the current 256x240 PPU viewport rectangle over the visible
// NameTable, including wrapped pieces and an origin marker.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawPPUViewportOverlay()
{
	if (!fShowViewportBox) {
		return;
	}

	auto strokeViewportInside = [&](BRect r) {
		PushState();

		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		// Dark backing lines for contrast against bright game colors.
		SetHighColor(0, 0, 0, 170);
		StrokeRect(r);

		BRect r1 = r.InsetByCopy(1.0f, 1.0f);
		if (r1.IsValid()) {
			SetHighColor(0, 0, 0, 120);
			StrokeRect(r1);
		}

		// Bright viewport lines drawn over the contrast backing.
		SetHighColor(255, 255, 0);
		StrokeRect(r);

		if (r1.IsValid()) {
			SetHighColor(255, 255, 0, 230);
			StrokeRect(r1);
		}

		BRect r2 = r.InsetByCopy(2.0f, 2.0f);
		if (r2.IsValid()) {
			SetHighColor(255, 255, 255, 150);
			StrokeRect(r2);
		}

		PopState();
	};

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

			int32 visL = std::max(pieceL, static_cast<int32>(0));
			int32 visT = std::max(pieceT, static_cast<int32>(0));
			int32 visR = std::min(pieceR, static_cast<int32>(256));
			int32 visB = std::min(pieceB, static_cast<int32>(240));

			if (visL >= visR || visT >= visB)
				continue;

			BRect r(static_cast<float>(visL), static_cast<float>(visT),
					static_cast<float>(visR - 1), static_cast<float>(visB - 1));

			// Keep the viewport box slightly inside the bitmap so the
			// top/bottom edges do not visually merge with the bitmap border.
			r.InsetBy(2.0f, 2.0f);

			if (r.IsValid()) {
				strokeViewportInside(r);
			}
		}
	}

	// Mark viewport origin inside the local wrapped view.
	float ox = static_cast<float>(fScrollX % 512);
	float oy = static_cast<float>(fScrollY % 480);

	while (ox >= 256.0f) {
		ox -= 512.0f;
	}

	while (oy >= 240.0f) {
		oy -= 480.0f;
	}

	if (ox >= 0.0f && ox < 256.0f && oy >= 0.0f && oy < 240.0f) {
		SetHighColor(255, 255, 0, 220);
		StrokeLine(BPoint(ox - 4.0f, oy), BPoint(ox + 4.0f, oy));
		StrokeLine(BPoint(ox, oy - 4.0f), BPoint(ox, oy + 4.0f));
	}
}


// -------------------------------------------------------------
// NameTableView::ActiveTile
//
// Returns the current active world tile, respecting hover, lock, and
// screen-position lock modes.
//
// Parameters:
//   outTX - Receives active world tile X coordinate.
//   outTY - Receives active world tile Y coordinate.
//
// Returns:
//   true if an active tile exists; false otherwise.
// -------------------------------------------------------------
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


// -------------------------------------------------------------
// NameTableView::DrawOverlays
//
// Draws the basic hover or lock outline for the active tile when it is
// visible in this NameTable view.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawOverlays()
{
	int32 drawTileX = -1;
	int32 drawTileY = -1;

	if (fTileLocked && !fLockToScreen) {
		if (!WorldTileToLocalTile(fLockedTileX, fLockedTileY, drawTileX, drawTileY))
			return;
	} else if (!fTileLocked) {
		if (!WorldTileToLocalTile(fHoverTileX, fHoverTileY, drawTileX, drawTileY))
			return;
	} else {
		return;
	}

	BRect r(drawTileX * 8, drawTileY * 8, (drawTileX * 8) + 7, (drawTileY * 8) + 7);

	if (fTileLocked) {
		SetHighColor(255, 0, 255);
	} else {
		SetHighColor(0, 255, 255);
	}

	StrokeRect(r);
}


// -------------------------------------------------------------
// NameTableView::CellRectForTile
//
// Converts a world tile coordinate into a local bitmap-space cell rect.
//
// Parameters:
//   tileX - World tile X coordinate.
//   tileY - World tile Y coordinate.
//
// Returns:
//   Bitmap-space rectangle for the tile, or an offscreen rect if hidden.
// -------------------------------------------------------------
BRect
NameTableView::CellRectForTile (int32 tileX, int32 tileY) const
{
	int32 originTX, originTY;
	NameTableTileOrigin(fWhichNameTable, originTX, originTY);

	int32 localTileX = tileX - originTX;
	int32 localTileY = tileY - originTY;

	if (localTileX < 0 || localTileX >= 32 || localTileY < 0 || localTileY >= 30) {
		return BRect(-1000.0f, -1000.0f, -999.0f, -999.0f);
	}

	float x = localTileX * 8.0f;
	float y = localTileY * 8.0f;

	return BRect(x, y, x + 7.0f, y + 7.0f);
}

// -------------------------------------------------------------
// NameTableView::DrawMatchingTileOverlay
//
// Highlights all visible NameTable cells that contain the same tile index
// as the active tile.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
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

	int32 worldTileX = -1;
	int32 worldTileY = -1;

	if (fTileLocked) {
		if (fLockToScreen) {
			if (!ComputeTileFromViewPoint(fLockedViewPoint, worldTileX, worldTileY))
				return;
		} else {
			worldTileX = fLockedTileX;
			worldTileY = fLockedTileY;
		}
	} else {
		worldTileX = fHoverTileX;
		worldTileY = fHoverTileY;
	}

	if (worldTileX < 0 || worldTileY < 0) {
		return;
	}

	int32 localTileX = -1;
	int32 localTileY = -1;

	if (!WorldTileToLocalTile(worldTileX, worldTileY, localTileX, localTileY)) {
		return;
	}

	uint32 nameTableBase = 0x2000 + (fWhichNameTable * 0x400);
	uint32 selectedTileAddr = nameTableBase + (localTileY * 32) + localTileX;
	uint8 selectedTile = DisplayVRAM(selectedTileAddr);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	rgb_color fillColor = {255, 255, 0, 14};
	rgb_color strokeColor = {255, 255, 0, 110};
	rgb_color activeStroke = {255, 0, 255, 200};

	for (int32 ty = 0; ty < 30; ty++) {
		for (int32 tx = 0; tx < 32; tx++) {
			uint32 tileAddr = nameTableBase + (ty * 32) + tx;
			uint8 tile = DisplayVRAM(tileAddr);

			if (tile != selectedTile) {
				continue;
			}

			BRect r(tx * 8.0f, ty * 8.0f, (tx * 8.0f) + 7.0f, (ty * 8.0f) + 7.0f);

			SetHighColor(fillColor);
			FillRect(r);

			SetHighColor(strokeColor);
			StrokeRect(r);
		}
	}

	BRect activeRect(localTileX * 8.0f, localTileY * 8.0f, 
					(localTileX * 8.0f) + 7.0f, (localTileY * 8.0f) + 7.0f);

	SetHighColor(activeStroke);
	StrokeRect(activeRect);
	StrokeRect(activeRect.InsetByCopy(-1.0f, -1.0f));

	PopState();
}


// -------------------------------------------------------------
// NameTableView::NotifyCHRExplorer
//
// Sends the active tile's CHR bytes, palette, NameTable address, and
// attribute information to the connected CHR explorer.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::NotifyCHRExplorer()
{
	if (!fCHRExplorer) {
		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	int32 worldTX = -1;
	int32 worldTY = -1;

	if (!ActiveTile(worldTX, worldTY)) {
		fCHRExplorer->Clear();

		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	int32 tileX = -1;
	int32 tileY = -1;

	if (!WorldTileToLocalTile(worldTX, worldTY, tileX, tileY)) {
		fCHRExplorer->Clear();

		if (fMainWindow) {
			fMainWindow->ClearPaletteDebuggerHighlight();
		}

		return;
	}

	uint8 qx = (tileX % 4) / 2;
	uint8 qy = (tileY % 4) / 2;
	uint8 quadrant = (qy << 1) | qx;

	int32 whichPT = (PatternBase() != 0) ? 1 : 0;
	uint32 tileAddr = fCurrentNameTableBase + (tileY * 32) + tileX;

	fCHRExplorer->SetTile8x8(whichPT, fHoverTileIndex, fTileLocked, fCHRTileAddress,
							fCHRBytes, fHoverPalette, fHoverWhichNameTable, tileAddr,
							fHoverAttrAddr, fHoverAttrByte, quadrant);
}


// -----------------------------------------------------------------------------
// NameTableView::SetExplorer
//
// Connects this NameTable view to a CHR Explorer and synchronizes the explorer
// with the current NameTable state.
//
// The host palette is supplied first.  If a ROM is already loaded, a valid
// default active tile is established when necessary and the explorer is
// populated immediately so connection order does not require subsequent mouse
// movement or a new PPU frame.
//
// Parameters:
//   explorer - CHRExplorerView to connect to this NameTable view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::SetExplorer (CHRExplorerView *explorer)
{
	fCHRExplorer = explorer;

	if (!fCHRExplorer) {
		return;
	}

	if (fPalette) {
		fCHRExplorer->SetHostPalette(fPalette);
	}

	if (!nes::cart.mapper()) {
		fCHRExplorer->Clear();
		return;
	}

	/*
	 * The explorer may be connected after AttachedToWindow(), so ensure
	 * that a valid world-space active tile exists here as well.
	 */
	if (!fTileLocked && (fHoverTileX < 0 || fHoverTileY < 0)) {
		int32 originTX = 0;
		int32 originTY = 0;

		NameTableTileOrigin(fWhichNameTable, originTX, originTY);
		fHoverTileX = originTX;
		fHoverTileY = originTY;
	}

	UpdateCHRExplorer();
}


// -------------------------------------------------------------
// NameTableView::BitmapOrigin
//
// Returns the view-space origin where the 256x240 NameTable bitmap is
// drawn below the controls panel.
//
// Parameters:
//   None.
//
// Returns:
//   View-space top-left point for the NameTable bitmap.
// -------------------------------------------------------------
BPoint
NameTableView::BitmapOrigin() const
{
	const float headerH = 148.0f;

	return BPoint(12.0f, headerH + 10.0f);
}

// -----------------------------------------------------------------------------
// NameTableView::Clear
//
// Clears all ROM-specific NameTable debugger state while preserving the
// debugger window's identity and user-selected display options.
//
// Hover and lock state, cached tile and attribute information, PPU scroll/frame
// state, the cached NameTable bitmap, CHR Explorer contents, PatternTable
// highlights, and Palette debugger highlighting are cleared.
//
// Display options such as attribute grid/map, matching tiles, viewport,
// follow mode, freeze mode, and attribute blocks are intentionally preserved.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::Clear()
{
	fHoverTileX = -1;
	fHoverTileY = -1;

	fTileLocked = false;
	fLockedTileX = -1;
	fLockedTileY = -1;

	fLockToScreen = false;
	fLockedViewPoint = BPoint(-1.0f, -1.0f);

	fCurrentNameTableBase = NameTableBaseFromIndex(fWhichNameTable);

	fHoverTileIndex = 0;
	fHoverPalette = 0;
	fHoverAttrByte = 0;
	fHoverAttrAddr = 0;
	fHoverWhichNameTable = -1;

	fCHRTileAddress = 0;

	memset(fCHRBytes, 0, sizeof(fCHRBytes));

	fScrollX = 0;
	fScrollY = 0;

	/*
	 * Force the first PPU frame of the next ROM to be treated as new.
	 */
	fLastPPUFrame = static_cast<uint64>(-1);

	/*
	 * The backing bitmap is explicitly cleared here, so it does not need
	 * rebuilding while no ROM is loaded.  The first new PPU frame will mark
	 * it dirty again after another ROM is loaded.
	 */
	fBitmapDirty = false;

	if (fBitmap && fBits) {
		memset(fBits, 0, fBitmap->BitsLength());
	}

	/*
	 * This NameTable view may currently be driving highlights in either
	 * PatternTable window.
	 */
	ClearPatternWindowHighlight(fPatternTable0);
	ClearPatternWindowHighlight(fPatternTable1);

	if (fMainWindow) {
		fMainWindow->ClearPaletteDebuggerHighlight();
	}

	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}
	
	fHavePPUSnapshot = false;
	fSnapshotPPUCTRL = 0x00;
	memset(fSnapshotVRAM, 0, sizeof(fSnapshotVRAM));

	Invalidate();
}


// -------------------------------------------------------------
// NameTableView::DrawActiveAttributeBlockOverlay
//
// Draws the active 2x2-tile attribute quadrant overlay around the
// current hover or locked tile.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawActiveAttributeBlockOverlay()
{
	int32 worldTX, worldTY;

	if (!ActiveTile(worldTX, worldTY)) {
		return;
	}

	int32 originTX, originTY;
	NameTableTileOrigin(fWhichNameTable, originTX, originTY);

	int32 localTX = worldTX - originTX;
	int32 localTY = worldTY - originTY;

	if (localTX < 0 || localTX >= 32 || localTY < 0 || localTY >= 30) {
		return;
	}

	int32 quadTileX = (localTX / 2) * 2;
	int32 quadTileY = (localTY / 2) * 2;

	float x = quadTileX * 8.0f;
	float y = quadTileY * 8.0f;

	BRect r(x, y, x + 15.0f, y + 15.0f);

	rgb_color fill;
	rgb_color stroke;

	if (fTileLocked) {
		fill = (rgb_color){ 255, 190, 80,  26 };  // locked: soft amber
		stroke = (rgb_color){ 255, 150, 40, 150 };
	} else {
		fill = (rgb_color){ 80, 200, 255, 22};  // hover: soft cyan
		stroke = (rgb_color){ 40, 160, 220, 140 };
	}

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	SetHighColor(fill);
	FillRect(r);

	SetHighColor(stroke);
	StrokeRect(r);
	StrokeRect(r.InsetByCopy(1, 1));

	PopState();
}


// -----------------------------------------------------------------------------
// NameTableView::DrawAttributeBlockOverlay
//
// Draws optional 16x16 attribute-quadrant tint blocks over the currently
// displayed NameTable.
//
// Each block corresponds to one 2x2-tile attribute quadrant in this specific
// NameTable.  Attribute data is therefore read using fWhichNameTable rather
// than the global PPU scroll position.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::DrawAttributeBlockOverlay()
{
	if (!fShowAttributeBlocks) {
		return;
	}

	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		return;
	}

	const uint32 nameTableBase = NameTableBaseFromIndex(fWhichNameTable);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	/*
	 * One palette quadrant covers 2x2 tiles, or 16x16 pixels.
	 */
	for (int32 tileY = 0; tileY < 30; tileY += 2) {
		for (int32 tileX = 0; tileX < 32; tileX += 2) {
			const uint8 palette = PaletteForAttribute(nameTableBase, tileX, tileY) % 4;

			rgb_color tint;

			switch (palette) {
				case 0:
					tint = (rgb_color){ 255, 80, 80, 34 };
					break;

				case 1:
					tint = (rgb_color){ 80, 180, 255, 34 };
					break;

				case 2:
					tint = (rgb_color){ 120, 255, 120, 34 };
					break;

				default:
					tint = (rgb_color){ 255, 210, 80, 34 };
					break;
			}

			const float x = tileX * 8.0f;
			const float y = tileY * 8.0f;

			/*
			 * The final NameTable row is only 16 pixels tall when tileY
			 * is 28, so this remains inside the 240-pixel bitmap.
			 */
			BRect r(x, y, x + 15.0f, y + 15.0f);

			SetHighColor(tint);
			FillRect(r);

			SetHighColor((rgb_color){0, 0, 0, 32});

			StrokeRect(r);
		}
	}

	PopState();
}


// -------------------------------------------------------------
// NameTableView::DrawAttributeMapOverlay
//
// Tints each 8x8 tile according to the palette selected by the
// NameTable attribute table.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawAttributeMapOverlay()
{
	if (!fShowAttributeMap) {
		return;
	}

	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper) {
		return;
	}

	static const rgb_color kPaletteTint[4] = {
		{ 255, 0, 255, 56 },	// pal 0: magenta
		{ 0, 220, 255, 56 },	// pal 1: cyan
		{ 120, 255, 0, 56 },	// pal 2: lime
		{ 255, 140, 0, 56 }		// pal 3: orange
	};

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	uint32 nameTableBase = 0x2000 + (fWhichNameTable * 0x400);

	for (int32 tileY = 0; tileY < 30; tileY++) {
		for (int32 tileX = 0; tileX < 32; tileX++) {
			uint8 pal = PaletteForAttribute(nameTableBase, tileX, tileY) % 4;

			BRect r(tileX * 8.0f, tileY * 8.0f, (tileX * 8.0f) + 7.0f, (tileY * 8.0f) + 7.0f);
			SetHighColor(kPaletteTint[pal]);
			FillRect(r);
		}
	}

	PopState();
}


// -------------------------------------------------------------
// NameTableView::DrawActiveAttributeCellOverlay
//
// Draws the active 4x4-tile attribute cell and its four quadrant guide
// lines around the active tile.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawActiveAttributeCellOverlay()
{
	int32 worldTX, worldTY;

	if (!ActiveTile(worldTX, worldTY)) {
		return;
	}

	int32 originTX, originTY;
	NameTableTileOrigin(fWhichNameTable, originTX, originTY);

	int32 localTX = worldTX - originTX;
	int32 localTY = worldTY - originTY;

	if (localTX < 0 || localTX >= 32 || localTY < 0 || localTY >= 30)
		return;

	int32 cellTileX = (localTX / 4) * 4;
	int32 cellTileY = (localTY / 4) * 4;

	float x = cellTileX * 8.0f;
	float y = cellTileY * 8.0f;

	BRect r(x, y, x + 31.0f, y + 31.0f);

	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	if (fTileLocked) {
		SetHighColor(255, 190, 80, 20);
	} else {
		SetHighColor(80, 200, 255, 16);
	}
	
	FillRect(r);

	if (fTileLocked) {
		SetHighColor(255, 150, 40, 135);
	} else {
		SetHighColor(40, 160, 220, 120);
	}
	
	StrokeRect(r);
	StrokeRect(r.InsetByCopy(1, 1));

	if (fTileLocked) {
		SetHighColor(255, 150, 40, 85);
	} else {
		SetHighColor(40, 160, 220, 75);
	}

	StrokeLine(BPoint(x + 16.0f, y), BPoint(x + 16.0f, y + 31.0f));
	StrokeLine(BPoint(x, y + 16.0f), BPoint(x + 31.0f, y + 16.0f));

	PopState();
}


// -----------------------------------------------------------------------------
// NameTableView::DrawHeaderUI
//
// Draws the top controls panel that documents the NameTable keyboard
// shortcuts and mouse actions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::DrawHeaderUI()
{
	const float headerH = 148.0f;
	const float panelLeft = 4.0f;
	const float panelTop = 4.0f;
	const float panelRight = Bounds().right - 2.0f;
	const float panelBottom = headerH - 6.0f;

	BRect panel(panelLeft, panelTop, panelRight, panelBottom);
	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 68.0f;
	float y = panel.top + 36.0f;

	auto drawLV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawLV("G/H/B/N:", "grid / map / both / none");
	drawLV("F/M:", "blocks / matching tiles");
	drawLV("V:", "viewport");
	drawLV("Space:", "freeze updates");
	drawLV("Mouse:", "hover / click lock / shift screen lock");
	drawLV("CHR:", "1-4 palette / 0 source");
}


// -------------------------------------------------------------
// NameTableView::WorldTileToLocalTile
//
// Converts a world tile coordinate into a local coordinate for this
// visible NameTable.
//
// Parameters:
//   worldTX    - World tile X coordinate.
//   worldTY    - World tile Y coordinate.
//   outLocalTX - Receives local tile X coordinate.
//   outLocalTY - Receives local tile Y coordinate.
//
// Returns:
//   true if the world tile is visible in this NameTable; false otherwise.
// -------------------------------------------------------------
bool
NameTableView::WorldTileToLocalTile (int32 worldTX, int32 worldTY, int32 &outLocalTX, int32 &outLocalTY) const
{
	int32 originTX;
	int32 originTY;
	
	NameTableTileOrigin(fWhichNameTable, originTX, originTY);
	int32 localTX = worldTX - originTX;
	int32 localTY = worldTY - originTY;

	if (localTX < 0 || localTX >= 32 || localTY < 0 || localTY >= 30) {
		return false;
	}

	outLocalTX = localTX;
	outLocalTY = localTY;

	return true;
}


// -----------------------------------------------------------------------------
// NameTableView::DrawDebugPanel
//
// Draws the bottom NameTable state panel, including aligned PPU state
// fields and overlay-state badges.
//
// Numeric PPU state values are displayed using a fixed-width font so
// hexadecimal register and address values remain consistently aligned.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::DrawDebugPanel()
{
	BPoint origin = BitmapOrigin();

	const float panelLeft = 4.0f;
	const float panelRight = Bounds().right - 8.0f;
	const float panelTop = origin.y + 240.0f + 10.0f;
	const float panelBottom = Bounds().bottom - 8.0f;

	BRect panel(panelLeft, panelTop, panelRight, panelBottom);
	::DrawDebugPanel(this, panel, "Name Table State");

	SetFontSize(11.0f);

	BFont uiFont;
	GetFont(&uiFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(uiFont.Size());

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 84.0f;

	float textY = panel.top + 36.0f;

	uint8 ctrl = DisplayPPUCTRL();
	uint16 bgPT = (ctrl & 0x10) ? 0x1000 : 0x0000;
	uint32 ntBase = NameTableBaseFromIndex(fWhichNameTable);

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

	s.SetToFormat("$%02X", ctrl);
	drawLV("PPUCTRL:", s.String(), true);

	s.SetToFormat("$%04X", bgPT);
	drawLV("Background:", s.String(), true);

	s.SetToFormat("$%04X", ntBase);
	drawLV("Name Table:", s.String(), true);

	/*
	 * State badges with automatic wrapping.
	 */
	SetFont(&uiFont);

	float sx = labelX;
	float sy = textY + 5.0f;

	const float maxW = panel.right - 8.0f;

	auto drawStateBadge = [&](const char *label, bool enabled) {
		const float padX = 5.0f;
		const float badgeH = 14.0f;
		const float gap = 5.0f;

		float textW = StringWidth(label);
		float badgeW = textW + (padX * 2.0f);

		if (sx + badgeW > maxW) {
			sx = labelX;
			sy += badgeH + 4.0f;
		}

		BRect r(sx, sy, sx + badgeW, sy + badgeH);

		if (enabled) {
			SetHighColor(210, 235, 210);
		} else {
			SetHighColor(224, 224, 224);
		}

		FillRect(r);

		if (enabled) {
			SetHighColor(80, 150, 80);
		} else {
			SetHighColor(165, 165, 165);
		}

		StrokeRect(r);

		if (enabled) {
			SetHighColor(20, 90, 20);
		} else {
			SetHighColor(105, 105, 105);
		}

		DrawString(label, BPoint(sx + padX, sy + 11.0f));

		sx += badgeW + gap;
	};

	drawStateBadge("GRID", fShowAttributeGrid);
	drawStateBadge("MAP", fShowAttributeMap);
	drawStateBadge("BLK", fShowAttributeBlocks);
	drawStateBadge("MATCH", fShowMatchingTiles);
	drawStateBadge("VIEW", fShowViewportBox);
	drawStateBadge("FREEZE", fFreezeUpdates);

	SetFont(&uiFont);
}


// -------------------------------------------------------------
// NameTableView::DrawBitmapPanel
//
// Draws the framed panel behind the 256x240 NameTable bitmap.
//
// Parameters:
//   origin - View-space origin of the NameTable bitmap.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawBitmapPanel (BPoint origin)
{
	BRect panel(origin.x - 5.0f, origin.y - 5.0f, origin.x + 255.0f + 5.0f, origin.y + 239.0f + 5.0f);

	SetHighColor(228, 228, 228);
	FillRect(panel);

	SetHighColor(150, 150, 150);
	StrokeRect(panel);
}


// -------------------------------------------------------------
// NameTableView::DrawNoROMMessage
//
// Draws a friendly empty-state message inside the NameTable bitmap
// area when no ROM is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   None.
// -------------------------------------------------------------
void
NameTableView::DrawNoROMMessage()
{
	BPoint origin = BitmapOrigin();

	BRect panel(origin.x, origin.y, origin.x + WIDTH - 1.0f, origin.y + HEIGHT - 1.0f);

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
	const char *detail = "Load a cartridge to view name tables.";

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

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// NameTableView::CapturePPUSnapshot
//
// Captures the current PPU control state and a complete snapshot of the PPU
// address space for use by the name-table debugger view.
//
// If no cartridge mapper is available, the snapshot is marked invalid.  When
// successful, PPUCTRL and the full $0000-$3FFF PPU memory range are copied into
// the view's snapshot storage.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableView::CapturePPUSnapshot()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		fHavePPUSnapshot = false;
		return;
	}

	fSnapshotPPUCTRL = nes::ppu::ppuctrl();

	for (uint32 address = 0; address < 0x4000; address++) {
		fSnapshotVRAM[address] = mapper->read_vram(address);
	}

	fHavePPUSnapshot = true;
}


// -----------------------------------------------------------------------------
// NameTableView::DisplayPPUCTRL
//
// Returns the PPUCTRL value represented by the current NameTable display state.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot PPUCTRL when available, otherwise the current live value.
// -----------------------------------------------------------------------------
uint8
NameTableView::DisplayPPUCTRL() const
{
	if (fHavePPUSnapshot) {
		return fSnapshotPPUCTRL;
	}

	return nes::ppu::ppuctrl();
}


// -----------------------------------------------------------------------------
// NameTableView::DisplayVRAM
//
// Reads one byte from the PPU memory state represented by this debugger view.
//
// Once a display snapshot exists, all NameTable rendering and inspection reads
// use that snapshot so overlays and linked debugger tools cannot observe newer
// live VRAM than the bitmap beneath them.
//
// Parameters:
//   address - PPU address to read.
//
// Returns:
//   Snapshot or live PPU-memory byte.
// -----------------------------------------------------------------------------
uint8
NameTableView::DisplayVRAM(uint32 address) const
{
	address &= 0x3fff;

	if (fHavePPUSnapshot) {
		return fSnapshotVRAM[address];
	}

	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		return 0;
	}

	return mapper->read_vram(address);
}

