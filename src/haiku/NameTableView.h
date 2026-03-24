#ifndef _NAME_TABLE_VIEW_H_
#define _NAME_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <Message.h>

class PretendoWindow;
class CHRExplorerView;


class NameTableView : public BView
{
	public:
    		NameTableView (BRect frame, PretendoWindow *mainWindow, int32 which, CHRExplorerView *explorer);
    virtual ~NameTableView();

   // inherited from BView
    public:
    virtual void AttachedToWindow();
    virtual void Draw (BRect updateRect);
    virtual void KeyDown (const char *bytes, int32 numBytes);
    virtual void MouseDown (BPoint where);
    virtual void MouseMoved (BPoint where, uint32 transit, const BMessage *message);
    virtual void Pulse();
    
	// this is it.
	void DrawNameTable (int32 which);

    // helpers
	BPoint ViewToBitmap (BPoint where) const;
	bool ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const;
	uint32 GetBgPatternBase() const;
	BRect CellRectForViewPoint (BPoint where) const;
	void DrawAttributeQuadrantOverlay();
	void DrawMatchingTileOverlay();

	private:
	// screen size, probably move to an enum at some point.
	static constexpr int32 WIDTH  = 256;
	static constexpr int32 HEIGHT = 240;

   // bitmap things
    BBitmap *fBitmap = nullptr;
    uint8* fBits = nullptr;
    int32 fRowBytes = 0;

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
	bool fShowTileInfoHUD = false;
	bool fShowViewportBox = true;
	bool fShowExplorer = true;
	bool fShowHelpHUD = true;
	bool fFollowViewport = true;
	bool fFreezeUpdates = false;

    // mouse/hover
	BPoint fLastMouse;
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
	
    // explorer
	private:
	void UpdateExplorer();
	void NotifyCHRExplorer();
	void MaybeUpdateCHRExplorer();
	bool ActiveTile (int32 &outTX, int32 &outTY) const;

	// rendering helpers
	private:
	BRect CellRectForTile (int32 tileX, int32 tileY) const;
	uint8 ColorForPixel (uint8 palette, uint8 pixel);
	void DrawAttributeGrid();
	void DrawAttributeMap (uint32 nameTableBase);
	void DrawFreezeBadge();
	void DrawHelpHUD();
	void DrawMatchingTileLegend();
	void DrawOverlays();
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawPPUViewportOverlay();
	void DrawScrolledViewport (int32 scrollX, int32 scrollY);
	void DrawTile (uint32 patternBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette);
	void DrawTileInfoHUD();
	uint8 PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY);	
};

#endif
