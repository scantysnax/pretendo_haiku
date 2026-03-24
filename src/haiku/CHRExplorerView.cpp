
#include <Screen.h>

#include "CHRExplorerView.h"

#include "Cart.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"


CHRExplorerView::CHRExplorerView (BRect frame)
    : BView(frame, "chr_explorer", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fTileIndex = 0;
	fValid = false;
	memset(fCHRBytes, 0, sizeof(fCHRBytes));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


CHRExplorerView::~CHRExplorerView()
{
}


void
CHRExplorerView::Clear()
{
	fValid = false;
	fTileIndex = 0;
	fCHRTileAddress = 0;
	fCHRTileAddressBottom = 0;
	fBgPalette = 0;
	fLocked = false;
	fIsTile16 = false;

	fAttrAddress = 0;
	fAttrByte = 0;
	fAttrQuadrant = 0;
	fWhichNameTable = -1;

	memset(fCHRBytes, 0, sizeof(fCHRBytes));
	memset(fCHRBytesBottom, 0, sizeof(fCHRBytesBottom));
	memset(fDecodedPixels, 0, sizeof(fDecodedPixels));
	memset(fDecodedPixelsBottom, 0, sizeof(fDecodedPixelsBottom));

	Invalidate();
}


void
CHRExplorerView::SetTile (int32 whichPT, int32 tileIndex, bool locked,
	uint32 chrAddr, const uint8 *chrBytes, uint8 bgPalette,
	int32 whichNT, uint32 attrAddr, uint8 attrByte, uint8 attrQuadrant)
{
	fWhichPatternTable = whichPT;
	fTileIndex = tileIndex;
	fLocked = locked;
	fCHRTileAddress = chrAddr;
	fBgPalette = bgPalette % 4;
	fIsTile16 = false;
	fValid = true;

	fWhichNameTable = whichNT;
	fAttrAddress = attrAddr;
	fAttrByte = attrByte;
	fAttrQuadrant = attrQuadrant % 4;

	memcpy(fCHRBytes, chrBytes, 16);
	DecodeTile();

	Invalidate();
}



void
CHRExplorerView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(0, 0, 0);
	FillRect(Bounds());
	SetHighColor(255, 255, 255);

	if (!fValid) {
		DrawString("Hover a tile (or click to lock) to explore CHR.", BPoint(8, 16));
		return;
	}

	float scale = 8.0f;
	BPoint origin(8, 8);

	if (!fIsTile16) {
		DrawDecodedZoomed(fDecodedPixels, origin, scale);
		DrawInfo(BPoint(8, origin.y + 8 * scale + 14));
	} else {
		DrawDecodedZoomed(fDecodedPixels, origin, scale);
		DrawDecodedZoomed(fDecodedPixelsBottom, BPoint(origin.x, origin.y + 8 * scale), scale);
		DrawInfo(BPoint(8, origin.y + 16 * scale + 14));
	}
}



void
CHRExplorerView::DrawDecodedZoomed (const uint8 decoded[8][8], BPoint origin, float scale)
{
	Mapper *mapper = nes::cart.mapper();
	BScreen screen(Window());
	const color_map *cmap = screen.ColorMap();

	if (!mapper || !cmap || !fHostPalette) {
		SetHighColor(80, 80, 80);
		FillRect(BRect(origin.x, origin.y,
			origin.x + 8 * scale - 1,
			origin.y + 8 * scale - 1));
		return;
	}

	rgb_color palette[4];

	uint8 color0 = mapper->read_vram(0x3f00) & 0x3f;
	uint8 color1 = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 0) & 0x3f;
	uint8 color2 = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 1) & 0x3f;
	uint8 color3 = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 2) & 0x3f;

	palette[0] = cmap->color_list[fHostPalette[color0]];
	palette[1] = cmap->color_list[fHostPalette[color1]];
	palette[2] = cmap->color_list[fHostPalette[color2]];
	palette[3] = cmap->color_list[fHostPalette[color3]];

	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			uint8 pixel = decoded[y][x] & 0x3;
			rgb_color color = palette[pixel];

			SetHighColor(color);
			FillRect(BRect(
				origin.x + x * scale,
				origin.y + y * scale,
				origin.x + (x + 1) * scale - 1,
				origin.y + (y + 1) * scale - 1
			));
		}
	}

	SetHighColor(255, 255, 255, 255);
	StrokeRect(BRect(origin.x, origin.y,
		origin.x + 8 * scale - 1,
		origin.y + 8 * scale - 1));
}


