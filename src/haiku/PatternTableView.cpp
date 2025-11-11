#include "PatternTableView.h"


PatternTableView::PatternTableView (BRect frame, int32 which)
	: BView (frame, "pattern_table_view", B_FOLLOW_ALL, B_WILL_DRAW|B_PULSE_NEEDED)
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
	fBitmap = new BBitmap(BRect(0, 0, screen_size::WIDTH-1, 
								screen_size::HEIGHT-1), B_CMAP8);
	fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
	fRowBytes = fBitmap->BytesPerRow();
	memset(fBits, 0x0, fBitmap->BitsLength());	
		
	BView::AttachedToWindow();
}


void 
PatternTableView::Draw (BRect updateRect)
{	
	if (fViewMode == view_mode::MODE_8x8) {
		DrawPatternTable8x8(fWhichPatternTable);
	} else if (fViewMode == view_mode::MODE_8x16) {
		DrawPatternTable8x16(fWhichPatternTable);
	}
	
	DrawBitmap(fBitmap, Bounds());	

	BView::Draw(updateRect);
}


void
PatternTableView::Pulse()
{
	// make sure we capture updates periodically
	if (nes::cart.mapper()) {
		Invalidate();
	}
		
	BView::Pulse();
}


void
PatternTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	uint8 *dest = fBits;
	int32 const rowbytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*rowbytes)) = color;
}


void
PatternTableView::DrawTile (int32 patternTable, int32 tileIndex, int32 tileX, int32 tileY)
{
	int32 shift;
	uint8 pixel;
	int32 xofs = (patternTable << 12)+(tileIndex*16);
	
	// greyscale palette reverse-engineered from haiku system palette
	uint8 const colors[] = {
		0x0,	// black 
		0xaf, 	// dark grey
		0x88,	// light grey
		0xff	// white
	};

	Mapper *mapper = nes::cart.mapper();
	if (! mapper) {
		return;
	}

	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = mapper->read_vram(xofs+0);
		uint8 secondPlane = mapper->read_vram(xofs+8);
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
		for (int32 x = 0; x < 16; x++) {	
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
