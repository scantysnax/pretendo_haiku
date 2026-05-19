#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <Bitmap.h>
#include <View.h>

#include <algorithm>

#include "Cart.h"
#include "Nes.h"
#include "Ppu.h"
#include "PretendoWindow.h"
#include "CHRExplorerView.h"


// -------------------------------------------------------------
// PatternTableView
//
// BView used by PatternTableWindow to display NES CHR pattern-table
// contents. The view renders either pattern table as a 256x256 bitmap,
// supports normal 8x8 tile layout and 8x16 sprite-pair layout, tracks
// hover/lock selection, and reports the active tile to CHRExplorerView.
// -------------------------------------------------------------

class PatternTableView : public BView
{
	public:
	// Logical unscaled pattern-table dimensions retained for reference.
	typedef enum {
		WIDTH = 128,
		HEIGHT = 128
	} screen_size;

	// Current PatternTable display mode.
	typedef enum {
		MODE_8x8,
		MODE_8x16
	} view_mode;

	public:
	PatternTableView (BRect frame, PretendoWindow* mainWindow, int32 which, CHRExplorerView* explorer);
	virtual ~PatternTableView();

	// inherited from BView
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MouseMoved (BPoint point, uint32 transit, const BMessage* msg);
	virtual void MouseDown (BPoint point);
	virtual void Pulse();

	public:
	// Explorer
	void SetExplorer (CHRExplorerView *explorer);

	// Drawing stuff
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (uint32 patternTable, int32 tileIndex, int32 tileX, int32 tileY);
	void DrawPatternTable8x8 (int32 which);
	void DrawPatternTable8x16 (int32 which);

	// CHR explorer stuff
	private:
	void UpdateExplorer();
	void NotifyCHRExplorer();

	private:
	// Tile stuff
	BRect CellRectForTileIndex (int32 tileIndex) const;
	int32 TileIndexFromPoint (BPoint where) const;
	int32 ActiveTileIndex() const;	
	BPoint BitmapOrigin() const;

	// UI 
	private:
	void DrawOverlays();
	void DrawPatternStatePanel();
	void DrawBitmapPanel (BPoint origin);
	void DrawHeaderUI();

	public:
	// Highlight api
	void SetExternalHighlight (int32 whichPT, int32 tileIndex);
	void ClearExternalHighlight();
	
	public:
	void SetViewMode (view_mode vm);

	// ---------------------------------------------------------
	// ViewMode
	//
	// Returns the current PatternTable display mode.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   MODE_8x8 or MODE_8x16.
	// ---------------------------------------------------------
	view_mode ViewMode() const
	{
		return fViewMode;
	}

	// ---------------------------------------------------------
	// Show8x16
	//
	// Tests whether the PatternTable view is currently in 8x16 mode.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true when current view mode is MODE_8x16; otherwise false.
	// ---------------------------------------------------------
	bool Show8x16() const
	{
		return fViewMode == MODE_8x16;
	}

	// ---------------------------------------------------------
	// SpriteMode8x16
	//
	// Tests whether the NES PPU control register currently selects 8x16
	// sprite mode.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if PPUCTRL bit 5 is set; otherwise false.
	// ---------------------------------------------------------
	bool SpriteMode8x16() const
	{
		return (nes::ppu::ppuctrl() & 0x20) != 0;
	}

	private:
	// Parent/debug view references.
	PretendoWindow *fMainWindow = nullptr;
	CHRExplorerView *fCHRExplorer = nullptr;

	private:
	// Backing bitmap used to render decoded CHR tiles.
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;

	private:
	// PatternTable identity and display mode.
	int32 fWhichPatternTable = 0;
	view_mode fViewMode = MODE_8x8;
	uint8 *fHostPalette = nullptr;
	
	private:
	// Tile geometry and selection state.
	int32 fTileSize = 16;
	int32 fHoverTileIndex = -1;
	int32 fLockedTileIndex = -1;
	bool fTileLocked = false;

	private:
	// Cached CHR data for the active tile.
	uint32 fCHRTileAddress = 0;
	uint8 fCHRBytes[16] = {0};

	private:
	// Last mouse position/state inside this view.
	BPoint fLastMouse;
	bool fMouseValid = false;

	private:
	// External highlight supplied by NameTableView.
	bool fHasExternalHighlight = false;
	int32 fExternalWhichPT = 0;
	int32 fExternalTileIndex = -1;
};

#endif



