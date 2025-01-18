
#include "PatternTableView.h"

#include "Cart.h"
#include "Nes.h"


PatternTableView::PatternTableView (BRect frame, int32 which)
	: BView (frame, "pattern_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fWhichPatternTable = which;
}


PatternTableView::~PatternTableView()
{
	delete fBitmap;
}


void
PatternTableView::AttachedToWindow()
{
	fBitmap = new BBitmap(BRect(0, 0,128, 128), B_CMAP8);
	fBits = (uint8 *)fBitmap->Bits();
	fRowBytes = fBitmap->BytesPerRow();

	memset (fBits, 0x0, fBitmap->BitsLength());	
}


void 
PatternTableView::Draw (BRect updateRect)
{
	(void)updateRect;
	
	DrawPatternTable(fWhichPatternTable);
	DrawBitmap(fBitmap, Bounds());	
}


void
PatternTableView::MessageReceived (BMessage *message)
{
	BView::MessageReceived (message);
}


void 
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	uint8 *dest = fBits;
	int32 row_bytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*row_bytes)) = color;
}


void
PatternTableView::DrawPatternTable (int32 which)
{
	int32 shift;
	uint8 pixel;
	int32 xofs = 0;
	
	// greyscale palette reverse-engineered from haiku system palette
	uint8 colors[] = {
		0x0,	// black 
		0xaf, 	// dark grey
		0x88,	// light grey
		0xff	// white
	};
		
	uint8 *chrRom = nes::cart.chr()+(which << 12);
	if (chrRom == nullptr) {
		return;
	}

	for (int32 col = 0; col < 16; col++) {
		for (int32 row = 0; row < 16; row++) {
			for (int32 y = 0; y < 8; y++) {
				uint8 firstPlane = chrRom[xofs+0];
				uint8 secondPlane = chrRom[xofs+8];
				shift = 7;
				
				for (int32 x = 0; x < 8; x++) {
					pixel = (firstPlane >> shift) & 0x1;
					pixel |= ((secondPlane >> shift) & 0x1) << 1;		
					shift--;
					
					DrawPixel(x+(row*8), y+(col*8), colors[pixel]);					
				}
						
				 xofs++;
			}
			
			xofs += 8;
		}
	}
	
}

