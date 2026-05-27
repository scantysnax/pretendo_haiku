#ifndef _CHR_EXPLORER_VIEW_H_
#define _CHR_EXPLORER_VIEW_H_

#include <View.h>
#include <Screen.h>
#include <String.h>


// -----------------------------------------------------------------------------
// CHRExplorerView
//
// Side-panel explorer used by the PatternTable and NameTable debugger windows.
// It displays the selected CHR tile, palette previews, raw CHR bytes, decoded
// pixel/bitplane information, NameTable attribute context, and compact tile
// analysis.
// -----------------------------------------------------------------------------

class CHRExplorerView : public BView
{
	public:
			CHRExplorerView (BRect frame);
	virtual ~CHRExplorerView();

	public:
	virtual void AttachedToWindow();
	virtual void KeyDown (const char  *bytes, int32 numBytes);
	virtual void MessageReceived (BMessage* message);
	virtual void MouseDown (BPoint where);
	virtual void MouseMoved (BPoint where, uint32 transit, const BMessage* message);
	virtual void Draw (BRect updateRect);

	public:
	void Clear();
	void SetHostPalette (uint8 *palette);
	void SetTile8x8 (int32 whichPT, int32 tileIndex, bool locked,
					uint32 chrAddr, const uint8* chrBytes, uint8 bgPalette,
					int32 whichNT, uint32 nameTileAddr,
					uint32 attrAddr, uint8 attrByte, uint8 attrQuadrant);
	
	void SetTile8x16 (int32 whichPT,
					 int32 topTileIndex, bool locked,
					 uint32 chrAddrTop, const uint8 *chrTop,
					 uint32 chrAddrBottom, const uint8 *chrBottom,
					 uint8 bgPalette);

	void UpdateHoverPixelFromMouse();

	private:
	void DecodeTile();
	void DrawDecodedZoomed (uint8 decoded[8][8], BPoint origin, float scale);
	void DrawInfo (BPoint point);
	void DrawPaletteSwatch (BPoint point);
	void DrawPalettePreviewGrid (BPoint origin);
	void DrawTileWithBgPalette (const uint8 decoded[8][8], BPoint origin, float scale, uint8 bgPalette);
	int32 PalettePreviewAt (BPoint where) const;
	void DrawQuadrantDiagram (BPoint origin);
	void DrawTileSummary (float x, float y);
	void DrawCHRAnalysis (float x, float y);

	public:
	// Preferred dimensions used by the debugger windows.
	static float PreferredWidth();
	static float PreferredHeightForNameTable();
	static float PreferredHeightForPatternTable();
	
	private:
	// Host palette mapping: NES color index -> host CMAP8 index.
	uint8 *fHostPalette = nullptr;

	// Current tile validity and mode.
	bool fValid = false;
	bool fLocked = false;
	bool fIsTile8x16 = false;

	// Source pattern/name table identity.
	int32 fWhichPatternTable = 0;
	int32 fWhichNameTable = -1;   // 0-3, -1 if not from NameTable context.

	// Current tile and selected/source palette information.
	int32 fTileIndex = 0;
	uint8 fPalette = 0;
	uint8 fQuadrantPalette = 0;

	// CHR addresses for 8x8 or 8x16 modes.
	uint32 fCHRTileAddress = 0;
	uint32 fCHRTileAddressBottom = 0;

	// Attribute information for NameTable-originated tiles.
	uint32 fAttrAddress = 0;
	uint8 fAttrByte = 0;
	uint8 fAttrQuadrant = 0; // 0=TL, 1=TR, 2=BL, 3=BR.

	// Raw CHR data.  fCHRBytesBottom is used only in 8x16 mode.
	uint8 fCHRBytes[16] = {0};
	uint8 fCHRBytesBottom[16] = {0};

	// Decoded 2bpp pixels.  Bottom storage is used only in 8x16 mode.
	uint8 fDecodedPixels[8][8] = {{0}};
	uint8 fDecodedPixelsBottom[8][8] = {{0}};

	// Hovered pixel within the zoomed preview.
	int32 fHoverPixelX = -1;
	int32 fHoverPixelY = -1;
	bool fHoverPixelValid = false;

	// Last mouse position inside this view.
	BPoint fLastMouse = BPoint(-1, -1);
	bool fMouseInside = false;

	// VRAM address of the source NameTable tile byte.
	uint32 fNameTileAddress = 0;
};

#endif

