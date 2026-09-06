#ifndef _APU_WRITE_LOG_VIEW_H_
#define _APU_WRITE_LOG_VIEW_H_

#include <ScrollBar.h>
#include <View.h>

#include <cmath>
#include <vector>

#include "Apu.h"
#include "Cart.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"

class APUWriteLogScrollBar;


constexpr double kNTSCCPUClock = 1789773.0;

const uint16 kNoisePeriodTable[16] = {
	4, 8, 16, 32, 64, 96, 128, 160,
	202, 254, 380, 508, 762, 1016, 2034, 4068
};


const uint8 kLengthCounterTable[32] = {
	10, 254, 20, 2, 40, 4, 80, 6,
	160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22,
	192, 24, 72, 26, 16, 28, 32, 30
};


// -----------------------------------------------------------------------------
// APUWriteLogView
//
// Debugger view for inspecting recent CPU writes to APU registers.  The view
// displays a rolling log of writes to the Square, Triangle, Noise, DMC, status,
// and frame-counter registers.
//
// The emulator's write log is copied into debugger-owned storage before it is
// displayed.  This gives the view stable redraws and allows freeze mode to
// preserve the exact visible log state.
// -----------------------------------------------------------------------------
class APUWriteLogView : public BView
{
	public:
			APUWriteLogView(BRect frame, PretendoWindow *parent);
	virtual ~APUWriteLogView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MessageReceived(BMessage *message);
	virtual void Pulse();

	private:
	friend class APUWriteLogScrollBar;
	
	private:
	void CaptureLogSnapshot();

	private:
	void DrawHeaderPanel();
	void DrawLogPanel();

	private:
	int32 VisibleRowCount() const;
	void ScrollRows (int32 rows);
	void ScrollPages (int32 pages);
	void FollowNewest();
	
	private:
	void UpdateScrollBar();
	void ScrollBarValueChanged (float value);
	
	private:
	uint16 NoiseTimerPeriodFromIndex (uint8 index) const;
	double NoiseClockRateHzFromIndex (uint8 index) const;

	private:
	const char *RegisterName (uint16 address) const;
	const char *ChannelName (uint16 address) const;
	void DescribeWrite (const nes::apu::apu_write_log_entry_t &entry, BString &text) const;

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage (BRect panel);

	private:
	bool fFreezeUpdates = false;
	bool fFollowNewest = true;
	int32 fFirstVisibleRow = 0;

	private:
	BScrollBar *fScrollBar = nullptr;

	private:
	std::vector<nes::apu::apu_write_log_entry_t> fLogSnapshot;
};


#endif // _APU_WRITE_LOG_VIEW_H_

