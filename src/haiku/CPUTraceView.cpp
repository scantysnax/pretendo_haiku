
#include "CPUTraceView.h"


// -----------------------------------------------------------------------------
// CPUTraceScrollBar
//
// Small scrollbar subclass that forwards value changes back to CPUTraceView.
//
// The scrollbar is intentionally not attached to CPUTraceView as a normal
// scrolling target.  Moving the scrollbar changes fBaseTraceIndex only; it does
// not physically scroll the BView contents.
// -----------------------------------------------------------------------------
class CPUTraceScrollBar : public BScrollBar
{
	public:
	CPUTraceScrollBar (BRect frame, const char *name, CPUTraceView *owner)
		: BScrollBar(frame, name, nullptr, 0.0f, 0.0f, B_VERTICAL),
		fOwner(owner)
	{
	}

	virtual void ValueChanged (float value)
	{
		if (fOwner) {
			fOwner->ScrollBarChanged(value);
		}
	}

	private:
	CPUTraceView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// CPUTraceView::CPUTraceView
//
// Creates the CPU trace view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow.  Currently unused by the view; ownership and
//            lifecycle callbacks are handled by CPUTraceWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
CPUTraceView::CPUTraceView (BRect frame, PretendoWindow *parent)
	: BView(
		frame, "cpu trace view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
{
	fParent = parent;
	fScrollBar = new CPUTraceScrollBar(BRect(0.0f, 0.0f, 0.0f, 0.0f), "cpu trace scroll", this);

	AddChild(fScrollBar);
}


// -----------------------------------------------------------------------------
// CPUTraceView::~CPUTraceView
//
// Destructor.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUTraceView::~CPUTraceView()
{
}


// -----------------------------------------------------------------------------
// CPUTraceView::AttachedToWindow
//
// Completes CPU trace view setup after attachment to a window.  The scrollbar is
// laid out, keyboard focus is enabled, and pointer events are requested for
// future row selection support.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);

	LayoutScrollBar();
	UpdateScrollBar();

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	MakeFocus(true);
}


// -----------------------------------------------------------------------------
// CPUTraceView::Draw
//
// Draws the CPU trace viewer.
//
// Parameters:
//   updateRect - Dirty region supplied by BeAPI.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();
	DrawTracePanel();
}


// -----------------------------------------------------------------------------
// CPUTraceView::KeyDown
//
// Handles keyboard controls for the CPU trace viewer.
//
// Parameters:
//   bytes    - Key bytes received from the keyboard event.
//   numBytes - Number of bytes in the key event.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::KeyDown(const char *bytes, int32 numBytes)
{
	if (HandleShortcut(bytes, numBytes)) {
		return;
	}

	BView::KeyDown(bytes, numBytes);
}


// -----------------------------------------------------------------------------
// CPUTraceView::HandleShortcut
//
// Handles keyboard shortcuts for the CPU trace view.
//
// Parameters:
//   bytes    - Key bytes from KeyDown.
//   numBytes - Number of key bytes.
//
// Returns:
//   true if the key was handled.
// -----------------------------------------------------------------------------
bool
CPUTraceView::HandleShortcut(const char* bytes, int32 numBytes)
{
	if (!bytes || numBytes <= 0) {
		return false;
	}

	switch (bytes[0]) {
		case ' ':
			ToggleFreeze();
			return true;

		case 'c':
		case 'C':
			ClearTrace();
			return true;

		case 'd':
		case 'D':
		case B_ENTER:
		{
			if (!fParent) {
				return true;
			}

			uint16 address = 0x0000;

			if (SelectedTraceAddress(address)) {
				fParent->JumpCPUDisasmToAddress(address);
			}

			return true;
		}

		case B_END:
			FollowNewest();
			return true;

		case B_UP_ARROW:
			if (!fFreezeUpdates) {
				CaptureSnapshot();
				fFreezeUpdates = true;
			}

			fFollowNewest = false;
			ScrollLines(-1);
			return true;

		case B_DOWN_ARROW:
			if (!fFreezeUpdates) {
				CaptureSnapshot();
				fFreezeUpdates = true;
			}

			fFollowNewest = false;
			ScrollLines(1);
			return true;

		case B_PAGE_UP:
			if (!fFreezeUpdates) {
				CaptureSnapshot();
				fFreezeUpdates = true;
			}

			fFollowNewest = false;
			ScrollLines(-VisibleTraceRows());
			return true;

		case B_PAGE_DOWN:
			if (!fFreezeUpdates) {
				CaptureSnapshot();
				fFreezeUpdates = true;
			}

			fFollowNewest = false;
			ScrollLines(VisibleTraceRows());
			return true;

		default:
			break;
	}

	return false;
}


