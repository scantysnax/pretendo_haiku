
#include "PatternTableView.h"

#include "Cart.h"
#include "Nes.h"


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
	fBitmap = new BBitmap(BRect(0,0,128, 128), B_CMAP8);
	fBits = (uint8 *)fBitmap->Bits();
	fRowBytes = fBitmap->BytesPerRow();

	if (nes::cart.has_chr_rom()) {
		fChrRom = nes::cart.chr()+fAddress;
	}

	memset (fBits, 0x0, fBitmap->BitsLength());	
}


void 
PatternTableView::Draw (BRect updateRect)
{
	(void)updateRect;
	
	DrawPatternTable(fChrRom);
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
PatternTableView::DrawPatternTable (uint8 *chr_rom)
{
	int32 shift;
	uint8 pixel;
	int32 xofs = 0;
	
	uint8 colors[] = {
		0x0,	// black 
		0xaf, 	// dark grey
		0x88,	// light grey
		0xff	// white
	};
	
	for (int32 col = 0; col < 16; col++) {
		for (int32 row = 0; row < 16; row++) {
			for (int32 y = 0; y < 8; y++) {
				uint8 firstPlane = chr_rom[xofs+0];
				uint8 secondPlane = chr_rom[xofs+8];
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

