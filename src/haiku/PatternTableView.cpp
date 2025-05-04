
#include "PatternTableView.h"

#include "Cart.h"
#include "Nes.h"


PatternTableView::PatternTableView (BRect frame, int32 which)
	: BView (frame, "pattern_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fWhichPatternTable(which)
{
	
}


PatternTableView::~PatternTableView()
{
	delete fBitmap;
}


void
PatternTableView::AttachedToWindow()
{
	fBitmap = new BBitmap(BRect(0, 0,127, 127), B_CMAP8);
	fBits = (uint8 *)fBitmap->Bits();
	fRowBytes = fBitmap->BytesPerRow();
	memset (fBits, 0x0, fBitmap->BitsLength());	
	
	fPopUpMenu = new BPopUpMenu("Tile Size");
	BMenuItem *tile8x8 = new  BMenuItem("8x8", new BMessage('8by8'));
	BMenuItem *tile8x16 = new  BMenuItem("8x16", new BMessage('8x16'));
	fPopUpMenu->AddItem(tile8x8);
	fPopUpMenu->AddItem(tile8x16);
	fPopUpMenu->SetRadioMode(true);
	tile8x8->SetMarked(true);
	
	BView::AttachedToWindow();
}


void 
PatternTableView::Draw (BRect updateRect)
{	
	if (fViewMode == 0) {
		DrawPatternTable8x8(fWhichPatternTable);
	} else if (fViewMode == 1) {
		DrawPatternTable8x16(fWhichPatternTable);
	}
	
	DrawBitmap(fBitmap, Bounds());	
	
	BView::Draw(updateRect);
}


void
PatternTableView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case '8by8':
			fViewMode = 0;
			memset (fBits, 0x0, fBitmap->BitsLength());
			Invalidate();
			break;
		
		case '8x16':
			fViewMode = 1;
			memset (fBits, 0x0, fBitmap->BitsLength());
			Invalidate();
			break;
	}
		
	BView::MessageReceived (message);
}


void
PatternTableView::MouseDown(BPoint point)
{
	uint32 mouseButtons;
	BPoint mousePos;
	
	GetMouse(&mousePos, &mouseButtons);
	
	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		ConvertToScreen(&point);
	
		fPopUpMenu->SetTargetForItems(this);
		fPopUpMenu->Go(point, true, true, true);
	}
	
   BView::MouseDown(point);	
}


void 
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	uint8 *dest = fBits;
	int32 rowbytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*rowbytes)) = color;
}


void
PatternTableView::DrawTile (int32 patternTable, int32 tileIndex, int32 tileX, int32 tileY)
{
	int32 shift;
	uint8 pixel;
	int32 xofs = tileIndex*16;
	
	// greyscale palette reverse-engineered from haiku system palette
	uint8 const colors[] = {
		0x0,	// black 
		0xaf, 	// dark grey
		0x88,	// light grey
		0xff	// white
	};
	
	// FIXME: this is broken (eli)
	uint8 *chrRom = nes::cart.chr()+(patternTable << 12);
	if (chrRom == nullptr) {
		return;
	}

	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = chrRom[xofs+0];
		uint8 secondPlane = chrRom[xofs+8];
		shift = 7;
				
		for (int32 x = 0; x < 8; x++) {
			pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;		
			shift--;
 	
			DrawPixel(x+(tileX*8), y+(tileY*8), colors[pixel]);	
		}
		
		xofs++;
	}
}


void
PatternTableView::DrawPatternTable8x8 (int32 which)
{	
	for (int32 y = 0; y < 16; y++) {
		for (int32 x = 0; x < 16; x++){
			DrawTile(which, x+(y*16), x, y);
		}                 
	}
}


void
PatternTableView::DrawPatternTable8x16 (int32 which)
{
	int32 x = 0;
	int32 y = 0;

	for (int32 i = 0; i < 8; i++) {
		for (int32 t = (i*32); t < ((i*32)+32); t += 2) {
			DrawTile(which, t, x, y);
			DrawTile(which, (t+1), x, (y+1));
			x++;
		}
		
		y += 2;
		x = 0;
	}
}
