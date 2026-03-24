#ifndef _CHR_EXLORER_VIEW_H_
#define _CHR_EXLORER_VIEW_H_

#include <View.h>


class CHRExplorerView : public BView
{
	public:
			CHRExplorerView (BRect frame);
	virtual ~CHRExplorerView();

	virtual void Draw (BRect updateRect);
	
	public:
	void Clear();
	void SetHostPalette (uint8 const* pal);

	void SetTile (int32 whichPT, int32 tileIndex, bool locked,
	uint32 chrAddr, const uint8 *chrBytes, uint8 bgPalette,
	int32 whichNT, uint32 attrAddr, uint8 attrByte, uint8 attrQuadrant);

	void SetTile16(int32 whichPT,
		int32 topTileIndex, bool locked,
		uint32 chrAddrTop, const uint8 *chrTop,
		uint32 chrAddrBottom, const uint8 *chrBottom,
		uint8 bgPalette);

	private:
	void DecodeTile();
	void DrawDecodedZoomed (const uint8 decoded[8][8], BPoint origin, float scale);
	void DrawTileZoomed (BPoint origin, float scale);
	void DrawInfo (BPoint point);
	void DrawPaletteSwatch (BPoint point);
	uint8 ColorForPixel (uint8 bgPalette, uint8 pixel) const;

	private:
	// host palette mapping: NES color index -> host CMAP8 index
	uint8 const *fHostPalette = nullptr;

	// selection / mode
	bool fValid = false;
	bool fLocked = false;
	bool fIsTile16 = false;

	int32 fWhichPatternTable = 0;
	int32 fWhichNameTable = -1;   // 0-3, -1 if not from nametable context

	uint8  fTileIndex = 0;
	uint8  fBgPalette = 0;

	// chr addresses
	uint32 fCHRTileAddress = 0;
	uint32 fCHRTileAddressBottom = 0;

	// attributes
	uint32 fAttrAddress = 0;
	uint8 fAttrByte = 0;
	uint8 fAttrQuadrant = 0; // 0: top-left, 1: top-right,  3: bottom-left, 3: bottom-right

	// raw CHR data
	uint8 fCHRBytes[16] = {0};
	uint8 fCHRBytesBottom[16] = {0};

	// already decoded 2bpp pixels
	uint8 fDecodedPixels[8][8] = {{0}};
	uint8 fDecodedPixelsBottom[8][8] = {{0}};
};

#endif
