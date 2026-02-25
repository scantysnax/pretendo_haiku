
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
NameTableView::Pulse()
{	
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



// Returns the final 8-bit display color for a single background pixel.

// 'palette' = which of the 4 background palettes this tile uses (0-3)
// 'pixel'   = the 2-bit pixel value decoded from the pattern table (0-3)
uint8
NameTableView::ColorForPixel (uint8 palette, uint8 pixel)
{
	Mapper *mapper = nes::cart.mapper();
	
	/*
	a note about pixel value 0:
		it is magical.  it is always the background color. in whichever palette is selected.
		each palette has four colors, color 0 of the palette always uses whatever is in 0x3f00.
		this makes it the universal background color.
		
	*/
	if (pixel == 0) {
		uint8 const bgColor = mapper->read_vram(0x3f00) & 0x3f; // 6-bit palette entries
		return fPalette[bgColor];
	}
	 
	/*
	for other pixel values, we use the tile's palette.
	background tiles are laid out this way:
		0x3f00: background color
		0x3f01: palette 0 color 1
		0x3f02: palette 0 color 2
		0x3f03: palette 0 color 3
		
		0x3f04: background color
		0x3f05: palette 1 color 1
		0x3f06: palette 1 color 2
		0x3f07: palette 1 color 3
		
		0x3f08: background color
		0x3f09: palette 2 color 1
		0x3f0a: palette 2 color 2
		0x3f0b: palette 2 color 3
		
		0x3f0c: background color
		0x3f0d: palette 3 color 1
		0x3f0e:	palette 3 color 2
		0x3f0f: palette 3 color 3
	
	figure the address of the palette entry we need where:
			base addresss:	0x3f00
			+1:				to skip the background color
			palette*4:		select which of four background palettes
			(pixel-1):		select color inside that palette
	*/
	uint32 const baseAddress = 0x3f00+1 + (palette*4) + (pixel-1);
	
	
	// read from that address to get the color, limit to 6-bits
	uint8 const color = mapper->read_vram(baseAddress) & 0x3f;
	
	// transform palette index to actual color by the renderer
	return fPalette[color];
}


// Returns which of the 4 background palettes (0-3) a specific tile uses.
//
// nameTableBase = Base address of the nametable being rendered ($2000/$2400/$2800/$2C00)
// tileX, tileY  = Tile coordinates within that nametable (0-31, 0-29)
uint8
NameTableView::PaletteForAttribute(uint32 nameTableBase, int32 tileX, int32 tileY)
{
/*
	nametables also have a 64-byte attribute table at 0x3c0-0x3ff where:
		nametable: 			0x2000-0x23bf (32x30 tiles)
		attribute table:	0x23c0-0x23ff (8x8 bytes)
		
		nametable: 			0x2400-0x27bf (32x30 tiles)
		attribute table:	0x27c0-0x27ff (8x8 bytes)
		
		nametable: 			0x2800-0x2bbf (32x30 tiles)
		attribute table:	0x2bc0-0x2bff (8x8 bytes)
		
		nametable: 			0x2c00-0x2fbf (32x30 tiles)
		attribute table:	0x2fc0-0x2fff (8x8 bytes)
		
		each attribute byte controls a 4x4 tile grid (32x32 pixels)
	*/
	
	/*
	find out which attribute byte corresponds to this tile
		divide tile X/Y by for which 4-tile row/column
		there are 8 attribute columns in each row
	*/
	uint32 attrAddr = nameTableBase + 0x3c0 + ((tileY / 4) * 8) + (tileX / 4);

    // fetch attribute byte
    uint8 attrByte = nes::cart.mapper()->read_vram(attrAddr);
    
    /*
    find out which quadrant inside that 4x4-tile area we are in where:
    	each attribute is set up like this:
    		bits 1-0: top-left 		2x2 tiles
    		bits 3-2: top-right 	2x2 tiles
    		bits 5-4: bottom-left	2x2 tiles
    		bits 7-6: bottom-right	2x2 tiles
    		
    		each byte selects 4 different palettes
	*/
    
    /*
    find out which 2x2-tile quadrant we are in where:
    	tile >> 1 isolates which 2-tile block we're in
    // 	& 0x1 extracts the local position (0 or 1).
    //
    // Resulting quadrant index:
    //   0 = top-left
    //   1 = top-right
    //   2 = bottom-left
    //   3 = bottom-right
   */
    uint8 quadrant =
        ((tileY >> 1) & 1) << 1 |   // vertical half (0 or 2)
        ((tileX >> 1) & 1);         // horizontal half (0 or 1)

    // --------------------------------------------------------------------
    // Each quadrant uses 2 bits inside attrByte.
    //
    // quadrant * 2 = bit offset (0,2,4,6)
    // Mask with &3 to extract the palette index (0-3).
    // --------------------------------------------------------------------
    return (attrByte >> (quadrant * 2)) & 3;
}


// Draws a single 8x8 background tile into the output bitmap.
//
// patternTable = Base address of the selected pattern table ($0000 or $1000)
// tileIndex    = Index of the tile from the nametable (0-255)
// tileX/Y      = Tile position inside the nametable (in tiles, not pixels)
// palette      = Attribute-selected background palette (0-3)
void
NameTableView::DrawTile (uint32 patternTable, uint8 tileIndex, int32 tileX, int32 tileY, uint8 palette)
{
    Mapper *mapper = nes::cart.mapper();
    if (! mapper) {
    	return;
    }
    
    /* 
	tiles are 16-bytes each, and made of two bitplanes where:
		bytes 0-7 are the first bitplane of the tile (low bits for row)
		bytes 8-15 are the second bitplane (high bits for row))
	
	multiply tileIndex by 16 (see above) to get the correct adddress for this tile
	*/	
    
    // figure out tile address based on the tileIndex, 
    uint32 const tileAddress = patternTable + (tileIndex * 16);

    for (int32 y = 0; y < 8; y++) {
		// fetch bitplanes for each row of 8 pixels
        uint8 const firstPlane  = mapper->read_vram(tileAddress+y+0);
        uint8 const secondPlane = mapper->read_vram(tileAddress+y+8);

       // merge the bitplanes
        for (int32 x = 0; x < 8; x++) {
            int32 const shift = 7 - x;  // leftmost pixels first
            uint8 pixel;
            uint8 color;
            
            /*
            there are two bits per pixel, where:
            	the first bit is from plane 0
            	the second bit is from plane 1
            
            the result is a two bit value that selects which color to use, where:
            	color 00 = background
            	color 01 = color 1
            	color 10 = color 2
            	color 11 = color 3
            */
            pixel  = (firstPlane  >> shift) & 0x1; // color low bit
            pixel |= ((secondPlane >> shift) & 0x1) << 1; // high bit
            
            // transform pixel into output color using the specified palette
            color = ColorForPixel(palette, pixel);
            
            /*
			draw the final pixel where:
            	we must convert from tile space to screen space        
			*/
			DrawPixel(x + (tileX * 8), y + (tileY * 8), color);
        }
    }
}


// Renders one of the NES's four nametables into the debug view.
//
// which = 0–3 selecting:
//   0 → $2000
//   1 → $2400
//   2 → $2800
//   3 → $2C00
//
// Each nametable is 32x30 tiles (960 bytes) followed by a 64-byte attribute table.
void
NameTableView::DrawNameTable (int32 which)
{
    // Access mapper so reads obey mirroring / CHR banking.
    Mapper *mapper = nes::cart.mapper();

    // --------------------------------------------------------------------
    // Compute base address of the selected nametable.
    // Each nametable occupies 0x400 bytes.
    // --------------------------------------------------------------------
    uint32 const baseAddr = 0x2000 + (which * 0x400);

    // --------------------------------------------------------------------
    // Determine which pattern table the PPU is using for BACKGROUND tiles.
    //
    // PPUCTRL bit 4 selects the background pattern table:
    //   0 → $0000
    //   1 → $1000
    //
    // (ppuctrl & 0x10) gives either 0x00 or 0x10.
    // Shifting left by 8 converts that into:
    //   0x00 << 8 = $0000
    //   0x10 << 8 = $1000
    // --------------------------------------------------------------------
    uint32 const patternBase = (nes::ppu::ppuctrl() & 0x10) << 8;

    // --------------------------------------------------------------------
    // Walk the 32x30 tile grid stored in the nametable.
    //
    // Layout in memory is linear:
    //   row0: 32 bytes
    //   row1: 32 bytes
    //   ...
    // --------------------------------------------------------------------
    for (int32 tileY = 0; tileY < 30; tileY++) {
        for (int32 tileX = 0; tileX < 32; tileX++) {

            // Address of this tile index inside the nametable.
            uint32 const ntblAddr = baseAddr + tileX + (tileY * 32);

            // Read the tile ID (0-255) which selects a CHR tile.
            uint8 const tileIndex = mapper->read_vram(ntblAddr);

            // Look up which of the 4 palettes applies to this tile using
            // the attribute table at $23C0-$23FF.
            uint8 const palette = PaletteForAttribute(baseAddr, tileX, tileY);

            // Draw the decoded 8x8 tile into the output buffer.
            DrawTile(patternBase, tileIndex, tileX, tileY, palette);
        }
    }
}


