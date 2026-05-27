#ifndef _NAME_TABLE_VIEW_H_
#define _NAME_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <Message.h>

#include <algorithm>

class PretendoWindow;
class CHRExplorerView;
class PatternTableWindow;


// -------------------------------------------------------------
// NameTableView
//
// Debugger view for one NES NameTable. The view renders the selected
// NameTable into a 256x240 bitmap, draws debug overlays, tracks hover
// and locked tiles, and forwards selected tile data to CHRExplorerView
// and PatternTableView.
// -------------------------------------------------------------

class NameTableView : public BView
{
	public:
    		NameTableView (BRect frame, PretendoWindow *mainWindow, int32 which, CHRExplorerView *explorer);
    virtual ~NameTableView();
    
    // inherited from BView
    public:
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void Draw (BRect updateRect);
    virtual void KeyDown (const char *bytes, int32 numBytes);
    virtual void MouseDown (BPoint where);
    virtual void MouseMoved (BPoint where, uint32 transit, const BMessage *message);
    virtual void Pulse();
    
    private:
	void DrawNameTable (int32 which);

	private:
    // helpers
	bool ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const;
	uint32 PatternBase() const;
	void DrawMatchingTileOverlay();
	
	public:
	// -------------------------------------------------------------
	// SetPatternTables
	//
	// Connects this NameTable view to the two PatternTable windows so
	// active tiles can be highlighted there.
	//
	// Parameters:
	//   pt0 - Pattern table window for $0000.
	//   pt1 - Pattern table window for $1000.
	//
	// Returns:
	//   None.
	// -------------------------------------------------------------
	void SetPatternTables (PatternTableWindow *pt0, PatternTableWindow *pt1) {
		fPatternTable0 = pt0;
		fPatternTable1 = pt1;
	}
	
	private: 
	// screen size, probably move to an enum at some point.
	static constexpr int32 WIDTH  = 256;
	static constexpr int32 HEIGHT = 240;

   // bitmap things
    BBitmap *fBitmap = nullptr;
    uint8 *fBits = nullptr;
    int32 fRowBytes = 0;
    bool fBitmapDirty = true;

	// app stuff
	PretendoWindow *fMainWindow = nullptr;
	CHRExplorerView *fCHRExplorer = nullptr;

	// name table
	int32 fWhichNameTable = 0;
	uint32 fCurrentNameTableBase = 0x2000;

	// debug things
	bool fShowAttributeGrid = false;
	bool fShowAttributeMap = false;
	bool fShowMatchingTiles = false;
	bool fShowViewportBox = true;
	bool fFollowViewport = true;
	bool fFreezeUpdates = false;

    // mouse/hover
	int32 fHoverTileX = -1;
	int32 fHoverTileY = -1;

	// lock
	bool fTileLocked = false;
	int32 fLockedTileX = -1;
	int32 fLockedTileY = -1;
	
	// optional screen-position lock
	bool fLockToScreen = false;
	BPoint fLockedViewPoint;

	// explorer stuff we need
	uint8 fHoverTileIndex = 0;     // nametable byte for hovered tile
	uint8 fHoverPalette = 0;       // 0-3 from attribute table
	uint8 fHoverAttrByte = 0;
	uint32 fHoverAttrAddr = 0;
	
	uint32 fCHRTileAddress = 0;
	uint8 fCHRBytes[16] = {0};

    // host palette (NES to 8-bit cmap index)
	uint8 *fPalette = nullptr;

	int32 fScrollX = 0;   // 0-511
	int32 fScrollY = 0;   // 0-479
	int32 fHoverWhichNameTable = -1;
	uint64 fLastPPUFrame = (uint64)-1;
	
	// pattern tables
	PatternTableWindow *fPatternTable0 = nullptr;
 	PatternTableWindow *fPatternTable1 = nullptr;
	
    // explorer
	private:
	// Rebuilds active tile data from PPU memory.
	void UpdateCHRExplorer();

	// Sends active tile data to CHRExplorerView.
	void NotifyCHRExplorer();

	// Updates the CHR explorer when one is attached.
	void MaybeUpdateCHRExplorer();

	// Returns the active world tile, respecting hover/lock modes.
	bool ActiveTile (int32 &outTX, int32 &outTY) const;

	// rendering helpers
	private:
	// Converts a world tile to a local bitmap-space rectangle.
	BRect CellRectForTile (int32 tileX, int32 tileY) const;

	// Converts a decoded NES pixel into a host palette index.
	uint8 ColorForPixel (uint8 palette, uint8 pixel);

	// Draws attribute quadrant and attribute-byte guide lines.
	void DrawAttributeGrid();

	// Draws hover/lock tile outlines.
	void DrawOverlays();

	// Writes one pixel into the backing bitmap.
	void DrawPixel (int32 x, int32 y, uint8 color);

	// Draws the current PPU viewport rectangle overlay.
	void DrawPPUViewportOverlay();

	// Draws a scrolled viewport image into the backing bitmap.
	void DrawScrolledViewport (int32 scrollX, int32 scrollY);

	// Draws one 8x8 tile into the backing bitmap.
	void DrawTile (uint32 patternBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette);

	// Draws the active 2x2 attribute quadrant overlay.
	void DrawActiveAttributeBlockOverlay();

	// Reads the palette selected by the NameTable attribute data.
	uint8 PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY);

	// Converts a world tile into this view's local tile coordinates.
	bool WorldTileToLocalTile (int32 worldTX, int32 worldTY, int32 &outLocalTX, int32 &outLocalTY) const;
	
	public:
	void SetExplorer (CHRExplorerView *explorer);
	BPoint BitmapOrigin() const;
	
	private:
	// Draws optional 16x16 attribute block tint overlays.
	void DrawAttributeBlockOverlay();
	bool fShowAttributeBlocks = false;

	private:
	// Draws the active 4x4 attribute-cell overlay.
	void DrawActiveAttributeCellOverlay();

	// Draws palette tint overlay for every visible tile.
	void DrawAttributeMapOverlay();
	
	// Draws the top controls panel.
	void DrawHeaderUI();

	// Draws the bottom NameTable state panel.
	void DrawDebugPanel();

	// Draws the framed panel behind the bitmap.
	void DrawBitmapPanel (BPoint origin);
};

#endif


