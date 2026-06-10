#ifndef _PALETTE_DEBUG_VIEW_H_
#define _PALETTE_DEBUG_VIEW_H_


#include <View.h>
#include <Rect.h>
#include <Point.h>
#include <SupportDefs.h>

class PretendoWindow;


// -----------------------------------------------------------------------------
// PaletteDebugView
//
// Debugger view for inspecting NES background and sprite palette RAM.  The
// view supports hover/click inspection, frozen live updates, mirrored-entry
// visualization, and external highlights from other debugger views.
// -----------------------------------------------------------------------------
class PaletteDebugView : public BView
{
	public:
	// Creates the palette debugger view.
	PaletteDebugView(BRect frame, PretendoWindow* parent);
	// Destroys the palette debugger view.
	virtual ~PaletteDebugView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* message);
	virtual void MouseDown(BPoint where);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Pulse();
	
	public:
	// Sets or clears highlights requested by other debugger views.
	void SetExternalHighlight(bool sprites, int32 palette, int32 entry = -1);
	void ClearExternalHighlight();

	private:
	// Draws the palette debugger panels and selected-entry information.
	void DrawHeaderUI();
	void DrawBackgroundPalettes();
	void DrawSpritePalettes();
	void DrawSelectedInfo();
	void DrawPaletteEntry(BRect r, uint16 address, bool selected);
	void DrawPalettePanel(BRect panel, const char* title, bool sprites);

	private:
	// Palette address lookup and NES palette RAM helpers.
	bool PaletteEntryAt(BPoint where, uint16& outAddress) const;
	uint16 ResolvePaletteAddress(uint16 address) const;
	uint8 ReadPalette(uint16 address) const;

	private:
	// Parent window and palette mapping state.
	PretendoWindow* fParent = nullptr;
	uint8* fHostPalette = nullptr;

	private:
	// Local interaction state.
	bool fFreezeUpdates = false;
	bool fMouseInside = false;
	bool fEntryLocked = false;

	private:
	// Active palette RAM addresses.
	uint16 fHoverAddress = 0x3f00;
	uint16 fLockedAddress = 0x3f00;
	
	private:
	// External highlight state supplied by other debugger views.
	bool fHasExternalHighlight = false;
	bool fExternalHighlightSprites = false;
	int32 fExternalHighlightPalette = -1;
	int32 fExternalHighlightEntry = -1;
};


#endif // _PALETTE_DEBUG_VIEW_H_


