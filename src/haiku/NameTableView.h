
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
	
	private:
	uint8 ColorForPixel (uint8 palette, uint8 pixel);
	uint8 PaletteForAttribute (uint32 nameTableBase, int32 tileX, int32 tileY);	
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (uint32 patternTableBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette);
    void DrawNameTable (int32 which);
	
    private:
    PretendoWindow *fMainWindow = nullptr;
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichNameTable = 0;
	uint8 *fPalette = nullptr;
};

#endif
