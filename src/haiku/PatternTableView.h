
#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <PopUpMenu.h>
#include <MenuItem.h>


constexpr int32 kPatternTableWidth = 128;
constexpr int32 kPatternTableHeight = 128;


class PatternTableView : public BView
{
	public:
	PatternTableView (BRect frame, int32 which);
	virtual ~PatternTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	virtual void MouseDown(BPoint point);
	
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
	int32 fViewMode = 0;
};


#endif //_PATTERN_TABLE_VIEW_H_

