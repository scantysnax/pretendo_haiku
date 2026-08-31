
#ifndef _ZERO_PAGE_VIEW_H_
#define _ZERO_PAGE_VIEW_H_

#include <View.h>

#include <cmath>

#include "Bus.h"
#include "Cart.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// ZeroPageView
//
// Displays CPU zero page RAM, $0000-$00FF, as a 16x16 byte grid with live/frozen
// snapshots, changed-byte highlighting, and selected-byte inspection.
// -----------------------------------------------------------------------------
class ZeroPageView : public BView
{
	public:
			ZeroPageView (BRect frame, PretendoWindow *parent);
	virtual ~ZeroPageView();

	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MouseDown (BPoint where);
	virtual void Pulse();

	private:
	void DrawHeaderUI();
	void DrawZeroPageGrid();
	void DrawSelectedBytePanel();
	void DrawNoROMMessage (BRect panel);

	private:
	void CaptureZeroPageSnapshot();
	void ResetDebuggerState();
	bool HasROMLoaded() const;
	bool AddressForPoint (BPoint where, uint16 &address) const;
	void MoveSelection (int32 delta);
	int32 ChangedByteCount() const;

	private:
	PretendoWindow *fParent = nullptr;

	private:
	bool fFreezeUpdates = false;
	bool fHaveSnapshot = false;

	private:
	uint8 fBytes[0x100] = {};
	uint8 fPreviousBytes[0x100] = {};
	bool fChanged[0x100] = {};
	uint8 fChangeAge[0x100] = {};

	private:
	bool fHasSelectedAddress = false;
	uint16 fSelectedAddress = 0x0000;
	
	private:
	bool fHadROMLoaded = false;
};


#endif // _ZERO_PAGE_VIEW_H_

