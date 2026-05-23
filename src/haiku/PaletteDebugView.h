#ifndef _PALETTE_DEBUG_VIEW_H_
#define _PALETTE_DEBUG_VIEW_H_


#include <View.h>
#include <Rect.h>
#include <Point.h>
#include <SupportDefs.h>

class PretendoWindow;


class PaletteDebugView : public BView
{
	public:
	PaletteDebugView(BRect frame, PretendoWindow* parent);
	virtual ~PaletteDebugView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* message);
	virtual void MouseDown(BPoint where);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Pulse();
	
	public:
	void SetExternalHighlight(bool sprites, int32 palette, int32 entry = -1);
	void ClearExternalHighlight();

	private:
	void DrawHeaderUI();
	void DrawBackgroundPalettes();
	void DrawSpritePalettes();
	void DrawSelectedInfo();
	void DrawPaletteEntry(BRect r, uint16 address, bool selected);
	void DrawPalettePanel(BRect panel, const char* title, bool sprites);

	private:
	bool PaletteEntryAt(BPoint where, uint16& outAddress) const;
	uint16 ResolvePaletteAddress(uint16 address) const;
	uint8 ReadPalette(uint16 address) const;

	private:
	PretendoWindow* fParent = nullptr;
	uint8* fHostPalette = nullptr;

	private:
	bool fFreezeUpdates = false;
	bool fMouseInside = false;
	bool fEntryLocked = false;

	private:
	uint16 fHoverAddress = 0x3f00;
	uint16 fLockedAddress = 0x3f00;
	
	private:
	bool fHasExternalHighlight = false;
	bool fExternalHighlightSprites = false;
	int32 fExternalHighlightPalette = -1;
	int32 fExternalHighlightEntry = -1;
};


#endif // _PALETTE_DEBUG_VIEW_H_


