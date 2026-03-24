#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <Bitmap.h>
#include <View.h>

#include "Cart.h"
#include "Nes.h"
#include "Ppu.h"
#include "PretendoWindow.h"
#include "CHRExplorerView.h"


class PatternTableView : public BView
{
	public:
	typedef enum {
		WIDTH = 128,
		HEIGHT = 128
	} screen_size;

	typedef enum {
		MODE_8x8,
		MODE_8x16
	} view_mode;

	
	public:
			PatternTableView (BRect frame, PretendoWindow *mainWindow, int32 which, CHRExplorerView *explorer);
	virtual ~PatternTableView();

	// inherited from BView
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *msg);
	virtual	void MouseDown (BPoint point);
	virtual void Pulse();
	

	// drawing stuff
	public:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (uint32 patternTable, int32 tileIndex, int32 tileX, int32 tileY);
	void DrawPatternTable8x8 (int32 which);
	void DrawPatternTable8x16 (int32 which);
	
	// chr explorer stuff
	public:
	void UpdateExplorer();
	void DrawExplorer (BView *dest);
	void NotifyCHRExplorer();
	
	public:
	bool ComputeTileFromViewPoint (BPoint where, int32 &outTX, int32 &outTY) const;
	BPoint ViewToBitmap(BPoint where) const;
	BRect CellRectForTileIndex(int32 tileIndex) const;
	int32 TileIndexFromPoint(BPoint where) const;
	void DrawOverlays();
	void DrawHoverBox (int32 tileIndex);
	
	int32 ActiveTileIndex() const;

	void SetViewMode(view_mode vm);

	view_mode ViewMode() const
	{
		return fViewMode;
	}

	bool Show8x16() const
	{
		return fViewMode == MODE_8x16;
	}

	bool SpriteMode8x16() const
	{
		return (nes::ppu::ppuctrl() & 0x20) != 0;
	}
	
	private:
	PretendoWindow *fMainWindow = nullptr;
	CHRExplorerView* fCHRExplorer = nullptr;
	BBitmap *fBitmap = nullptr;
	uint8* fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichPatternTable = 0;
	view_mode fViewMode = MODE_8x8;
	uint8 *fPalette = nullptr;

	int32 fTileSize = 16;
	int32 fHoverTileIndex = -1;
	int32 fLockedTileIndex = -1;

	bool fTileLocked = false;

	uint32 fCHRTileAddress = 0;
	uint8 fCHRBytes[16] = {0};

	bool fExploreEnabled = true;
	BPoint fLastMouse;
	bool fMouseValid = false;

};

#endif