// -----------------------------------------------------------------------------
// CPUTraceView::ToggleFreeze
//
// Toggles between live trace display and a local frozen snapshot.  Freezing does
// not stop CPU trace capture; it copies the current trace into fFrozenEntries so
// the displayed backtrace remains stable while the emulator continues running.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::ToggleFreeze()
{
	if (!fFreezeUpdates) {
		CaptureSnapshot();

		fFreezeUpdates = true;
		fFollowNewest = false;

		const uint32 count = TraceDisplayCount();
		const int32 visibleRows = VisibleTraceRows();

		if (count > static_cast<uint32>(visibleRows)) {
			fBaseTraceIndex = count - static_cast<uint32>(visibleRows);
		} else {
			fBaseTraceIndex = 0;
		}

		UpdateScrollBar();
		Invalidate();
		return;
	}

	fFrozenEntries.clear();

	fFreezeUpdates = false;
	fFollowNewest = true;

	JumpToNewest();
}


// -----------------------------------------------------------------------------
// CPUTraceView::ClearTrace
//
// Clears the CPU trace buffer and resets the visible trace position.  If the
// view is frozen, the frozen snapshot is also cleared.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::ClearTrace()
{
	nes::cpu::debug_clear_cpu_trace();
	fFrozenEntries.clear();

	fBaseTraceIndex = 0;

	if (!fFreezeUpdates) {
		fFollowNewest = true;
	} else {
		fFollowNewest = false;
	}

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::FollowNewest
//
// Leaves frozen snapshot mode and resumes following the live CPU trace buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::FollowNewest()
{
	fFrozenEntries.clear();

	fFreezeUpdates = false;
	fFollowNewest = true;

	JumpToNewest();
}


// -----------------------------------------------------------------------------
// CPUTraceView::MessageReceived
//
// Handles mouse-wheel scrolling for the CPU trace viewer.  Since the scrollbar
// is not attached as a normal BView scrolling target, wheel events are converted
// into trace-row scrolling manually.
//
// Parameters:
//   message - Incoming BeAPI message.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY = 0.0f;

			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK) {
				break;
			}

			if (deltaY == 0.0f) {
				break;
			}

			int32 lines = static_cast<int32>(deltaY);

			if (lines == 0) {
				lines = deltaY > 0.0f ? 1 : -1;
			}

			if (!fFreezeUpdates) {
				CaptureSnapshot();
				fFreezeUpdates = true;
			}

			fFollowNewest = false;
			ScrollLines(lines);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


