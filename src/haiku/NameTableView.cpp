
#include "NameTableView.h"

uint8 const kWrongPalette[64] = {
    0x75, 0x27, 0x2a, 0x52, 0x7f, 0xab, 0x8b, 0x43, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xbc, 0x73, 0x6f, 0x9f, 0xd4, 0xff, 0xf7, 0x8f, 0x7b, 0x3c, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xbc, 0xb8, 0xd8, 0xff, 0xff, 0xff, 0xd8, 0xc3, 0x8f, 0x73, 0x52, 0x52, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd8, 0xbc, 0xab, 0xab, 0x00, 0x00, 0x00
};

	
NameTableView::NameTableView (BRect frame, NameTableWindow *parent, int32 which)
	: BView (frame, "name_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED)
{
	fWhichNameTable = which;
	fParent = parent;
}


NameTableView::~NameTableView()
{
	delete fBitmap;
}


void
NameTableView::AttachedToWindow()
{
	fBitmap = new BBitmap(BRect(0, 0, kNameTableWidth-1, kNameTableHeight-1), B_CMAP8);
	fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
	fRowBytes = fBitmap->BytesPerRow();
	memset(fBits, 0x0, fBitmap->BitsLength());	
	
	fParent->blah();
	
	BView::AttachedToWindow();
}


void 
NameTableView::Draw (BRect updateRect)
{	
	DrawNameTable(fWhichNameTable);
	DrawBitmap(fBitmap, Bounds());	
	
	BView::Draw (updateRect);
}


void
NameTableView::MessageReceived (BMessage *message)
{		
	BView::MessageReceived (message);
}


void
NameTableView::Pulse()
{
	BView::Pulse();	
}


void 
NameTableView::DrawPixel (int32 x, int32 y, uint8 color)
{
	uint8 *dest = fBits;
	int32 rowBytes = fRowBytes;
	
	*(uint8 *)(dest+x+(y*rowBytes)) = color;
}


uint8
NameTableView::GetBackgroundColor (uint8 palette, uint8 pixel)
{
	Mapper *mapper = nes::cart.mapper();
    
    // color 0 is background color
	if (pixel == 0) {
		uint8 bgColor = mapper->read_vram(0x3f00) & 0x3f;
		return kWrongPalette[bgColor];
	}

	uint32 address = 0x3f01 + (palette * 4) + (pixel - 1);
	uint8 color = mapper->read_vram(address) & 0x3f;
	
    return kWrongPalette[color];
}


uint8
NameTableView::GetAttributePalette (uint32 nameTableBase, int32 tileX, int32 tileY)
{
	Mapper *mapper = nes::cart.mapper();
	uint32 attrBase = (nameTableBase + 0x3c0);

	int32 attrX = tileX >> 2;
	int32 attrY = tileY >> 2;

	uint8 attrByte = mapper->read_vram(attrBase + (attrY * 8) + attrX);
	int32 shift = ((tileY & 0x02) << 1) | (tileX & 0x02);
	
	return (attrByte >> shift) & 0x3;
}


void
NameTableView::DrawTile (uint32 patternTableBase, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette)
{
	Mapper *mapper = nes::cart.mapper();
	uint32 tileAddr = patternTableBase + (tileIndex * 16);
    
	for (int32 y = 0; y < 8; y++) { 
		uint8 firstPlane  = mapper->read_vram(tileAddr + y);
		uint8 secondPlane = mapper->read_vram(tileAddr + y + 8);

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			
			uint8 color = GetBackgroundColor(palette, pixel);
			DrawPixel(x + tileX * 8, y + tileY * 8, color);
		}
	}
}


void
NameTableView::DrawNameTable (int32 which)
{
	Mapper *mapper = nes::cart.mapper();
	uint32 baseAddr = 0x2000 + (which * 0x400);
	
	// select active pattern table
	uint32 patternBase = (nes::ppu::ppuctrl() & 0x10) << 8;
	
	// draw the nametable
	for (int32 tileY = 0; tileY < 30; tileY++) {
    	for (int32 tileX = 0; tileX < 32; tileX++) {
			uint32 ntblAddr = baseAddr + tileY * 32 + tileX;
			uint8 tileIndex = mapper->read_vram(ntblAddr);
			uint8 palette = GetAttributePalette(baseAddr, tileX, tileY);
			
			DrawTile(patternBase, tileIndex, tileX, tileY, palette);
		}
	}
}


