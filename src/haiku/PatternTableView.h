
#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>


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
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	
	public:
	void DrawPatternTable (int32 which);
	
	private:
	uint32 fAddress = 0x0;
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichPatternTable = 0;
};


#endif //_PATTERN_TABLE_VIEW_H_

