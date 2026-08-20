
#ifndef _PPU_WRITE_LOG_VIEW_H_
#define _PPU_WRITE_LOG_VIEW_H_

#include <View.h>

#include <cmath>

#include "Cart.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"
#include "Ppu.h"


// -----------------------------------------------------------------------------
// PPUWriteLogView
//
// Debugger view for inspecting recent CPU writes to PPU-facing registers.  The
// view displays a rolling log of writes to PPUCTRL, PPUMASK, OAMADDR, OAMDATA,
// PPUSCROLL, PPUADDR, PPUDATA, and OAM DMA.
//
// The log is useful for debugging palette uploads, scroll writes, nametable
// updates, sprite DMA, mid-frame effects, and status bar/split-screen behavior.
// -----------------------------------------------------------------------------


class PPUWriteLogView : public BView
{
	public:
			PPUWriteLogView (BRect frame, PretendoWindow *parent);
	virtual ~PPUWriteLogView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void Pulse();

	private:
	void DrawHeaderPanel();
	void DrawLogPanel();

	private:
	const char *RegisterName (uint16 address) const;
	void DescribeWrite (uint16 address, uint8 value, BString &text) const;
	
	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage (BRect panel);

	private:
	PretendoWindow *fParent = nullptr;
	bool fFreezeUpdates = false;
};


#endif // _PPU_WRITE_LOG_VIEW_H_

