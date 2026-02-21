
#include "NameTableView.h"


NameTableView::NameTableView (BRect frame, PretendoWindow *mainWindow, int32 which)
	: BView (frame, "name_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED)
{
	fMainWindow = mainWindow;
	fWhichNameTable = which;
	
	fPalette = fMainWindow->Palette();
}


NameTableView::~NameTableView()
{
	delete fBitmap;
}


void
NameTableView::AttachedToWindow()
{
	fBitmap = new BBitmap(BRect(0, 0, NameTableWindow::nametable_size::WIDTH-1, 
								NameTableWindow::nametable_size::HEIGHT-1), B_CMAP8);
	fBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
	fRowBytes = fBitmap->BytesPerRow();
	memset(fBits, 0x0, fBitmap->BitsLength());	
	
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
	// make sure we capture updates periodically
	if (nes::cart.mapper()) {
		fPalette = fMainWindow->Palette();
		Invalidate();
	}
	
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
NameTableView::BackgroundColor (uint8 palette, uint8 pixel)
{
	Mapper *mapper = nes::cart.mapper();
    
    // color 0 is background color
	if (pixel == 0) {
		uint8 bgColor = mapper->read_vram(0x3f00) & 0x3f;
		return fPalette[bgColor];
	}
	
	// we already handled the background color
	uint32 address = 0x3f00 + 1 + (palette*4) + (pixel-1);
	uint8 color = mapper->read_vram(address) & 0x3f;
	
    return fPalette[color];
}


uint8
NameTableView::AttributePalette (uint32 nameTableBase, int32 tileX, int32 tileY)
{
	Mapper *mapper = nes::cart.mapper();
	uint32 attrBase = (nameTableBase + 0x3c0);
	int32 attrX = (tileX >> 2);
	int32 attrY = (tileY >> 2);
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
			
			uint8 color = BackgroundColor(palette, pixel);
			DrawPixel(x + tileX * 8, y + tileY * 8, color);
		}
	}
}


void
NameTableView::DrawNameTable (int32 which)
{
	Mapper *mapper = nes::cart.mapper();
	uint32 baseAddr = 0x2000 + (which * 0x400);
	uint32 patternBase = (nes::ppu::ppuctrl() & 0x10) << 8;
	
	for (int32 tileY = 0; tileY < 30; tileY++) {
    	for (int32 tileX = 0; tileX < 32; tileX++) {
			uint32 ntblAddr = baseAddr + tileX + (tileY * 32);
			uint8 tileIndex = mapper->read_vram(ntblAddr);
			uint8 palette = AttributePalette(baseAddr, tileX, tileY);
			
			DrawTile(patternBase, tileIndex, tileX, tileY, palette);
		}
	}
}


