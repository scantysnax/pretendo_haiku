// -----------------------------------------------------------------------------
// CHRExplorerView.cpp
//
// Implements the CHR tile explorer used by the PatternTable and NameTable
// debugger windows.  The view displays a zoomed tile, raw CHR bytes, palette
// previews, pixel/bitplane details, and compact tile analysis.
// -----------------------------------------------------------------------------

#include "CHRExplorerView.h"
#include "DebugHelpers.h"

#include "Cart.h"
#include "Mapper.h"
#include "Nes.h"
#include "Ppu.h"


// -----------------------------------------------------------------------------
// ByteToBinary
//
// Converts an 8-bit value into a null-terminated string of eight binary digits.
//
// Parameters:
//   value - Byte to convert.
//   out   - Destination buffer. Must have room for 9 chars, including '\0'.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static inline void
ByteToBinary (uint8 value, char out[9])
{
	for (int32 i = 0; i < 8; i++) {
		out[i] = (value & (0x80 >> i)) ? '1' : '0';
	}

	out[8] = '\0';
}


// -----------------------------------------------------------------------------
// CHRExplorerView::CHRExplorerView
//
// Constructs the CHR explorer view and initializes all selection, palette,
// hover, decoded-pixel, and raw CHR state to safe defaults.
//
// Parameters:
//   frame - Initial view frame in parent-window coordinates.
//
// Returns:
//   A CHRExplorerView instance.
// -----------------------------------------------------------------------------
CHRExplorerView::CHRExplorerView (BRect frame)
	: BView(frame, "chr_explorer", B_FOLLOW_ALL,
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	// Start invalid; the source views will populate the explorer with a tile.
	fTileIndex = 0;
	fValid = false;

	fMouseInside = false;
	fLastMouse = BPoint(-1, -1);
	fHoverPixelValid = false;
	fHoverPixelX = -1;
	fHoverPixelY = -1;

	// Initialize source tile and CHR address state.
	fWhichPatternTable = 0;
	fCHRTileAddress = 0;
	fCHRTileAddressBottom = 0;
	fPalette = 0;
	fLocked = false;
	fIsTile8x16 = false;

	fAttrAddress = 0;
	fAttrByte = 0;
	fAttrQuadrant = 0;
	fWhichNameTable = -1;

	fHostPalette = nullptr;

	// Clear raw and decoded tile storage.
	memset(fCHRBytes, 0, sizeof(fCHRBytes));
	memset(fCHRBytesBottom, 0, sizeof(fCHRBytesBottom));
	memset(fDecodedPixels, 0, sizeof(fDecodedPixels));
	memset(fDecodedPixelsBottom, 0, sizeof(fDecodedPixelsBottom));

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


// -----------------------------------------------------------------------------
// CHRExplorerView::~CHRExplorerView
//
// Destroys the CHR explorer view.  The view does not own the host palette or
// emulator objects, so no explicit cleanup is required here.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CHRExplorerView::~CHRExplorerView()
{
}


// -----------------------------------------------------------------------------
// CHRExplorerView::AttachedToWindow
//
// Performs view setup that requires the view to be attached to a window,
// including transparent background setup and pointer-event tracking.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::KeyDown
//
// Handles keyboard shortcuts local to the CHR explorer.  Number keys select
// preview palettes, and 0 restores the source quadrant palette.
//
// Parameters:
//   bytes    - Key bytes provided by BView. bytes[0] is used for shortcuts.
//   numBytes - Number of bytes in the key event. Currently unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::KeyDown (const char *bytes, int32 numBytes)
{	
	(void)numBytes;

	if (!bytes || !fValid) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case '1':
			fPalette = 0;
			Invalidate();
			return;

		case '2':
			fPalette = 1;
			Invalidate();
			return;

		case '3':
			fPalette = 2;
			Invalidate();
			return;

		case '4':
			fPalette = 3;
			Invalidate();
			return;

		case '0':
			fPalette = fQuadrantPalette;
			Invalidate();
			return;
	}

	BView::KeyDown(bytes, numBytes);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::MessageReceived
//
// Handles messages delivered to the CHR explorer view.  No custom messages are
// currently processed here, so messages are forwarded to BView.
//
// Parameters:
//   message - Message received by this view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::MessageReceived (BMessage *message)
{
	BView::MessageReceived(message);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::MouseDown
//
// Handles mouse clicks inside the CHR explorer.  Palette-preview clicks select
// the active preview palette; other clicks fall back to BView handling.
//
// Parameters:
//   where - Mouse position in this view's coordinate system.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::MouseDown (BPoint where)
{
	MakeFocus(true);

	int32 pal = PalettePreviewAt(where);
	if (pal >= 0 && pal < 4) {
		fPalette = static_cast<uint8>(pal);
		Invalidate();
		return;
	}

	BView::MouseDown(where);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::MouseMoved
//
// Tracks the mouse over the zoomed tile preview so the Pixel / CHR panel can
// display the current pixel value and bitplane source bits.
//
// Parameters:
//   where   - Mouse position in this view's coordinate system.
//   transit - BView mouse-transit code, such as B_EXITED_VIEW.
//   message - Optional drag message. Currently unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::MouseMoved (BPoint where, uint32 transit, const BMessage *message)
{
	(void)message;

	if (transit == B_EXITED_VIEW) {
		fMouseInside = false;
		fHoverPixelValid = false;
		Invalidate();
		return;
	}

	fMouseInside = true;
	fLastMouse = where;

	UpdateHoverPixelFromMouse();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::Draw
//
// Draws the full CHR explorer UI: boxed panels, zoomed tile preview, tile
// metadata, palette previews, attribute quadrant diagram, pixel/bitplane
// details, selected-palette legend, and CHR analysis.
//
// Parameters:
//   updateRect - The invalidated region supplied by the app_server. Currently
//                unused because the view redraws its full contents.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::Draw (BRect updateRect)
{
	(void)updateRect;

	rgb_color bg = {216, 216, 216, 255};

	SetHighColor(bg);
	FillRect(Bounds());

	// Refresh pixel-hover state before drawing pixel/bitplane details.
	UpdateHoverPixelFromMouse();

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float textAscent = fh.ascent;

	const float zoomScale = 8.0f;
	const BPoint zoomOrigin(12.0f, 52.0f);

	const float leftColumnX = 12.0f;

	const float minRightColumnX = 170.0f;
	const float rightColumnW = 130.0f;
	const float rightColumnX = std::max(minRightColumnX, Bounds().right - rightColumnW);
	const float rightColumnY = 44.0f;

	const float previewScale = 3.0f;
	const float cellH = 8.0f * previewScale;
	const float previewH = fIsTile8x16 ? (cellH * 2.0f) : cellH;
	const float gapY = 18.0f;
	const float rowStride = previewH + gapY;
	const float gridBottom = rightColumnY + rowStride + previewH;

	const bool hasNameTableContext = (fWhichNameTable >= 0);

	// -------------------------------------------------
	// Panel backgrounds
	// -------------------------------------------------

	BRect tilePanel(
		4.0f,
		8.0f,
		rightColumnX - 16.0f,
		Bounds().bottom - 8.0f
	);

	BRect palettePanel(
		rightColumnX - 8.0f,
		8.0f,
		Bounds().right - 8.0f,
		gridBottom + 8.0f
	);

	BRect pixelPanel(
		rightColumnX - 8.0f,
		gridBottom + 16.0f,
		Bounds().right - 8.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, tilePanel, "Tile");
	::DrawDebugPanel(this, palettePanel, "Palettes");
	::DrawDebugPanel(this, pixelPanel, "Pixel / CHR");

	// -------------------------------------------------
	// Left-side tile preview + info
	// -------------------------------------------------

	const float infoY = zoomOrigin.y + 16.0f * zoomScale + 14.0f;

	if (!fIsTile8x16) {
		DrawDecodedZoomed(fDecodedPixels, zoomOrigin, zoomScale);
	} else {
		DrawDecodedZoomed(fDecodedPixels, zoomOrigin, zoomScale);
		DrawDecodedZoomed(
			fDecodedPixelsBottom, 
			BPoint(zoomOrigin.x, zoomOrigin.y + 8.0f * zoomScale), 
					zoomScale);
	}

	DrawInfo(BPoint(leftColumnX, infoY));

	// -------------------------------------------------
	// Right-side palette preview
	// -------------------------------------------------

	DrawPalettePreviewGrid(BPoint(rightColumnX, rightColumnY));

	// Start below the Pixel / CHR panel title.
	float textY = gridBottom + 50.0f;

	if (hasNameTableContext) {
		const float quadCell = 20.0f;
		const float quadHeight = 2.0f * quadCell;

		DrawString("Attribute Quadrants", BPoint(rightColumnX, textY));
		textY += lineH;

		DrawQuadrantDiagram(BPoint(rightColumnX, textY));

		// No separate "Source palette" line here.
		// It is already shown in DrawInfo() as "Source Pal: N".
		textY += quadHeight + lineH + 8.0f;
	}

	// -------------------------------------------------
	// Pixel / CHR info
	// -------------------------------------------------

	int32 px = 0;
	int32 py = 0;
	bool havePixel = false;

	if (fHoverPixelValid) {
		px = fHoverPixelX;
		py = fHoverPixelY;
		havePixel = true;
	} else if (fValid) {
		px = 0;
		py = 0;
		havePixel = true;
	}

	const float pixelLabelX = rightColumnX;
	const float pixelValueX = rightColumnX + 54.0f;

	auto drawPixelKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(pixelLabelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(pixelValueX, textY));

		textY += lineH;
	};

	if (havePixel) {
		uint8 value = 0;

		if (!fIsTile8x16 || py < 8) {
			value = fDecodedPixels[py][px] & 0x3;
		} else {
			value = fDecodedPixelsBottom[py - 8][px] & 0x3;
		}

		BString pix;
		pix.SetToFormat("(%ld,%ld) = %u",
			(long)px,
			(long)py,
			(unsigned)value);
		drawPixelKV("Pixel:", pix.String());

		uint8 plane0 = value & 0x1;
		uint8 plane1 = (value >> 1) & 0x1;
		
		BString bits;
		bits.SetToFormat("P0=%u  P1=%u",
			(unsigned)plane0,
			(unsigned)plane1);
		drawPixelKV("Bits:", bits.String());
		

		int32 row = py;

		uint8 rowP0 = 0;
		uint8 rowP1 = 0;

		if (!fIsTile8x16 || py < 8) {
			rowP0 = fCHRBytes[row];
			rowP1 = fCHRBytes[row + 8];
		} else {
			row -= 8;
			rowP0 = fCHRBytesBottom[row];
			rowP1 = fCHRBytesBottom[row + 8];
		}

		char bin0[9];
		char bin1[9];

		ByteToBinary(rowP0, bin0);
		ByteToBinary(rowP1, bin1);

		BFont oldFont;
		GetFont(&oldFont);

		BFont mono(be_fixed_font);
		SetFont(&mono);

		font_height monoFH;
		GetFontHeight(&monoFH);
		const float monoLineH = ceilf(monoFH.ascent + monoFH.descent
			+ monoFH.leading) + 2.0f;

		float bitW = StringWidth("0");
		float bitX = pixelValueX + (px * bitW);

		// Row P0
		SetHighColor(80, 80, 80, 255);
		DrawString("Row P0:", BPoint(pixelLabelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(bin0, BPoint(pixelValueX, textY));

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		SetHighColor(255, 255, 0, 80);
		FillRect(BRect(
			bitX - 1.0f,
			textY - monoFH.ascent,
			bitX + bitW + 1.0f,
			textY + monoFH.descent
		));

		SetHighColor(255, 180, 0, 220);
		StrokeRect(BRect(
			bitX - 1.0f,
			textY - monoFH.ascent,
			bitX + bitW + 1.0f,
			textY + monoFH.descent
		));

		PopState();

		textY += monoLineH;

		// Row P1
		SetHighColor(80, 80, 80, 255);
		DrawString("Row P1:", BPoint(pixelLabelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(bin1, BPoint(pixelValueX, textY));

		PushState();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		SetHighColor(255, 255, 0, 80);
		FillRect(BRect(
			bitX - 1.0f,
			textY - monoFH.ascent,
			bitX + bitW + 1.0f,
			textY + monoFH.descent
		));

		SetHighColor(255, 180, 0, 220);
		StrokeRect(BRect(
			bitX - 1.0f,
			textY - monoFH.ascent,
			bitX + bitW + 1.0f,
			textY + monoFH.descent
		));

		PopState();

		textY += monoLineH;

		SetFont(&oldFont);
		SetHighColor(0, 0, 0, 255);

		// resolve the hovered pixel to its palette RAM address and NES color.
		uint32 palAddr = 0x3f00;
		uint8 nesColor = 0;
		Mapper* mapper = nes::cart.mapper();

		if (mapper) {
			if (value == 0) {
				palAddr = 0x3f00;
			} else if (fUseSpritePalette) {
				palAddr = 0x3f10 + (fPalette * 4) + value;
			} else {
				palAddr = 0x3f00 + 1 + (fPalette * 4) + (value - 1);
			}

			nesColor = mapper->read_vram(palAddr) & 0x3f;
		}
		
		BString palInfo;
		palInfo.SetToFormat("$%04X", (unsigned)palAddr);
		drawPixelKV("PalAddr:", palInfo.String());
		
		BString nesInfo;
		nesInfo.SetToFormat("$%02X", (unsigned)nesColor);
		drawPixelKV("NES:", nesInfo.String());
	} else {
		drawPixelKV("Pixel:", "--");
		drawPixelKV("Bits:", "--");

		BFont oldFont;
		GetFont(&oldFont);

		BFont mono(be_fixed_font);
		SetFont(&mono);

		font_height monoFH;
		GetFontHeight(&monoFH);
		const float monoLineH = ceilf(monoFH.ascent + monoFH.descent
			+ monoFH.leading) + 2.0f;

		SetHighColor(80, 80, 80, 255);
		DrawString("Row P0:", BPoint(pixelLabelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString("--", BPoint(pixelValueX, textY));
		textY += monoLineH;

		SetHighColor(80, 80, 80, 255);
		DrawString("Row P1:", BPoint(pixelLabelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString("--", BPoint(pixelValueX, textY));
		textY += monoLineH;

		SetFont(&oldFont);
		SetHighColor(0, 0, 0, 255);

		drawPixelKV("PalAddr:", "--");
		drawPixelKV("NES:", "--");
	}

	// ----- LEGEND -----

	textY += 8.0f;

	const float legendBox = 12.0f;
	const float legendTextX = rightColumnX + 20.0f;

	BRect selectedBox(
		rightColumnX,
		textY - textAscent,
		rightColumnX + legendBox,
		textY - textAscent + legendBox
	);

	SetHighColor(255, 0, 0, 255);
	StrokeRect(selectedBox);
	StrokeRect(selectedBox.InsetByCopy(-1, -1));

	SetHighColor(0, 0, 0, 255);
	DrawString("Selected Palette", BPoint(legendTextX, textY));

	// ----- CHR ANALYSIS -----

	textY += lineH + 14.0f;
	DrawCHRAnalysis(rightColumnX, textY);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::Clear
//
// Resets the explorer to an empty/invalid state and clears cached CHR, decoded
// pixel, hover, palette, and NameTable-context data.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::Clear()
{
	fValid = false;
	fTileIndex = 0;
	fCHRTileAddress = 0;
	fCHRTileAddressBottom = 0;

	fPalette = 0;
	fQuadrantPalette = 0;

	fLocked = false;
	fIsTile8x16 = false;
	fUseSpritePalette = false;
	fFlipH = false;
	fFlipV = false;

	fAttrAddress = 0;
	fAttrByte = 0;
	fAttrQuadrant = 0;
	fWhichNameTable = -1;
	fWhichPatternTable = 0;

	fMouseInside = false;
	fLastMouse = BPoint(-1, -1);
	fHoverPixelValid = false;
	fHoverPixelX = -1;
	fHoverPixelY = -1;
	
	fNameTileAddress = 0;

	memset(fCHRBytes, 0, sizeof(fCHRBytes));
	memset(fCHRBytesBottom, 0, sizeof(fCHRBytesBottom));
	memset(fDecodedPixels, 0, sizeof(fDecodedPixels));
	memset(fDecodedPixelsBottom, 0, sizeof(fDecodedPixelsBottom));

	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetHostPalette
//
// Sets the host color-map palette used to convert NES palette entries into
// drawable Haiku colors.  The palette is owned by the caller.
//
// Parameters:
//   palette - Mapping from NES color index to host CMAP8 color index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetHostPalette(uint8 *palette)
{
	fHostPalette = palette;
	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetUseSpritePalette
//
// Selects whether decoded tile previews use background palette addresses
// ($3F00-$3F0F) or sprite palette addresses ($3F10-$3F1F).
//
// Parameters:
//   useSpritePalette - true for sprite/OAM tiles, false for background tiles.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetUseSpritePalette(bool useSpritePalette)
{
	fUseSpritePalette = useSpritePalette;
	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetTileTransform
//
// Sets optional display transforms for sprite/OAM inspection.  These affect
// the rendered previews only; the raw CHR bytes remain unchanged.
//
// Parameters:
//   flipH - true to draw the tile horizontally flipped.
//   flipV - true to draw the tile vertically flipped.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetTileTransform(bool flipH, bool flipV)
{
	fFlipH = flipH;
	fFlipV = flipV;

	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetSelectedPalette
//
// Sets the currently selected palette row for the CHR explorer.  The OAM
// debugger uses this when inspecting sprites so the explorer's selected palette
// matches the sprite palette encoded in the sprite attribute byte.
//
// The palette value is reduced to the valid NES palette-row range of 0-3.
// Both fQuadrantPalette and fPalette are updated so the source palette and the
// actively selected preview palette stay in sync.
//
// Parameters:
//   palette - NES background/sprite palette row index.  Values are wrapped to
//             the valid range of 0-3.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetSelectedPalette(uint8 palette)
{
	fQuadrantPalette = palette % 4;
	fPalette = fQuadrantPalette;

	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetTile8x8
//
// Loads an 8x8 CHR tile into the explorer, records optional NameTable and
// attribute context, decodes the tile pixels, and schedules a redraw.
//
// Parameters:
//   whichPT       - Pattern table index that supplied the tile.
//   tileIndex     - Tile index within the selected pattern table.
//   locked        - true if the source view is locked to this tile.
//   chrAddr       - CHR address of the tile.
//   chrBytes      - Pointer to 16 raw 2bpp CHR bytes.
//   bgPalette     - Source background palette number, 0-3.
//   whichNT       - Source NameTable index, or -1 if not from a NameTable.
//   nameTileAddr  - VRAM address of the NameTable tile byte.
//   attrAddr      - VRAM address of the attribute byte.
//   attrByte      - Raw attribute byte for the source area.
//   attrQuadrant  - Attribute quadrant index, 0=TL, 1=TR, 2=BL, 3=BR.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetTile8x8(int32 whichPT, int32 tileIndex, bool locked,
	uint32 chrAddr, const uint8* chrBytes, uint8 bgPalette,
	int32 whichNT, uint32 nameTileAddr,
	uint32 attrAddr, uint8 attrByte, uint8 attrQuadrant)
{
	// A null CHR pointer means there is no valid tile to inspect.
	if (!chrBytes) {
		Clear();
		return;
	}

	fWhichPatternTable = whichPT;
	fTileIndex = tileIndex;
	fLocked = locked;

	fCHRTileAddress = chrAddr;

	fQuadrantPalette = bgPalette % 4;

	if (!fLocked)
		fPalette = fQuadrantPalette;

	fIsTile8x16 = false;
	fUseSpritePalette = false;
	fFlipH = false;
	fFlipV = false;
	fValid = true;
	
	fWhichNameTable = whichNT;
	fNameTileAddress = nameTileAddr;
	fAttrAddress = attrAddr;
	fAttrByte = attrByte;
	fAttrQuadrant = attrQuadrant % 4;

	// Copy raw CHR bytes and decode them for drawing/analysis.
	memcpy(fCHRBytes, chrBytes, 16);

	memset(fCHRBytesBottom, 0, sizeof(fCHRBytesBottom));
	memset(fDecodedPixelsBottom, 0, sizeof(fDecodedPixelsBottom));

	DecodeTile();

	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawDecodedZoomed
//
// Draws a decoded 8x8 tile at a zoomed scale using the currently selected
// palette, then draws the hover-pixel box when applicable.
//
// Parameters:
//   decoded - 8x8 decoded pixel values, each 0-3.
//   origin  - Top-left position where the zoomed tile should be drawn.
//   scale   - Pixel scale factor.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawDecodedZoomed(uint8 decoded[8][8], BPoint origin, float scale)
{
	// Draw the tile using the *current* palette,
	// not a hardcoded palette 0.
	DrawTileWithBgPalette(decoded, origin, scale, fPalette);

	// hover pixel box
	if (!fHoverPixelValid) {
		return;
	}

	int32 px = fHoverPixelX;
	int32 py = fHoverPixelY;

	if (px < 0 || px >= 8 || py < 0 || py >= 16) {
		return;
	}

	// if this is the top tile of an 8x16 view, only draw hover box for rows 0-7 here
	if (py >= 8) {
		return;
	}

	BRect r(
		origin.x + px * scale,
		origin.y + py * scale,
		origin.x + (px + 1) * scale - 1.0f,
		origin.y + (py + 1) * scale - 1.0f
	);

	PushState();
	SetHighColor(255, 255, 255, 255);
	StrokeRect(r);
	StrokeRect(r.InsetByCopy(-1, -1));
	PopState();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawInfo
//
// Draws the left-side tile metadata block, raw CHR byte dump, and compact tile
// summary beneath the zoomed tile preview.
//
// Parameters:
//   point - Baseline position for the first metadata row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawInfo(BPoint point)
{
	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;

	const float labelX = point.x;
	const float valueX = point.x + 62.0f;

	float textY = point.y;
	BString line;

	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(labelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(valueX, textY));

		textY += lineH;
	};

	line.SetToFormat("%ld", (long)fWhichPatternTable);
	drawKV("PT:", line.String());

	line.SetToFormat("%u", (unsigned)fTileIndex);
	drawKV("Tile:", line.String());

	drawKV("State:", fLocked ? "LOCKED" : "HOVER");


	if (!fIsTile8x16) {
		line.SetToFormat("$%04X", (unsigned)fCHRTileAddress);
		drawKV("CHR:", line.String());
	} else {
		line.SetToFormat("$%04X", (unsigned)fCHRTileAddress);
		drawKV("CHR Top:", line.String());

		line.SetToFormat("$%04X", (unsigned)fCHRTileAddressBottom);
		drawKV("CHR Bot:", line.String());
	}

	if (fWhichNameTable >= 0) {
		static const char *kQuadrantNames[4] = {
			"TL", "TR", "BL", "BR"
		};

		int32 shift = (fAttrQuadrant % 4) * 2;

		line.SetToFormat("%ld", (long)fWhichNameTable);
		drawKV("NT:", line.String());

		line.SetToFormat("$%04X", (unsigned)fNameTileAddress);
		drawKV("Tile Addr:", line.String());

		line.SetToFormat("$%02X", (unsigned)fTileIndex);
		drawKV("Tile Index:", line.String());

		line.SetToFormat("$%04X", (unsigned)fAttrAddress);
		drawKV("Attr:", line.String());

		line.SetToFormat("$%02X", (unsigned)fAttrByte);
		drawKV("Attr Byte:", line.String());

		line.SetToFormat("%s  Shift:%ld", kQuadrantNames[fAttrQuadrant % 4],
			(long)shift);
		drawKV("Quadrant:", line.String());

		line.SetToFormat("%u", (unsigned)fQuadrantPalette);
		drawKV("Source Pal:", line.String());
	}

	line.SetToFormat("%u", (unsigned)fPalette);
	drawKV("Selected:", line.String());
	
	DrawPaletteSwatch(BPoint(valueX + 30.0f, textY - lineH - 10.0f));

	textY += 8.0f;

	// -------------------------------------------------
	// CHR bytes
	// -------------------------------------------------

	BFont oldFont;
	GetFont(&oldFont);

	BFont mono(be_fixed_font);
	SetFont(&mono);

	font_height monoFH;
	GetFontHeight(&monoFH);
	const float monoLineH = ceilf(monoFH.ascent + monoFH.descent + monoFH.leading) + 2.0f;

	const float cellW = StringWidth("FF ") + 2.0f;
	const int32 cols = 8;

	int32 totalBytes = fIsTile8x16 ? 32 : 16;
	int32 byteRows = (totalBytes + cols - 1) / cols;

	SetHighColor(0, 0, 0, 255);

	for (int32 i = 0; i < totalBytes; i++) {
		uint8 b;

		if (!fIsTile8x16) {
			b = fCHRBytes[i];
		} else {
			if (i < 16)
				b = fCHRBytes[i];
			else
				b = fCHRBytesBottom[i - 16];
		}

		line.SetToFormat("%02X", b);

		int32 col = i % cols;
		int32 row = i / cols;

		DrawString(line.String(), BPoint(point.x + col * cellW, textY + row * monoLineH));
	}

	float afterBytesY = textY + (byteRows * monoLineH) + 6.0f;

	SetFont(&oldFont);
	SetHighColor(0, 0, 0, 255);

	DrawTileSummary(point.x, afterBytesY);
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DecodeTile
//
// Decodes the current 16-byte 8x8 CHR tile into 2bpp pixel values stored in
// fDecodedPixels.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawPaletteSwatch
//
// Draws a small four-color swatch for the currently selected background
// palette.
//
// Parameters:
//   point - Top-left position of the swatch.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawPaletteSwatch(BPoint point)
{
	Mapper *mapper = nes::cart.mapper();
	BScreen screen(Window());
	const color_map *cmap = screen.ColorMap();

	if (!mapper || !cmap || !fHostPalette)
		return;

	uint8 colors[4];
	colors[0] = mapper->read_vram(0x3f00) & 0x3f;
	colors[1] = mapper->read_vram(0x3f00 + 1 + (fPalette * 4) + 0) & 0x3f;
	colors[2] = mapper->read_vram(0x3f00 + 1 + (fPalette * 4) + 1) & 0x3f;
	colors[3] = mapper->read_vram(0x3f00 + 1 + (fPalette * 4) + 2) & 0x3f;

	const float w = 18.0f;
	const float h = 10.0f;

	for (int i = 0; i < 4; i++) {
		rgb_color c = cmap->color_list[fHostPalette[colors[i]]];
		BRect r(point.x + i * w, point.y, point.x + (i + 1) * w - 2, point.y + h);

		SetHighColor(c);
		FillRect(r);

		SetHighColor(0, 0, 0, 255);
		StrokeRect(r);
	}
}


// -----------------------------------------------------------------------------
// CHRExplorerView::SetTile8x16
//
// Loads a paired 8x16 sprite tile into the explorer, decodes the top and bottom
// 8x8 tiles, records palette/source state, and schedules a redraw.
//
// Parameters:
//   whichPT       - Pattern table index that supplied the tile pair.
//   topTileIndex  - Tile index of the top half of the 8x16 sprite.
//   locked        - true if the source view is locked to this tile.
//   chrAddrTop    - CHR address of the top 8x8 tile.
//   chrTop        - Pointer to the top tile's 16 CHR bytes.
//   chrAddrBottom - CHR address of the bottom 8x8 tile.
//   chrBottom     - Pointer to the bottom tile's 16 CHR bytes.
//   bgPalette     - Source background/sprite palette number, 0-3.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::SetTile8x16(int32 whichPT, int32 topTileIndex, bool locked,
	uint32 chrAddrTop, const uint8* chrTop,
	uint32 chrAddrBottom, const uint8* chrBottom,
	uint8 bgPalette)
{
	// Both halves are required for an 8x16 tile.
	if (!chrTop || !chrBottom) {
		Clear();
		return;
	}

	fWhichPatternTable = whichPT;
	fTileIndex = topTileIndex;
	fLocked = locked;

	// Source palette from the caller.
	fQuadrantPalette = bgPalette % 4;

	// Preserve manual palette selection while locked, but update it normally
	// during hover/explore mode.
	if (!fLocked)
		fPalette = fQuadrantPalette;

	fCHRTileAddress = chrAddrTop;
	fCHRTileAddressBottom = chrAddrBottom;

	memcpy(fCHRBytes, chrTop, 16);
	memcpy(fCHRBytesBottom, chrBottom, 16);

	fWhichNameTable = -1;
	fNameTileAddress = 0;
	fAttrAddress = 0;
	fAttrByte = 0;
	fAttrQuadrant = 0;

	// Default CHR explorer behavior is non-sprite:
	// PatternTableView also uses this function, so OAM-specific sprite palette
	// and flip handling must be enabled separately by OAMDebugView.
	fIsTile8x16 = true;
	fUseSpritePalette = false;
	fFlipH = false;
	fFlipV = false;
	fValid = true;

	// Decode top tile.
	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = fCHRBytes[y + 0];
		uint8 secondPlane = fCHRBytes[y + 8];

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			fDecodedPixels[y][x] = pixel;
		}
	}

	// Decode bottom tile.
	for (int32 y = 0; y < 8; y++) {
		uint8 firstPlane = fCHRBytesBottom[y + 0];
		uint8 secondPlane = fCHRBytesBottom[y + 8];

		for (int32 x = 0; x < 8; x++) {
			int32 shift = 7 - x;
			uint8 pixel = (firstPlane >> shift) & 0x1;
			pixel |= ((secondPlane >> shift) & 0x1) << 1;
			fDecodedPixelsBottom[y][x] = pixel;
		}
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawTileWithBgPalette
//
// Draws a decoded 8x8 tile using a specific NES background palette.
//
// Parameters:
//   decoded   - 8x8 decoded pixel values, each 0-3.
//   origin    - Top-left draw position.
//   scale     - Pixel scale factor.
//   bgPalette - Palette number to use when looking up NES colors.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawTileWithBgPalette(const uint8 decoded[8][8],
	BPoint origin, float scale, uint8 bgPalette)
{
	Mapper* mapper = nes::cart.mapper();
	BScreen screen(Window());
	const color_map* cmap = screen.ColorMap();

	if (!mapper || !cmap || !fHostPalette)
		return;

	rgb_color pal[4];

	uint8 nes0 = mapper->read_vram(0x3f00) & 0x3f;
	uint8 nes1;
	uint8 nes2;
	uint8 nes3;

	if (fUseSpritePalette) {
		// Sprite palettes live at $3f10-$3f1f.
		//
		// Pixel 0 is transparent for real sprites, but in the CHR explorer
		// preview we draw it with the universal background color so the tile
		// still has a visible background, matching the OAM preview behavior.
		nes1 = mapper->read_vram(0x3f10 + (bgPalette * 4) + 1) & 0x3f;
		nes2 = mapper->read_vram(0x3f10 + (bgPalette * 4) + 2) & 0x3f;
		nes3 = mapper->read_vram(0x3f10 + (bgPalette * 4) + 3) & 0x3f;
	} else {
		// Background palettes live at $3F00-$3F0F.
		nes1 = mapper->read_vram(0x3f00 + 1 + (bgPalette * 4) + 0) & 0x3f;
		nes2 = mapper->read_vram(0x3f00 + 1 + (bgPalette * 4) + 1) & 0x3f;
		nes3 = mapper->read_vram(0x3f00 + 1 + (bgPalette * 4) + 2) & 0x3f;
	}

	pal[0] = cmap->color_list[fHostPalette[nes0]];
	pal[1] = cmap->color_list[fHostPalette[nes1]];
	pal[2] = cmap->color_list[fHostPalette[nes2]];
	pal[3] = cmap->color_list[fHostPalette[nes3]];

	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			uint8 pix = decoded[y][x] & 0x3;

			SetHighColor(pal[pix]);
			FillRect(BRect(
				origin.x + x * scale,
				origin.y + y * scale,
				origin.x + (x + 1) * scale - 1,
				origin.y + (y + 1) * scale - 1
			));
		}
	}

	SetHighColor(0, 0, 0, 255);
	StrokeRect(BRect(
		origin.x,
		origin.y,
		origin.x + 8 * scale - 1,
		origin.y + 8 * scale - 1
	));
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawPalettePreviewGrid
//
// Draws the 2x2 palette-preview grid showing the current tile rendered with all
// four background palettes.  The selected and source palettes are outlined.
//
// Parameters:
//   origin - Top-left position of the preview grid.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawPalettePreviewGrid(BPoint origin)
{
	if (!fValid)
		return;

	PushState();

	const float scale = 3.0f;
	const float cellW = 8.0f * scale;
	const float tileH = 8.0f * scale;
	const float previewH = fIsTile8x16 ? (tileH * 2.0f) : tileH;

	const float gapX = 14.0f;
	const float gapY = 18.0f;

	BString label;

	// Draw each palette variant in a 2x2 grid.
	for (int32 pal = 0; pal < 4; pal++) {
		int col = pal & 1;
		int row = pal >> 1;

		BPoint p(
			origin.x + col * (cellW + gapX),
			origin.y + row * (previewH + gapY)
		);

		// label
		label.SetToFormat("Pal %d", pal);
		SetHighColor(0, 0, 0, 255);
		DrawString(label.String(), BPoint(p.x, p.y - 4));

		// top tile
		DrawTileWithBgPalette(fDecodedPixels, p, scale, (uint8)pal);

		// bottom tile in 8x16 mode
		if (fIsTile8x16) {
			DrawTileWithBgPalette(
				fDecodedPixelsBottom,
				BPoint(p.x, p.y + tileH),
				scale,
				(uint8)pal
			);
		}

		BRect tileRect(
			p.x,
			p.y,
			p.x + cellW - 1,
			p.y + previewH - 1
		);

		bool isSelected = ((uint8)pal == fPalette);
		bool isSource = (fWhichNameTable >= 0 && (uint8)pal == fQuadrantPalette);

		if (isSelected && isSource) {
			SetHighColor(0, 0, 0, 255);
			StrokeRect(tileRect.InsetByCopy(-3, -3));

			SetHighColor(255, 0, 0, 255);
			StrokeRect(tileRect.InsetByCopy(-2, -2));
			StrokeRect(tileRect.InsetByCopy(-1, -1));
		} else if (isSelected) {
			SetHighColor(255, 0, 0, 255);
			StrokeRect(tileRect.InsetByCopy(-2, -2));
			StrokeRect(tileRect.InsetByCopy(-1, -1));
		} else if (isSource) {
			SetHighColor(0, 0, 0, 255);
			StrokeRect(tileRect.InsetByCopy(-2, -2));
		}
	}

	PopState();
}

// -----------------------------------------------------------------------------
// CHRExplorerView::UpdateHoverPixelFromMouse
//
// Converts the last mouse position into a pixel coordinate within the zoomed
// tile preview, updating hover-pixel state for the Pixel / CHR panel.
//
// Parameters:
//   None. Uses fLastMouse and current tile mode.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::UpdateHoverPixelFromMouse()
{
	fHoverPixelValid = false;
	fHoverPixelX = -1;
	fHoverPixelY = -1;

	if (!fValid || !fMouseInside)
		return;

	const float zoomScale = 8.0f;
	const BPoint zoomOrigin(12.0f, 52.0f);

	float localX = fLastMouse.x - zoomOrigin.x;
	float localY = fLastMouse.y - zoomOrigin.y;

	float totalH = fIsTile8x16 ? 16.0f * zoomScale : 8.0f * zoomScale;

	if (localX < 0.0f || localY < 0.0f)
		return;

	if (localX >= 8.0f * zoomScale || localY >= totalH)
		return;

	fHoverPixelX = (int32)(localX / zoomScale);
	fHoverPixelY = (int32)(localY / zoomScale);

	if (fHoverPixelX < 0 || fHoverPixelX >= 8)
		return;

	if (!fIsTile8x16) {
		if (fHoverPixelY < 0 || fHoverPixelY >= 8)
			return;
	} else {
		if (fHoverPixelY < 0 || fHoverPixelY >= 16)
			return;
	}

	fHoverPixelValid = true;
}


// -----------------------------------------------------------------------------
// CHRExplorerView::PalettePreviewAt
//
// Determines which palette-preview tile, if any, contains the supplied point.
//
// Parameters:
//   where - Point in this view's coordinate system.
//
// Returns:
//   Palette index 0-3 if the point is over a preview; -1 otherwise.
// -----------------------------------------------------------------------------
int32
CHRExplorerView::PalettePreviewAt(BPoint where) const
{
	if (!fValid)
		return -1;

	// Must match CHRExplorerView::Draw().
	const float minRightColumnX = 170.0f;
	const float rightColumnW = 130.0f;
	const float rightColumnX = std::max(minRightColumnX,
		Bounds().right - rightColumnW);
	const float rightColumnY = 44.0f;

	// Must match DrawPalettePreviewGrid().
	const float scale = 3.0f;
	const float cellW = 8.0f * scale;
	const float tileH = 8.0f * scale;
	const float previewH = fIsTile8x16 ? (tileH * 2.0f) : tileH;

	const float gapX = 14.0f;
	const float gapY = 18.0f;

	for (int32 pal = 0; pal < 4; pal++) {
		int32 col = pal & 1;
		int32 row = pal >> 1;

		BPoint p(
			rightColumnX + col * (cellW + gapX),
			rightColumnY + row * (previewH + gapY)
		);

		BRect r(
			p.x,
			p.y,
			p.x + cellW - 1.0f,
			p.y + previewH - 1.0f
		);

		// Slightly expand clickable area around the preview tile.
		r.InsetBy(-3.0f, -3.0f);

		if (r.Contains(where))
			return pal;
	}

	return -1;
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawQuadrantDiagram
//
// Draws a 2x2 attribute-quadrant diagram showing palette bits for each quadrant
// and highlighting the quadrant that supplied the current NameTable tile.
//
// Parameters:
//   origin - Top-left position of the quadrant diagram.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawQuadrantDiagram(BPoint origin)
{
	const float cell = 20.0f;

	for (int q = 0; q < 4; q++) {
		int col = q & 1;
		int row = q >> 1;

		BRect r(
			origin.x + col * cell,
			origin.y + row * cell,
			origin.x + col * cell + cell - 1,
			origin.y + row * cell + cell - 1
		);

		bool active = (q == (fAttrQuadrant % 4));
		uint8 pal = (fAttrByte >> (q * 2)) & 0x3;

		if (active) {
			SetHighColor(200, 200, 255, 255);
			FillRect(r);
		}

		SetHighColor(0, 0, 0, 255);
		StrokeRect(r);

		BString buf;
		buf.SetToFormat("%u", (unsigned)pal);
		DrawString(buf.String(), BPoint(r.left + 6.0f, r.bottom - 4.0f));
	}

	SetHighColor(0, 0, 0, 255);
	DrawString("TL", BPoint(origin.x,               origin.y - 2.0f));
	DrawString("TR", BPoint(origin.x + cell + 2.0f, origin.y - 2.0f));
	DrawString("BL", BPoint(origin.x,               origin.y + 2.0f * cell + 10.0f));
	DrawString("BR", BPoint(origin.x + cell + 2.0f, origin.y + 2.0f * cell + 10.0f));
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawTileSummary
//
// Draws a compact aligned summary of the current tile, including pattern table,
// tile index, CHR address, mode, palette, and lock/hover state.
//
// Parameters:
//   x - Left edge of the summary text.
//   y - Baseline of the summary title.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawTileSummary(float x, float y)
{
	if (!fValid)
		return;

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = x;
	const float valueX = x + 42.0f;

	float textY = y;

	BString s;

	SetHighColor(0, 0, 0, 255);
	DrawString("Tile Summary", BPoint(labelX, textY));
	textY += lineH;

	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(labelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(valueX, textY));

		textY += lineH;
	};

	s.SetToFormat("%ld", (long)fWhichPatternTable);
	drawKV("PT:", s.String());

	s.SetToFormat("$%02lX", (long)(fTileIndex & 0xff));
	drawKV("Tile:", s.String());

	if (!fIsTile8x16) {
		s.SetToFormat("$%04lX", (unsigned long)fCHRTileAddress);
		drawKV("CHR:", s.String());
	} else {
		s.SetToFormat("$%04lX/$%04lX", (unsigned long)fCHRTileAddress, 
						(unsigned long)fCHRTileAddressBottom);
		
		drawKV("CHR:", s.String());
	}
	
	s.SetToFormat("%s   Pal:%u", fIsTile8x16 ? "8x16" : "8x8", (unsigned)fPalette);
	drawKV("Mode:", s.String());

	drawKV("State:", fLocked ? "LOCKED" : "HOVER");
}


// -----------------------------------------------------------------------------
// CHRExplorerView::DrawCHRAnalysis
//
// Analyzes the decoded tile pixels and draws compact information about bitplane
// usage, colors used, and opaque-pixel count.
//
// Parameters:
//   x - Left edge of the analysis text.
//   y - Baseline of the analysis title.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CHRExplorerView::DrawCHRAnalysis(float x, float y)
{
	if (!fValid)
		return;

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = x;
	const float valueX = x + 48.0f;

	float textY = y;

	SetHighColor(0, 0, 0, 255);
	DrawString("CHR Analysis:", BPoint(labelX, textY));
	textY += lineH;

	auto drawKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80, 255);
		DrawString(label, BPoint(labelX, textY));

		SetHighColor(0, 0, 0, 255);
		DrawString(value, BPoint(valueX, textY));

		textY += lineH;
	};

	bool usedP0 = false;
	bool usedP1 = false;
	bool usedColor[4] = { false, false, false, false };
	int32 opaquePixels = 0;

	const int32 height = fIsTile8x16 ? 16 : 8;

	// Walk decoded pixels to collect simple per-tile usage stats.
	for (int32 py = 0; py < height; py++) {
		for (int32 px = 0; px < 8; px++) {
			uint8 value;

			if (!fIsTile8x16 || py < 8) {
				value = fDecodedPixels[py][px] & 0x3;
			} else {
				value = fDecodedPixelsBottom[py - 8][px] & 0x3;
			}

			if (value & 0x1)
				usedP0 = true;

			if (value & 0x2)
				usedP1 = true;

			usedColor[value] = true;

			if (value != 0)
				opaquePixels++;
		}
	}

	drawKV("P0:", usedP0 ? "used" : "blank");
	drawKV("P1:", usedP1 ? "used" : "blank");

	BString colors;

	for (int32 i = 0; i < 4; i++) {
		if (!usedColor[i])
			continue;

		if (!colors.IsEmpty())
			colors.Append(" ");

		BString tmp;
		tmp.SetToFormat("%ld", (long)i);
		colors.Append(tmp);
	}

	if (colors.IsEmpty())
		colors.SetTo("-");

	drawKV("Colors:", colors.String());

	BString opaque;
	opaque.SetToFormat("%ld/%ld",
						(long)opaquePixels, (long)(height * 8));
	drawKV("Opaque:", opaque.String());
}


// -----------------------------------------------------------------------------
// CHRExplorerView::PreferredWidth
//
// Reports the preferred width for the CHR explorer side panel.
//
// Parameters:
//   None.
//
// Returns:
//   Preferred view width in pixels.
// -----------------------------------------------------------------------------
float
CHRExplorerView::PreferredWidth()
{
	return 340.0f;
}


// -----------------------------------------------------------------------------
// CHRExplorerView::PreferredHeightForNameTable
//
// Reports the preferred combined window height when the CHR explorer is attached
// to a NameTable window.
//
// Parameters:
//   None.
//
// Returns:
//   Preferred NameTable debugger height in pixels.
// -----------------------------------------------------------------------------
float
CHRExplorerView::PreferredHeightForNameTable()
{
	return 580.0f;
}


// -----------------------------------------------------------------------------
// CHRExplorerView::PreferredHeightForPatternTable
//
// Reports the preferred combined window height when the CHR explorer is attached
// to a PatternTable window.
//
// Parameters:
//   None.
//
// Returns:
//   Preferred PatternTable debugger height in pixels.
// -----------------------------------------------------------------------------
float
CHRExplorerView::PreferredHeightForPatternTable()
{
	return 510.0f;
}


