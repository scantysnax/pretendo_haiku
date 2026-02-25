
#include "PatternTableView.h"


PatternTableView::PatternTableView (BRect frame, PretendoWindow *mainWindow, int32 which)
	: BView (frame, "pattern_table_view", B_FOLLOW_ALL, B_WILL_DRAW|B_PULSE_NEEDED)
{
	fMainWindow = mainWindow;
	fWhichPatternTable = which;
	fPalette = fMainWindow->Palette();
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
	if (nes::cart.mapper()) {
		fPalette = fMainWindow->Palette();
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


// Draw a single tile from a pattern table into the pattern-table debug view.
//
// patternTable = 0 or 1 selecting:
//      0 → pattern table at $0000
//      1 → pattern table at $1000
//
// tileIndex = Which 8x8 tile inside that table (0-255 per table)
// tileX/Y   = Where to place it in the debug grid (in tiles)

void
PatternTableView::DrawTile (uint32 patternTable, int32 tileIndex, int32 tileX, int32 tileY)
{
    Mapper *mapper = nes::cart.mapper();
    if (! mapper) {
        return; // No cartridge / CHR source available.
    }

    /*
        Optional fixed greyscale palette (useful for debugging CHR data only).

        Pattern-table viewers often ignore real NES palettes and instead show
        tiles using a neutral ramp so graphics are readable even if the game
        hasn't initialized palette RAM yet.

        This was reverse-engineered from the Haiku system palette:
            0x00 = black
            0xAF = dark gray
            0x88 = light gray
            0xFF = white
    */

    // --------------------------------------------------------------------
    // Instead of using the tile's attribute-selected palette (like the PPU),
    // this viewer always uses Background Palette 0 at $3F00-$3F03.
    //
    // This is intentional: a pattern-table viewer has no nametable context,
    // so we just pick a stable palette so tiles are visible.
    // --------------------------------------------------------------------
    uint32 const paletteBase = 0x3f00;
    uint8 palette[4];

    // Fetch the 4 NES color entries and convert them through the host palette.
    for (int32 i = 0; i < 4; i++) {
        uint8 const color = mapper->read_vram(paletteBase + i) & 0x3f;
        palette[i] = fPalette[color];
    }

    // --------------------------------------------------------------------
    // Compute the address of this tile inside CHR memory.
    //
    // patternTable is 0 or 1, so shifting by 12 gives:
    //   0 << 12 = $0000
    //   1 << 12 = $1000
    //
    // Each tile is 16 bytes.
    // --------------------------------------------------------------------
    uint32 const tileAddr = (patternTable << 12) + (tileIndex * 16);

    // Decode the 8 rows of the tile.
    for (int32 y = 0; y < 8; y++) {

        // NES stores graphics as two bitplanes.
        uint8 const firstPlane  = mapper->read_vram(tileAddr + y + 0);
        uint8 const secondPlane = mapper->read_vram(tileAddr + y + 8);

        // Extract the 8 pixels packed into those two bytes.
        for (int32 x = 0; x < 8; x++) {
            int32 const shift = 7 - x;  // MSB is leftmost pixel

            uint8 pixel;
            uint8 color;

            // Combine the two bitplanes into a 2-bit pixel value.
            pixel  = (firstPlane  >> shift) & 0x1;
            pixel |= ((secondPlane >> shift) & 0x1) << 1;

            // Unlike the real renderer, we directly index into our
            // chosen debug palette instead of doing attribute lookup.
            color = palette[pixel];

            // Draw into the viewer grid.
            DrawPixel(x + (tileX * 8), y + (tileY * 8), color);
        }
    }
}


// Draw an entire pattern table as a 16x16 grid of 8x8 tiles.
//
// which = 0 or 1 selecting:
//   0 → Pattern Table at $0000
//   1 → Pattern Table at $1000
//
// Each pattern table contains 256 tiles total.
// We visualize them in a 16x16 grid purely for readability.
void
PatternTableView::DrawPatternTable8x8 (int32 which)
{	
    // Loop over the rows of tiles in the viewer grid.
    // 16 rows * 16 columns = 256 tiles (entire pattern table).
	for (int32 y = 0; y < 16; y++) {
        // Loop over columns in this row.
		for (int32 x = 0; x < 16; x++) {

            // ----------------------------------------------------------------
            // Compute which tile index we are drawing.
            //
            // Pattern tables are stored linearly in CHR memory:
            //   tile 0, tile 1, tile 2, ... tile 255
            //
            // We remap that linear order into a 16x16 2D grid:
            //
            //   index = x + (y * 16)
            // ----------------------------------------------------------------
			DrawTile(which, x + (y * 16), x, y);
		}
	}
}

// Draw the pattern table arranged the way the PPU uses it in 8x16 sprite mode.
//
// In 8x16 mode, each sprite is made of TWO stacked tiles:
//   top  tile = even index
//   bottom tile = next odd index
//
// The NES does not treat these as independent tiles — they are always paired.
// This function rearranges the linear CHR data to reflect that pairing.
void
PatternTableView::DrawPatternTable8x16 (int32 which)
{
    // x,y here are VIEWER coordinates (tile grid position),
    // not CHR memory coordinates.
	int32 x = 0;
	int32 y = 0;

    // There are 256 tiles total.
    // In 8x16 mode they form 128 sprite pairs.
    //
    // We draw them as 8 rows of 32 tiles (16 sprites per row).
	for (int32 i = 0; i < 8; i++) {

        // Each row processes 32 tiles = 16 sprite pairs.
        //
        // (i * 32) selects the start of this row in CHR memory.
        // We advance by 2 because each sprite consumes two tiles.
		for (int32 t = (i * 32); t < ((i * 32) + 32); t += 2) {

            // Draw the TOP half of the sprite (even tile index).
			DrawTile(which, t, x, y);

            // Draw the BOTTOM half (odd tile index) directly beneath it.
            // In 8x16 mode, the PPU *always* fetches tiles this way.
			DrawTile(which, (t + 1), x, (y + 1));

            // Move to the next sprite column.
			x++;
		}

        // After finishing a row of sprites:
        // advance down by 2 because each sprite is 16 pixels tall (2 tiles).
		y += 2;

        // Reset to the left side for the next row.
		x = 0;
	}
}