void
CHRExplorerView::DrawTileZoomed (BPoint origin, float scale)
{
	Mapper *mapper = nes::cart.mapper();
	BScreen screen(Window());
	color_map const *cmap = screen.ColorMap();

	if (!mapper || !cmap || !fHostPalette) {
		SetHighColor(100, 100, 100);
		FillRect(BRect(origin.x, origin.y, 
			origin.x + 8 * scale - 1, 
			origin.y + 8 * scale - 1));
		return;
	}

	// Make sure decoded pixels exist
	DecodeTile();


    // build rgb palette using the tile's bg palette (0-3)
    rgb_color palette[4];
    
	// recall, pixel zero is the background color
	uint8 bgColor = mapper->read_vram(0x3f00) & 0x3f;
	uint8 index = fHostPalette[bgColor];
	palette[0] = cmap->color_list[index];

    // pixels 1-3 use the selected bg palette:
	// $3f01 + (bgPalette*4) + (pixel-1)
	for (int32 pixel = 1; pixel <= 3; pixel++) {
		uint32 addr = 0x3f00 + 1 + (fBgPalette * 4) + (pixel - 1);
		uint8 color = mapper->read_vram(addr) & 0x3f;
		uint8 index = fHostPalette[color];
		palette[pixel] = cmap->color_list[index];
	}

    // draw zoomed pixel
	for (int32 y = 0; y < 8; y++) {
		for (int32 x = 0; x < 8; x++) {
			uint8 pix = fDecodedPixels[y][x] & 0x3;
	 		rgb_color color = palette[pix];

			SetHighColor(color);
			BRect r(origin.x + x * scale, 
					origin.y + y * scale, 
					origin.x + (x + 1) * scale - 1, 
					origin.y + (y + 1) * scale - 1
					);
			FillRect(r);
		}
	}

	// border
	SetHighColor(255, 255, 255, 255);
	StrokeRect(BRect(origin.x, origin.y, 
						origin.x + 8 * scale - 1, origin.y + 8 * scale - 1
				));
}


void
CHRExplorerView::DrawInfo (BPoint point)
{
	char line[256];
	
	// header line
	sprintf(line, "PT:%ld  Tile:%u  %s",
		(long)fWhichPatternTable,
		(unsigned)fTileIndex,
		fLocked ? "LOCK" : "HOVER");
	DrawString(line, point);

	
	// chr address line(s)
	point.y += 14;

	if (!fIsTile16) {
		sprintf(line, "CHR $%04X", (unsigned)fCHRTileAddress);
	} else {
		sprintf(line, "CHR TOP $%04X  BOT $%04X",
			(unsigned)fCHRTileAddress,
			(unsigned)fCHRTileAddressBottom);
	}
	
	DrawString(line, point);

	// nametable / attribute info
	point.y += 14;

	if (fWhichNameTable >= 0) {
		static const char* kQuadrantNames[4] = {
			"TL", "TR", "BL", "BR"
		};

		sprintf(line, "NT:%ld  Attr $%04X = %02X  Q:%s",
			(long)fWhichNameTable,
			(unsigned)fAttrAddress,
			(unsigned)fAttrByte,
			kQuadrantNames[fAttrQuadrant % 4]);
		DrawString(line, point);

		point.y += 14;
	}

	// bg palette line + swatch
	sprintf(line, "BG Pal: %u", (unsigned)fBgPalette);
	DrawString(line, point);

	DrawPaletteSwatch(BPoint(point.x + 78, point.y - 10));

	// hex dump of bytes
	point.y += 24;

	float rowH  = 14.0f;
	float cellW = StringWidth("FF") + 10.0f;

	float availW = Bounds().right - point.x - 8.0f;
	int32 cols = availW / cellW;
	
	if (cols < 1) {
		cols = 1;
	}
	
	if (cols > 8) {
		cols = 8;
	}

	int32 totalBytes = fIsTile16 ? 32 : 16;

	for (int32 i = 0; i < totalBytes; i++) {
		uint8 b;

		if (!fIsTile16) {
			b = fCHRBytes[i];
		} else {
			if (i < 16) {
				b = fCHRBytes[i];
			} else {
				b = fCHRBytesBottom[i-16];
			}
		}

		sprintf(line, "%02X", b);

		int32 col = i % cols;
		int32 row = i / cols;

		DrawString(line, BPoint(point.x + col * cellW, point.y + row * rowH));
	}
}


