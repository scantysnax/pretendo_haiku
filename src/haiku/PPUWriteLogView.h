
#ifndef _PPU_WRITE_LOG_VIEW_H_
#define _PPU_WRITE_LOG_VIEW_H_

#include <View.h>

#include <cmath>
#include <vector>

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
//
// The emulator's write log is copied into debugger-owned storage before it is
// displayed.  This gives the view stable redraws and allows freeze mode to
// preserve the exact visible log state.
// -----------------------------------------------------------------------------
class PPUWriteLogView : public BView
{
	public:
			PPUWriteLogView(BRect frame, PretendoWindow *parent);
	virtual ~PPUWriteLogView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void Pulse();

	private:
	void CaptureLogSnapshot();

	void DrawHeaderPanel();
	void DrawLogPanel();

	private:
	const char *RegisterName (uint16 address) const;
	void DescribeWrite (const nes::ppu::ppu_write_log_entry_t &entry, BString &text) const;

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage(BRect panel);


	bool fFreezeUpdates = false;

	std::vector<nes::ppu::ppu_write_log_entry_t> fLogSnapshot;
};


#endif // _PPU_WRITE_LOG_VIEW_H_
