#ifndef _PPU_WRITE_LOG_VIEW_H_
#define _PPU_WRITE_LOG_VIEW_H_

#include <ScrollBar.h>
#include <View.h>

#include <cmath>
#include <vector>

#include "PretendoWindow.h"


class PPUWriteLogScrollBar;


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
	virtual void MessageReceived(BMessage *message);
	virtual void Pulse();

	private:
	friend class PPUWriteLogScrollBar;

	void CaptureLogSnapshot();

	void DrawHeaderPanel();
	void DrawLogPanel();

	private:
	int32 VisibleRowCount() const;
	void ScrollRows (int32 rows);
	void ScrollPages (int32 pages);
	void FollowNewest();

	void UpdateScrollBar();
	void ScrollBarValueChanged (float value);

	private:
	const char *RegisterName (uint16 address) const;
	void DescribeWrite (const nes::ppu::ppu_write_log_entry_t &entry, BString &text) const;

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage(BRect panel);

	private:
	bool fFreezeUpdates = false;
	bool fFollowNewest = true;

	int32 fFirstVisibleRow = 0;

	BScrollBar *fScrollBar = nullptr;

	std::vector<nes::ppu::ppu_write_log_entry_t> fLogSnapshot;
};


#endif // _PPU_WRITE_LOG_VIEW_H_
