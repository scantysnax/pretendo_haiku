#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <Bitmap.h>
#include <View.h>

#include "Cart.h"
#include "Nes.h"

#include <iostream>

class PatternTableWindow;

class PatternTableView : public BView
{
	public:
	typedef enum {
		WIDTH = 128,
		HEIGHT = 128
	} screen_size;
	
	public:
	typedef enum {
		MODE_8x8 = 0,
		MODE_8x16 = 1
	} view_mode;
		
	public:
			PatternTableView (BRect frame, int32 which);
	virtual ~PatternTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (int32 patternTable, int32 tileIndex, int32 tileX, int32 tileY);
	void DrawPatternTable8x8 (int32 which);
	void DrawPatternTable8x16 (int32 which);
	
	public:
	void SetViewMode (view_mode vm) {
		fViewMode = vm;
		Invalidate();
	}
	
	public:
	view_mode ViewMode() {	
		return fViewMode;
	}

	private:
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichPatternTable = 0;
	view_mode fViewMode = view_mode::MODE_8x8;
};


#endif //_PATTERN_TABLE_VIEW_H_
