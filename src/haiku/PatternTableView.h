
#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <PopUpMenu.h>
#include <MenuItem.h>

#include "Cart.h"
#include "Nes.h"


class PatternTableView : public BView
{
	private:
	typedef enum {
		SHOW_8x8 = 	'8x8 ',
		SHOW_8x16 = '8x16'
	} messages;
	
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
	virtual void MessageReceived (BMessage *message);
	virtual void MouseDown (BPoint point);
	virtual void Pulse();
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (int32 patternTable, int32 tileIndex, int32 tileX, int32 tileY);
	void DrawPatternTable8x8 (int32 which);
	void DrawPatternTable8x16 (int32 which);
		
	private:
	BPopUpMenu *fPopUpMenu = nullptr;
	
	private:
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichPatternTable = 0;
	view_mode fViewMode = view_mode::MODE_8x8;
};


#endif //_PATTERN_TABLE_VIEW_H_

