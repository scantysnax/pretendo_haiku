
#ifndef _NAMETABLE_VIEW_H_
#define _NAMETABLE_VIEW_H_

#include <View.h>
#include <Bitmap.h>


constexpr int32 kNameTableWidth = 32*8;
constexpr int32 kNameTableHeight = 30*8;


class NameTableView : public BView
{
	public:
	NameTableView (BRect frame, int32 which);
	virtual ~NameTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	virtual void Pulse();
	
	private:
	void DrawPixel (int32 x, int32 y, uint8 color);
	void DrawTile (int32 patternTable, int32 tileIndex, int32 tileX, int32 tileY);

	private:
	void DrawNameTable (int32 which);
	
	private:
	BBitmap *fBitmap = nullptr;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fWhichNameTable = 0;
};

#endif