void
CHRExplorerView::DecodeTile()
{
	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = fCHRBytes[y+0];
		uint8 secondPlane = fCHRBytes[y+8];

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			fDecodedPixels[y][x] = pixel;
		}
	}
}


uint8
CHRExplorerView::ColorForPixel (uint8 bgPalette /*0-3*/, uint8 pixel /*0-3*/) const
{
	Mapper *mapper = nes::cart.mapper();
	
	if (!mapper || !fHostPalette)
		return 0;

	// recall, pixel 0 uses universal background color at $3f00
	if (pixel == 0) {
		uint8 color = mapper->read_vram(0x3f00) & 0x3f;
		return fHostPalette[color];
	}

	uint32 addr = 0x3f00 + 1 + (bgPalette * 4) + (pixel - 1);
	uint8 color = mapper->read_vram(addr) & 0x3f;
	
	return fHostPalette[color];
}


void
CHRExplorerView::DrawPaletteSwatch (BPoint point)
{
	Mapper *mapper = nes::cart.mapper();
	BScreen screen(Window());
	const color_map *cmap = screen.ColorMap();

	if (!mapper || !cmap || !fHostPalette)
		return;

	uint8 colors[4];
	colors[0] = mapper->read_vram(0x3f00) & 0x3f;
	colors[1] = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 0) & 0x3f;
	colors[2] = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 1) & 0x3f;
	colors[3] = mapper->read_vram(0x3f00 + 1 + (fBgPalette * 4) + 2) & 0x3f;

	const float w = 18.0f;
	const float h = 10.0f;

	for (int i = 0; i < 4; i++) {
		rgb_color c = cmap->color_list[fHostPalette[colors[i]]];
		BRect r(point.x + i * w, point.y, point.x + (i + 1) * w - 2, point.y + h);

		SetHighColor(c);
		FillRect(r);

		SetHighColor(255, 255, 255, 255);
		StrokeRect(r);
	}
}


void
CHRExplorerView::SetHostPalette(const uint8 *pal)
{
	fHostPalette = pal;
	
	Invalidate();
}


void
CHRExplorerView::SetTile16 (int32 whichPT, int32 topTileIndex, bool locked,
	uint32 chrAddrTop, const uint8 *chrTop,
	uint32 chrAddrBottom, const uint8 *chrBottom, uint8 bgPalette)
{
	if (!chrTop || !chrBottom) {
		Clear();
		return;
	}

	fWhichPatternTable = whichPT;
	fTileIndex = topTileIndex;
	fLocked = locked;
	fBgPalette = bgPalette % 4;

	fCHRTileAddress = chrAddrTop;
	memcpy(fCHRBytes, chrTop, 16);

	fCHRTileAddressBottom = chrAddrBottom;
	memcpy(fCHRBytesBottom, chrBottom, 16);

	fIsTile16 = true;
	fValid = true;

	// top tile
	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = fCHRBytes[y+0];
		uint8 secondPlane = fCHRBytes[y+8];

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			
			fDecodedPixels[y][x] = pixel;
		}
	}
	
	// bottom tile
	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = fCHRBytesBottom[y+0];
		uint8 secondPlane = fCHRBytesBottom[y+8];

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			
			fDecodedPixelsBottom[y][x] = pixel;
		}
	}

	Invalidate();
}
