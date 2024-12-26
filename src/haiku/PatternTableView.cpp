
#include "PatternTableView.h"


PatternTableView::PatternTableView (BRect frame, uint32 address)
	: BView (frame, "pattern_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fAddress = address;
}


PatternTableView::~PatternTableView()
{
	delete fBitmap;
}


void
PatternTableView::AttachedToWindow()
{
	fBitmap = new BBitmap(Bounds(), B_CMAP8);
	fBits = (uint8 *)fBitmap->Bits();
	fRowBytes = fBitmap->BytesPerRow();
	memset (fBits, 0x0, fBitmap->BitsLength());
	
	SetViewBitmap(fBitmap);
}

void 
PatternTableView::Draw (BRect updateRect)
{
	int32 height = 32*8;
	int32 width = 32*8;

	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			DrawPixel(x, y, 0xfe);	
	}	}
	
	BView::Draw(updateRect);
}


void 
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	uint8 *dest = fBits;
	int32 row_bytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*row_bytes)) = color;
}


void
PatternTableView::MessageReceived (BMessage *message)
{
	//switch (message->what) {
			
				
	//	default:
	//		break;
	//}
	
	BView::MessageReceived (message);
}





