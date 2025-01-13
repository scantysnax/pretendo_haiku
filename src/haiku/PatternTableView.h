
#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>


class PatternTableView : public BView
{
	public:
	PatternTableView (BRect frame, uint32 address);
	virtual ~PatternTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	public:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawPatternTable (uint8 *chr_rom);
	
	private:
	uint32 fAddress = 0x0;
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	uint8 *fChrRom = nullptr;
};


#endif //_PATTERN_TABLE_VIEW_H_

