
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
}

void 
PatternTableView::Draw (BRect updateRect)
{
	DrawPatternTable(fAddress);
	DrawBitmap(fBitmap);
	
	BView::Draw(updateRect);
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


void 
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	// 0x0  black
	// 0xd6 dark grey
	// 0xaf	dark grey #2
	// 0x10 grey
	// 0x1b lighter grey
	// 0x1d very light grey
	// 0x61 grey #2
	// 0x88 grey #3
	// 0xff white
	
	uint8 *dest = fBits;
	int32 row_bytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*row_bytes)) = color;
}

void
PatternTableView::DrawPatternTable (uint32 address)
{
	(void)address;
	
	int32 width = 16 * 16;
	int32 height = 16 * 16;
	
	/*
	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			DrawPixel(x, y, 0x1b);
		}
	}
	*/
	 
	 int32 w0 = (width / 4);
	 uint8 colors[] = {
		 0x0,	// black 
		 0x10, 	// dark grey
		 0x1b, 	// light grey
		 0xff	// white
	};

	for (int32 y = 0; y < height; y++) {
		
		for (int32 x = 0; x < w0; x++) {
			DrawPixel(x, y, colors[0]);
		}
		
		for (int32 x = w0; x < w0*2; x++) {
			DrawPixel(x,y, colors[1]);
		}
		
		for (int32 x = w0*2; x < w0*3; x++) {
			DrawPixel(x, y, colors[2]);
		}
		
		for (int32 x = w0*3; x < w0*4; x++) {
			DrawPixel(x, y, colors[3]);
		}
	}
}