// -----------------------------------------------------------------------------
// CPUTraceView::MouseDown
//
// Gives keyboard focus to the CPU trace view and selects a trace row when the
// click lands inside the visible instruction table.  Clicking the selected row
// again clears the selection.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::MouseDown(BPoint where)
{
	MakeFocus(true);

	uint32 index = 0;

	if (!TraceIndexForPoint(where, index)) {
		return;
	}

	if (fHasSelectedTraceIndex && fSelectedTraceIndex == index) {
		fHasSelectedTraceIndex = false;
		fSelectedTraceIndex = 0;
	} else {
		fHasSelectedTraceIndex = true;
		fSelectedTraceIndex = index;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::Pulse
//
// Refreshes the CPU trace viewer.  In live follow mode, the view follows the
// newest entries.  In frozen mode, the view displays only the captured snapshot
// and does not allow the live CPU trace buffer to affect the visible range.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	if (!fFreezeUpdates && fFollowNewest) {
		const uint32 count = TraceDisplayCount();
		const int32 visibleRows = VisibleTraceRows();

		if (count > static_cast<uint32>(visibleRows)) {
			fBaseTraceIndex = count - static_cast<uint32>(visibleRows);
		} else {
			fBaseTraceIndex = 0;
		}

		UpdateScrollBar();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::FrameResized
//
// Updates the CPU trace viewer layout after the view size changes.  The
// scrollbar is repositioned, its range/value are refreshed, and the view is
// invalidated so the trace table redraws using the new bounds.
//
// Parameters:
//   width  - New view width.
//   height - New view height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::FrameResized (float width, float height)
{
	(void)width;
	(void)height;

	LayoutScrollBar();
	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::DrawHeaderPanel
//
// Draws the CPU trace header panel, including trace state, entry counts, and
// keyboard controls.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::DrawHeaderPanel()
{
	BRect panel = Bounds();
	panel.bottom = 72.0f;

	SetHighColor(235, 235, 235);
	FillRect(panel);

	SetHighColor(170, 170, 170);
	StrokeLine(BPoint(panel.left, panel.bottom), BPoint(panel.right, panel.bottom));

	BFont prevFont;
	GetFont(&prevFont);

	BFont normal;
	GetFont(&normal);
	normal.SetSize(11.0f);
	SetFont(&normal);

	BString s;
	const uint32 liveCount = nes::cpu::debug_cpu_trace_count();
	const uint32 displayCount = TraceDisplayCount();
	const uint32 capacity = nes::cpu::debug_cpu_trace_capacity();

	SetHighColor(0, 0, 0);

	if (fFreezeUpdates) {
		s.SetToFormat("CPU Trace: frozen snapshot   entries %lu / %lu",
						static_cast<unsigned long>(displayCount),
						static_cast<unsigned long>(capacity));
	} else {
		s.SetToFormat("CPU Trace: live   entries %lu / %lu",
						static_cast<unsigned long>(liveCount),
						static_cast<unsigned long>(capacity));
	}

	DrawString(s.String(), BPoint(panel.left + 10.0f, panel.top + 20.0f));

	float x = panel.left + 10.0f;
	const float y = panel.top + 44.0f;

	if (fFreezeUpdates) {
		SetHighColor(150, 80, 0);
		DrawString("Frozen", BPoint(x, y));
		x += normal.StringWidth("Frozen   ");
	} else {
		SetHighColor(0, 100, 0);
		DrawString("Live", BPoint(x, y));
		x += normal.StringWidth("Live   ");
	}

	if (fFollowNewest) {
		SetHighColor(0, 100, 0);
		DrawString("Follow newest", BPoint(x, y));
		x += normal.StringWidth("Follow newest   ");
	} else {
		SetHighColor(110, 110, 110);
		DrawString("Manual", BPoint(x, y));
		x += normal.StringWidth("Manual   ");
	}

	if (fFreezeUpdates) {
		SetHighColor(90, 90, 90);
		s.SetToFormat("live buffer %lu", static_cast<unsigned long>(liveCount));
		DrawString(s.String(), BPoint(x, y));
	}

	SetHighColor(70, 70, 70);
	DrawString("Space Freeze/Live   C Clear   End Newest   D/Enter Disassmebly   "
				"Arrows/Page Up/Page Down/Wheel Scroll", BPoint(panel.left + 10.0f, panel.top + 64.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUTraceView::DrawTracePanel
//
// Draws the CPU execution trace table.  Live mode reads from the CPU trace
// buffer.  Frozen mode reads only from fFrozenEntries so the displayed backtrace
// cannot change while the emulator continues running.  A selected-row inspector
// is drawn at the bottom of the panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::DrawTracePanel()
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
		? fScrollBar->Frame().left - 4.0f
		: Bounds().right - 4.0f;

	BRect panel(4.0f, 80.0f, rightEdge, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Executed Instructions");

	if (!HasROMLoaded()) {
		DrawNoROMMessage(panel);
		return;
	}

	const float inspectorHeight = 82.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;
	const float tableBottom = inspectorTop - 8.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float cycleX = panel.left + 8.0f;
	const float indexX = cycleX + 92.0f;
	const float pcX = indexX + 56.0f;
	const float bytesX = pcX + 58.0f;
	const float instrX = bytesX + 92.0f;
	const float regX = instrX + 172.0f;

	float y = panel.top + 42.0f;

	SetHighColor(80, 80, 80);
	DrawString("Cycle", BPoint(cycleX, y));
	DrawString("#", BPoint(indexX, y));
	DrawString("PC", BPoint(pcX, y));
	DrawString("Bytes", BPoint(bytesX, y));
	DrawString("Instruction", BPoint(instrX, y));
	DrawString("A  X  Y  S  P", BPoint(regX, y));

	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));

	y += 4.0f;

	const uint32 count = TraceDisplayCount();
	int32 visibleRows = static_cast<int32>((tableBottom - y) / lineH);

	if (visibleRows < 1) {
		visibleRows = 1;
	}

	if (count == 0) {
		SetHighColor(90, 90, 90);

		if (fFreezeUpdates) {
			DrawString("Frozen snapshot is empty.", BPoint(panel.left + 12.0f, y + lineH));
		} else {
			DrawString("No CPU trace entries yet.", BPoint(panel.left + 12.0f, y + lineH));
		}

		SetFont(&prevFont);

		SetHighColor(170, 170, 170);
		StrokeLine(BPoint(panel.left + 8.0f, inspectorTop - 4.0f), BPoint(panel.right - 8.0f, inspectorTop - 4.0f));
		DrawSelectedTraceInfo(panel);
		return;
	}

	if (fBaseTraceIndex >= count) {
		fBaseTraceIndex = count - 1;
	}

	BString s;

	for (int32 row = 0; row < visibleRows; row++) {
		const uint32 traceIndex = fBaseTraceIndex + static_cast<uint32>(row);

		if (traceIndex >= count) {
			break;
		}

		nes::cpu::cpu_trace_entry_t entry;

		if (!TraceDisplayEntry(traceIndex, entry)) {
			break;
		}

		uint8 instructionLength = entry.length;

		if (!fFreezeUpdates) {
			CPUDisasmLine line = DisassembleCPU(entry.pc);

			if (line.length == 0 || line.length > 3) {
				instructionLength = 1;
			} else {
				instructionLength = line.length;
			}
		}

		if (instructionLength == 0 || instructionLength > 3) {
			instructionLength = 1;
		}

		BString instr;

		if (!TraceDisplayInstruction(traceIndex, instr)) {
			instr.SetTo("?");
		}

		const bool newest = traceIndex + 1 == count;
		const bool selected = fHasSelectedTraceIndex && (fSelectedTraceIndex == traceIndex);
		BRect rowRect(panel.left + 8.0f, y - 11.0f, panel.right - 8.0f, y + 3.0f);

		if (selected) {
			SetHighColor(190, 215, 245);
			FillRect(rowRect);

			SetHighColor(70, 120, 180);
			StrokeRect(rowRect);
		} else if (newest) {
			SetHighColor(255, 245, 170);
			FillRect(rowRect);
		}

		SetHighColor(80, 80, 80);
		s.SetToFormat("%llu", static_cast<unsigned long long>(entry.cycle));
		DrawString(s.String(), BPoint(cycleX, y));

		s.SetToFormat("%lu", static_cast<unsigned long>(traceIndex));
		DrawString(s.String(), BPoint(indexX, y));

		SetHighColor(0, 0, 0);
		s.SetToFormat("$%04X", entry.pc);
		DrawString(s.String(), BPoint(pcX, y));

		SetHighColor(80, 80, 80);

		BString bytes;

		for (uint8 i = 0; i < instructionLength && i < 3; i++) {
			if (i > 0) {
				bytes << " ";
			}

			s.SetToFormat("%02X", entry.bytes[i]);
			bytes << s;
		}

		DrawString(bytes.String(), BPoint(bytesX, y));

		SetHighColor(0, 0, 0);
		DrawString(instr.String(), BPoint(instrX, y));

		SetHighColor(50, 50, 50);
		s.SetToFormat("%02X %02X %02X %02X %02X",
			entry.a,
			entry.x,
			entry.y,
			entry.s,
			entry.p);
		DrawString(s.String(), BPoint(regX, y));

		y += lineH;
	}

	SetFont(&prevFont);

	SetHighColor(170, 170, 170);
	StrokeLine(BPoint(panel.left + 8.0f, inspectorTop - 4.0f), BPoint(panel.right - 8.0f, inspectorTop - 4.0f));

	DrawSelectedTraceInfo(panel);
}


// -----------------------------------------------------------------------------
// CPUTraceView::DrawSelectedTraceInfo
//
// Draws the selected trace row inspector at the bottom of the trace panel.
//
// Parameters:
//   panel - Main trace panel rectangle.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::DrawSelectedTraceInfo (BRect panel)
{
	const float inspectorHeight = 82.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;

	BFont prevFont;
	GetFont(&prevFont);

	BFont normal;
	GetFont(&normal);
	normal.SetSize(11.0f);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);

	auto drawNormal = [&](const char *text, float x, float y, rgb_color color) {
		SetFont(&normal);
		SetHighColor(color);
		DrawString(text, BPoint(x, y));
	};

	auto drawFixed = [&](const char *text, float x, float y, rgb_color color) {
		SetFont(&fixed);
		SetHighColor(color);
		DrawString(text, BPoint(x, y));
	};

	const float x = panel.left + 10.0f;
	float y = inspectorTop + 14.0f;

	drawNormal("Selected Trace", x, y, rgb_color{0, 0, 0, 255});

	nes::cpu::cpu_trace_entry_t entry;
	BString instruction;

	if (!SelectedTraceEntry(entry, instruction)) {
		drawNormal("Click a trace row to inspect it.", x, y + 18.0f, rgb_color{90, 90, 90, 255});

		SetFont(&prevFont);
		return;
	}

	BString s;

	const float labelX = x;
	const float valueX = x + 92.0f;
	const float col2X = x + 270.0f;
	const float col2ValueX = col2X + 78.0f;
	const float col3X = x + 470.0f;
	const float col3ValueX = col3X + 46.0f;

	y += 18.0f;

	drawNormal("Index:", labelX, y, rgb_color{0, 0, 0, 255});
	s.SetToFormat("%lu", static_cast<unsigned long>(fSelectedTraceIndex));
	drawFixed(s.String(), valueX, y, rgb_color{0, 0, 0, 255});

	drawNormal("PC:", col2X, y, rgb_color{0, 0, 0, 255});
	s.SetToFormat("$%04X", entry.pc);
	drawFixed(s.String(), col2ValueX, y, rgb_color{0, 0, 0, 255});

	drawNormal("A:", col3X, y, rgb_color{0, 0, 0, 255});
	s.SetToFormat("$%02X", entry.a);
	drawFixed(s.String(), col3ValueX, y, rgb_color{0, 0, 0, 255});

	y += 16.0f;

	drawNormal("Cycle:", labelX, y, rgb_color{0, 0, 0, 255});
	s.SetToFormat("%llu", static_cast<unsigned long long>(entry.cycle));
	drawFixed(s.String(), valueX, y, rgb_color{0, 0, 0, 255});
	drawNormal("Bytes:", col2X, y, rgb_color{0, 0, 0, 255});

	BString bytes;
	uint8 length = entry.length;

	if (length == 0 || length > 3) {
		length = 1;
	}

	for (uint8 i = 0; i < length; i++) {
		if (i > 0) {
			bytes << " ";
		}

		s.SetToFormat("%02X", entry.bytes[i]);
		bytes << s;
	}

	drawFixed(bytes.String(), col2ValueX, y, rgb_color{0, 0, 0, 255});
	drawNormal("X:", col3X, y, rgb_color{0, 0, 0, 255});
	
	s.SetToFormat("$%02X", entry.x);
	drawFixed(s.String(), col3ValueX, y, rgb_color{0, 0, 0, 255});
	drawNormal("Y:", col3X + 76.0f, y, rgb_color{0, 0, 0, 255});
	
	s.SetToFormat("$%02X", entry.y);
	drawFixed(s.String(), col3X + 102.0f, y, rgb_color{0, 0, 0, 255});

	y += 16.0f;

	drawNormal("Instruction:", labelX, y, rgb_color{0, 0, 0, 255});
	drawFixed(instruction.String(), valueX, y, rgb_color{0, 0, 0, 255});
	drawNormal("S:", col3X, y, rgb_color{0, 0, 0, 255});
	
	s.SetToFormat("$%02X", entry.s);
	drawFixed(s.String(), col3ValueX, y, rgb_color{0, 0, 0, 255});
	drawNormal("P:", col3X + 76.0f, y, rgb_color{0, 0, 0, 255});
	
	s.SetToFormat("$%02X", entry.p);
	drawFixed(s.String(), col3X + 102.0f, y, rgb_color{0, 0, 0, 255});

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUTraceView::DrawNoROMMessage
//
// Draws the no-ROM message in a trace panel.
//
// Parameters:
//   panel - Panel rectangle.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::DrawNoROMMessage (BRect panel)
{
	SetHighColor(80, 80, 80);
	DrawString("Load a ROM to view CPU execution trace.", BPoint(panel.left + 12.0f, panel.top + 40.0f));
}


// -----------------------------------------------------------------------------
// CPUTraceView::HasROMLoaded
//
// Checks whether a ROM mapper is available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded.
// -----------------------------------------------------------------------------
bool
CPUTraceView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// CPUTraceView::ScrollLines
//
// Scrolls the trace view by a number of trace rows.
//
// Parameters:
//   lines - Signed trace row count to scroll.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::ScrollLines (int32 lines)
{
	const uint32 count = TraceDisplayCount();
	const int32 visibleRows = VisibleTraceRows();

	int32 maxBase = static_cast<int32>(count) - visibleRows;

	if (maxBase < 0) {
		maxBase = 0;
	}

	int32 base = static_cast<int32>(fBaseTraceIndex);
	base += lines;

	if (base < 0) {
		base = 0;
	}

	if (base > maxBase) {
		base = maxBase;
	}

	fBaseTraceIndex = static_cast<uint32>(base);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::TraceDisplayCount
//
// Returns the number of trace entries currently displayed.  Frozen mode uses a
// local snapshot so the circular CPU trace buffer can continue recording without
// changing the frozen view.
//
// Parameters:
//   None.
//
// Returns:
//   Number of displayable trace entries.
// -----------------------------------------------------------------------------
uint32
CPUTraceView::TraceDisplayCount() const
{
	if (fFreezeUpdates) {
		return static_cast<uint32>(fFrozenEntries.size());
	}

	return nes::cpu::debug_cpu_trace_count();
}


// -----------------------------------------------------------------------------
// CPUTraceView::TraceDisplayEntry
//
// Reads one trace entry from either the frozen snapshot or the live CPU trace
// buffer.
//
// Parameters:
//   index - Chronological trace index.
//   entry - Receives the trace entry.
//
// Returns:
//   true if an entry was available.
// -----------------------------------------------------------------------------
bool
CPUTraceView::TraceDisplayEntry (uint32 index, nes::cpu::cpu_trace_entry_t& entry) const
{
	if (fFreezeUpdates) {
		if (index >= fFrozenEntries.size()) {
			return false;
		}

		entry = fFrozenEntries[index].trace;
		return true;
	}

	return nes::cpu::debug_cpu_trace_entry(index, entry);
}


// -----------------------------------------------------------------------------
// CPUTraceView::SelectedTraceEntry
//
// Retrieves the currently selected trace entry and stable instruction text.
//
// Parameters:
//   entry       - Receives the selected trace entry.
//   instruction - Receives the selected instruction text.
//
// Returns:
//   true if a selected trace entry is available.
// -----------------------------------------------------------------------------
bool
CPUTraceView::SelectedTraceEntry(nes::cpu::cpu_trace_entry_t &entry, BString &instruction) const
{
	instruction.SetTo("");

	if (!fHasSelectedTraceIndex) {
		return false;
	}

	if (!TraceDisplayEntry(fSelectedTraceIndex, entry)) {
		return false;
	}

	if (!TraceDisplayInstruction(fSelectedTraceIndex, instruction)) {
		instruction.SetTo("?");
	}

	return true;
}


// -----------------------------------------------------------------------------
// CPUTraceView::SelectedTraceAddress
//
// Retrieves the CPU address for the currently selected trace row.
//
// Parameters:
//   address - Receives the selected trace row PC.
//
// Returns:
//   true if a selected trace row is available.
// -----------------------------------------------------------------------------
bool
CPUTraceView::SelectedTraceAddress (uint16 &address) const
{
	nes::cpu::cpu_trace_entry_t entry;
	BString instruction;

	if (!SelectedTraceEntry(entry, instruction)) {
		return false;
	}

	address = entry.pc;
	return true;
}


// -----------------------------------------------------------------------------
// CPUTraceView::TraceDisplayInstruction
//
// Reads stable instruction text for a displayed trace row.  Frozen mode returns
// the instruction text captured in the snapshot.  Live mode decodes current CPU
// memory at the trace PC.
//
// Parameters:
//   index       - Chronological trace index.
//   instruction - Receives the display instruction text.
//
// Returns:
//   true if instruction text was available.
// -----------------------------------------------------------------------------
bool
CPUTraceView::TraceDisplayInstruction(uint32 index, BString& instruction) const
{
	instruction.SetTo("");

	if (fFreezeUpdates) {
		if (index >= fFrozenEntries.size()) {
			return false;
		}

		instruction.SetTo(fFrozenEntries[index].instruction);
		return true;
	}

	nes::cpu::cpu_trace_entry_t entry;

	if (!nes::cpu::debug_cpu_trace_entry(index, entry)) {
		return false;
	}

	CPUDisasmLine line = DisassembleCPU(entry.pc);

	if (line.operand.Length() > 0) {
		instruction.SetToFormat("%s %s", line.mnemonic.String(), line.operand.String());
	} else {
		instruction.SetTo(line.mnemonic);
	}

	return true;
}

// -----------------------------------------------------------------------------
// CPUTraceView::TraceIndexForPoint
//
// Converts a point in view coordinates to a visible trace index.
//
// Parameters:
//   where - Point in view coordinates.
//   index - Receives the trace index if the point is over a visible row.
//
// Returns:
//   true if a visible trace row was hit.
// -----------------------------------------------------------------------------
bool
CPUTraceView::TraceIndexForPoint(BPoint where, uint32 &index) const
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
		? fScrollBar->Frame().left - 4.0f
		: Bounds().right - 4.0f;

	BRect panel(4.0f, 80.0f, rightEdge, Bounds().bottom - 8.0f);
	
	if (!panel.Contains(where)) {
		return false;
	}

	const float inspectorHeight = 82.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;
	const float tableBottom = inspectorTop - 8.0f;

	if (where.y >= inspectorTop) {
		return false;
	}

	BFont prevFont;
	const_cast<CPUTraceView*>(this)->GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	const_cast<CPUTraceView*>(this)->SetFont(&mono);

	font_height fh;
	const_cast<CPUTraceView*>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const_cast<CPUTraceView*>(this)->SetFont(&prevFont);

	float y = panel.top + 42.0f;
	y += lineH + 8.0f;
	y += 4.0f;

	int32 visibleRows = static_cast<int32>((tableBottom - y) / lineH);

	if (visibleRows < 1) {
		visibleRows = 1;
	}

	const uint32 count = TraceDisplayCount();

	for (int32 row = 0; row < visibleRows; row++) {
		const uint32 traceIndex = fBaseTraceIndex + static_cast<uint32>(row);

		if (traceIndex >= count) {
			break;
		}

		const float rowTop = y - 11.0f;
		const float rowBottom = y + 3.0f;

		if (where.y >= rowTop && (where.y <= rowBottom)) {
			index = traceIndex;
			return true;
		}

		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// CPUTraceView::CaptureSnapshot
//
// Copies the current CPU trace buffer into a local frozen snapshot.  Instruction
// length and decoded instruction text are resolved during the snapshot so frozen
// display does not depend on live memory changing later.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::CaptureSnapshot()
{
	fFrozenEntries.clear();

	const uint32 count = nes::cpu::debug_cpu_trace_count();
	fFrozenEntries.reserve(count);

	for (uint32 i = 0; i < count; i++) {
		nes::cpu::cpu_trace_entry_t entry;

		if (!nes::cpu::debug_cpu_trace_entry(i, entry)) {
			continue;
		}

		CPUDisasmLine line = DisassembleCPU(entry.pc);

		if (line.length == 0 || line.length > 3) {
			entry.length = 1;
		} else {
			entry.length = line.length;
		}

		CPUTraceFrozenEntry frozenEntry;
		frozenEntry.trace = entry;

		if (line.operand.Length() > 0) {
			frozenEntry.instruction.SetToFormat("%s %s", line.mnemonic.String(), line.operand.String());
		} else {
			frozenEntry.instruction.SetTo(line.mnemonic);
		}

		fFrozenEntries.push_back(frozenEntry);
	}
}


// -----------------------------------------------------------------------------
// CPUTraceView::JumpToNewest
//
// Moves the trace view to the newest entries and enables follow-newest mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::JumpToNewest()
{
	const uint32 count = TraceDisplayCount();
	const int32 visibleRows = VisibleTraceRows();

	if (count > static_cast<uint32>(visibleRows)) {
		fBaseTraceIndex = count - static_cast<uint32>(visibleRows);
	} else {
		fBaseTraceIndex = 0;
	}

	fFollowNewest = true;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::LayoutScrollBar
//
// Positions the vertical scrollbar along the right side of the CPU trace view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::LayoutScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const float scrollBarWidth = B_V_SCROLL_BAR_WIDTH;
	BRect frame(Bounds().right - scrollBarWidth, 80.0f, Bounds().right, Bounds().bottom - 8.0f);

	fScrollBar->MoveTo(frame.LeftTop());
	fScrollBar->ResizeTo(frame.Width(), frame.Height());
	fScrollBar->Show();
}


// -----------------------------------------------------------------------------
// CPUTraceView::UpdateScrollBar
//
// Updates the CPU trace scrollbar range and value.  Frozen mode uses the local
// snapshot count, not the live CPU trace count.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const uint32 count = TraceDisplayCount();
	const int32 visibleRows = VisibleTraceRows();

	int32 maxBase = static_cast<int32>(count) - visibleRows;

	if (maxBase < 0) {
		maxBase = 0;
	}

	if (fBaseTraceIndex > static_cast<uint32>(maxBase)) {
		fBaseTraceIndex = static_cast<uint32>(maxBase);
	}

	fUpdatingScrollBar = true;

	fScrollBar->SetRange(0.0f, static_cast<float>(maxBase));
	fScrollBar->SetSteps(1.0f, static_cast<float>(visibleRows));
	fScrollBar->SetProportion(count > 0
							? static_cast<float>(visibleRows) / static_cast<float>(count) : 1.0f);
	fScrollBar->SetValue(static_cast<float>(fBaseTraceIndex));

	fUpdatingScrollBar = false;
}


// -----------------------------------------------------------------------------
// CPUTraceView::ScrollBarChanged
//
// Handles user scrollbar movement.  The scrollbar value is converted into a
// display trace index.  Frozen mode scrolls the local snapshot; live mode first
// captures a snapshot, then scrolls that stable snapshot.
//
// Parameters:
//   value - New scrollbar value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceView::ScrollBarChanged(float value)
{
	if (fUpdatingScrollBar) {
		return;
	}

	if (!fFreezeUpdates) {
		CaptureSnapshot();

		fFreezeUpdates = true;
		fFollowNewest = false;
	}

	const uint32 count = TraceDisplayCount();
	const int32 visibleRows = VisibleTraceRows();

	int32 maxBase = static_cast<int32>(count) - visibleRows;

	if (maxBase < 0) {
		maxBase = 0;
	}

	int32 base = static_cast<int32>(value + 0.5f);

	if (base < 0) {
		base = 0;
	}

	if (base > maxBase) {
		base = maxBase;
	}

	fBaseTraceIndex = static_cast<uint32>(base);
	fFollowNewest = false;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUTraceView::VisibleTraceRows
//
// Calculates how many trace rows fit in the visible trace table.  The selected
// trace inspector at the bottom of the panel is excluded so follow-newest,
// scrolling, hit-testing, and drawing all agree on the same row count.
//
// Parameters:
//   None.
//
// Returns:
//   Number of visible trace rows.
// -----------------------------------------------------------------------------
int32
CPUTraceView::VisibleTraceRows() const
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
		? fScrollBar->Frame().left - 4.0f
		: Bounds().right - 4.0f;

	BRect panel(4.0f, 80.0f, rightEdge, Bounds().bottom - 8.0f);
	const float inspectorHeight = 82.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;
	const float tableBottom = inspectorTop - 8.0f;

	BFont prevFont;
	const_cast<CPUTraceView *>(this)->GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	const_cast<CPUTraceView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<CPUTraceView *>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const_cast<CPUTraceView *>(this)->SetFont(&prevFont);

	float y = panel.top + 42.0f;
	y += lineH + 8.0f;
	y += 4.0f;

	const int32 rows = static_cast<int32>((tableBottom - y) / lineH);

	if (rows < 1) {
		return 1;
	}

	return rows;
}

