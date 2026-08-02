#ifndef _CPU_TRACE_VIEW_H_
#define _CPU_TRACE_VIEW_H_


#include "CPUTraceView.h"

#include "Cart.h"
#include "Cpu.h"
#include "CPUDisasm.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"

#include <Font.h>
#include <Message.h>
#include <String.h>
#include <Window.h>

#include <cmath>
#include <vector>

class BScrollBar;
class CPUTraceScrollBar;
class PretendoWindow;


// -----------------------------------------------------------------------------
// CPUTraceFrozenEntry
//
// Stable row data captured when the CPU trace view is frozen.  The CPU trace
// entry stores cycle/register/opcode data, while instruction stores the decoded
// instruction text as it appeared at freeze time.
// -----------------------------------------------------------------------------
struct CPUTraceFrozenEntry {
	nes::cpu::cpu_trace_entry_t trace;
	BString instruction;
};


// -----------------------------------------------------------------------------
// CPUTraceView
//
// Displays the recent CPU execution trace captured by the CPU core.  The trace
// is a circular history of executed instructions, including cycle count,
// instruction address, opcode bytes, decoded instruction text, and CPU register
// state.
//
// Initial controls:
//
//   Space - freeze/unfreeze the trace view
//   C     - clear CPU trace history
//   End   - follow newest trace entry
// -----------------------------------------------------------------------------
class CPUTraceView : public BView
{
	public:
			CPUTraceView(BRect frame, PretendoWindow *parent);
	virtual ~CPUTraceView();

	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MessageReceived (BMessage *message);
	virtual void MouseDown (BPoint where);
	virtual void Pulse();
	virtual void FrameResized (float width, float height);

	bool HandleShortcut (const char *bytes, int32 numBytes);

	void ToggleFreeze();
	void ClearTrace();
	void FollowNewest();

	private:
	void DrawHeaderPanel();
	void DrawTracePanel();
	void DrawNoROMMessage (BRect panel);
	void DrawSelectedTraceInfo (BRect panel);
	bool SelectedTraceEntry (nes::cpu::cpu_trace_entry_t &entry, BString &instruction) const;

	private:
	bool HasROMLoaded() const;

	private:
	void ScrollLines(int32 lines);
	void JumpToNewest();

	private:
	uint32 TraceDisplayCount() const;
	bool TraceDisplayEntry(uint32 index, nes::cpu::cpu_trace_entry_t &entry) const;
	bool TraceDisplayInstruction(uint32 index, BString &instruction) const;
	bool TraceIndexForPoint (BPoint where, uint32& index) const;
	void CaptureSnapshot();

	private:
	void LayoutScrollBar();
	void UpdateScrollBar();
	void ScrollBarChanged(float value);
	int32 VisibleTraceRows() const;

	private:
	friend class CPUTraceScrollBar;
	BScrollBar *fScrollBar = nullptr;
	bool fUpdatingScrollBar = false;

	private:
	bool fFreezeUpdates = false;
	bool fFollowNewest = true;

	uint32 fBaseTraceIndex = 0;
	std::vector<CPUTraceFrozenEntry> fFrozenEntries;
	bool fHasSelectedTraceIndex = false;
	uint32 fSelectedTraceIndex = 0;
};

#endif // _CPU_TRACE_VIEW_H_
