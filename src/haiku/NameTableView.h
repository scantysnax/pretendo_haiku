
#ifndef _NAMETABLE_VIEW_H_
#define _NAMETABLE_VIEW_H_

#include <Bitmap.h>
#include <View.h>

#include "Cart.h"
#include "Nes.h"
#include "Ppu.h"

#include "PretendoWindow.h"


class PretendoWindow;


class NameTableView : public BView
{
	public:
	typedef enum {
		HEIGHT = 30*8,
		WIDTH = 32*8
	} nametable_size;
		
	
	public:
			NameTableView (BRect frame, PretendoWindow *mainWindow, int32 which);
	virtual ~NameTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	virtual void KeyDown (const char* bytes, int32 numBytes);
	virtual void MouseMoved (BPoint where, uint32 transit, const BMessage* msg);
	virtual void MouseDown (BPoint where);
	virtual void WindowActivated(bool active);
	
	private:
	uint8 ColorForPixel (uint8 palette, uint8 pixel);
	uint8 PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY);	
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (uint32 patternTableBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette);
    void DrawNameTable (int32 which);
    void DrawAttributeGrid();
	void DrawAttributeMap (uint32 nameTable);
	void UpdateInspector();
	void DrawHoverBox();
	void DrawInspectorText();
	void LockTile(int32 tileX, int32 tileY);
	void UnlockTile() {
    	fTileLocked = false;
	}
	
    private:
    PretendoWindow *fMainWindow = nullptr;
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichNameTable = 0;
	uint8 *fPalette = nullptr;
	
	bool fShowAttributeGrid = false;
	bool fShowAttributeMap = false;
	bool fShowInspector = true;

	int32 fHoverTileX = -1;
	int32 fHoverTileY = -1;
	int32 fHoverTileIndex = 0;
	uint8 fHoverAttrByte = 0;
	uint32 fHoverAttrAddr = 0;
	uint8 fHoverPalette = 0;
	BPoint fLastMouse;
	bool fMouseValid = false;
	bool fTileLocked = false;
	int32 fLockedTileX = -1;
	int32 fLockedTileY = -1;
	bool fViewActive = false;
};

#endif
