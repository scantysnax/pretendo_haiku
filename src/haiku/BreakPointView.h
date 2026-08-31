#ifndef _BREAK_POINT_VIEW_H_
#define _BREAK_POINT_VIEW_H_

#include <View.h>

class PretendoWindow;


// -----------------------------------------------------------------------------
// BreakPointView
//
// Displays and manages debugger BreakPoint and watchpoint conditions.
//
// Supported conditions include:
//
//   - CPU memory READ watchpoints.
//   - CPU memory WRITE watchpoints.
//   - Execute BreakPoints.
//   - SP-threshold BreakPoint.
//   - Stack-wrap BreakPoint.
//   - Current debugger break reason.
//
// READ watchpoints, WRITE watchpoints, and Execute BreakPoints support mouse
// selection, unified keyboard navigation, deletion, and accumulated hit counts.
//
// Execute BreakPoints additionally support navigation to the CPU disassembler.
//
// Address conditions are presented and traversed in this order:
//
//   READ -> WRITE -> Execute
// -----------------------------------------------------------------------------
class BreakPointView : public BView
{
	public:
			BreakPointView (BRect frame, PretendoWindow *parent);
	virtual	~BreakPointView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MouseDown (BPoint where);

	private:
	enum breakpoint_entry_mode {
		BREAKPOINT_ENTRY_NONE = 0,
		BREAKPOINT_ENTRY_EXECUTE,
		BREAKPOINT_ENTRY_READ,
		BREAKPOINT_ENTRY_WRITE
	};

	enum watchpoint_selection_type {
		WATCHPOINT_SELECTION_NONE = 0,
		WATCHPOINT_SELECTION_READ,
		WATCHPOINT_SELECTION_WRITE
	};

	struct execute_breakpoint_entry_t {
		uint16 address = 0x0000;
		uint32 hitCount = 0;
	};

	struct read_watchpoint_entry_t {
		uint16 address = 0x0000;
		uint32 hitCount = 0;
		
	};

	struct write_watchpoint_entry_t {
		uint16 address = 0x0000;
		uint32 hitCount = 0;
	};

	
	static const int32 kMaximumDisplayedBreakPoints = 16;

	private:
	void DrawHeaderPanel();
	void DrawConditionPanel();
	void DrawReadWatchPointPanel();
	void DrawWriteWatchPointPanel();
	void DrawExecuteBreakPointPanel();
	void DrawNoROMMessage (BRect panel);

	private:
	int32 CaptureReadWatchPoints (read_watchpoint_entry_t *entries, int32 capacity) const;
	int32 CaptureWriteWatchPoints (write_watchpoint_entry_t *entries, int32 capacity) const;
	int32 CaptureExecuteBreakPoints (execute_breakpoint_entry_t *entries, int32 capacity) const;

	private:
	int32 ExecuteBreakPointCount() const;
	int32 ReadWatchPointCount() const;
	int32 WriteWatchPointCount() const;

	private:
	bool HasROMLoaded() const;
	const char *CurrentBreakReasonText() const;

	private:
	bool ExecuteBreakPointAddressForPoint (BPoint where, uint16 &address) const;
	bool ReadWatchPointAddressForPoint (BPoint where, uint16 &address) const;
	bool WriteWatchPointAddressForPoint (BPoint where, uint16 &address) const;
	bool SelectedExecuteBreakPoint (uint16 &address) const;
	bool SelectedWatchPoint (watchpoint_selection_type &type, uint16 &address) const;
	void MoveBreakPointSelection (int32 direction);

	private:
	PretendoWindow *fParent = nullptr;

	bool fHasSelectedExecuteBreakPoint = false;
	uint16 fSelectedExecuteBreakPoint = 0x0000;

	watchpoint_selection_type fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
	uint16 fSelectedWatchPointAddress = 0x0000;

	breakpoint_entry_mode fEntryMode = BREAKPOINT_ENTRY_NONE;

	uint16 fPendingBreakPointAddress = 0x0000;
	int32 fPendingBreakPointDigits = 0;
};


#endif	// _BREAK_POINT_VIEW_H_

