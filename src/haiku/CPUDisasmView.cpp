
#include "CPUDisasmView.h"


class CPUDisasmScrollBar : public BScrollBar
{
	public:
	// -------------------------------------------------------------------------
	// CPUDisasmScrollBar::CPUDisasmScrollBar
	//
	// Constructs the vertical scrollbar used by the CPU disassembly view.
	//
	// The scrollbar covers the CPU address range from $8000 through $FFFF and
	// forwards position changes to its owning CPUDisasmView.
	//
	// Parameters:
	//   frame - Scrollbar frame rectangle.
	//   owner - CPU disassembly view that receives scrollbar changes.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	CPUDisasmScrollBar (BRect frame, CPUDisasmView *owner)
		: BScrollBar(frame, "cpu_disasm_scrollbar", owner, 0x8000, 0xffff, B_VERTICAL)
	{
		fOwner = owner;

		SetSteps(1.0f, 256.0f);
	}

	public:
	// -------------------------------------------------------------------------
	// CPUDisasmScrollBar::ValueChanged
	//
	// Handles a scrollbar position change and forwards the new value to the
	// owning CPU disassembly view.
	//
	// Parameters:
	//   value - New scrollbar value.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	virtual void ValueChanged (float value)
	{
		if (fOwner) {
			fOwner->ScrollBarChanged(value);
		}
	}

	private:
	CPUDisasmView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// CPUDisasmView::CPUDisasmView
//
// Constructs the CPU disassembly view and associates it with the parent
// Pretendo window.
//
// The view is configured for transparent drawing, pulse notifications, and
// frame-resize events.
//
// Parameters:
//   frame  - Initial view frame rectangle.
//   parent - Parent Pretendo window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUDisasmView::CPUDisasmView (BRect frame, PretendoWindow *parent)
	: BView(frame, "cpu_disasm_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::~CPUDisasmView
//
// Destroys the CPU disassembly view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUDisasmView::~CPUDisasmView()
{
}


// -----------------------------------------------------------------------------
// CPUDisasmView::AttachedToWindow
//
// Creates the scrollbar after the disassembly view is attached to its window,
// lays out the view, and initializes the disassembly position.  If a ROM is
// already loaded, ResetView() jumps to the reset-vector target so opening the
// disassembler after loading a ROM starts in the expected code area.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (!fScrollBar) {
		BRect scrollFrame(Bounds().right - B_V_SCROLL_BAR_WIDTH, 124.0f, Bounds().right, Bounds().bottom - 8.0f);

		fScrollBar = new CPUDisasmScrollBar(scrollFrame, this);
		AddChild(fScrollBar);
	}

	LayoutScrollBar();

	MakeFocus(true);
	
	ResetView();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::FrameResized
//
// Handles changes to the CPU disassembly view dimensions.
//
// The vertical scrollbar is repositioned to match the new view bounds before
// the resize event is passed to the base BView implementation.
//
// Parameters:
//   width  - New view width.
//   height - New view height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::FrameResized (float width, float height)
{
	(void)width;
	(void)height;

	LayoutScrollBar();

	BView::FrameResized(width, height);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::Pulse
//
// Handles periodic view updates from the Haiku pulse mechanism.
//
// No work is performed until a ROM is loaded.  While live updates are enabled,
// the view is invalidated so the disassembly and debugger state are refreshed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	if (!fFreezeUpdates) {
		Invalidate();
	}
}


// -----------------------------------------------------------------------------
// CPUDisasmView::KeyDown
//
// Handles CPU disassembly viewer controls.
//
// Space freezes or unfreezes the disassembly view.  Freezing captures the CPU
// registers and complete debugger-visible CPU memory snapshot so mapper changes
// and subsequent CPU execution cannot alter the frozen disassembly.
//
// S enters debugger step mode and advances one CPU instruction.
// V advances one full video frame while debugger-paused.
// G resumes normal emulator execution.
// F toggles follow-PC mode.
// P returns the view to the current PC and clears row selection.
// B toggles an execute breakpoint at the selected row, or at the current/frozen
// PC if no row is selected.
// C clears execute BreakPoints.
// R jumps to the RESET vector.
// N jumps to the NMI vector.
// I jumps to the IRQ vector.
// Enter centers the view around the selected row.
// Esc clears the selected row.
// Arrow/Page keys scroll through disassembly.
//
// Parameters:
//   bytes    - Key bytes.
//   numBytes - Number of key bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		if (bytes[0] == ' ') {
			fFreezeUpdates = !fFreezeUpdates;

			if (!fFreezeUpdates) {
				fHaveFrozenSnapshot = false;
			}

			Invalidate();
			return;
		}

		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case ' ':
		{
			if (!fFreezeUpdates) {
				CaptureFrozenSnapshot();

				fFreezeUpdates = true;

				if (fFollowPC) {
					fBaseAddress = FindContextBase(fFrozenPC, 5);
					UpdateScrollBar();
				}
			} else {
				fFreezeUpdates = false;
				fHaveFrozenSnapshot = false;

				if (fFollowPC) {
					nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
					fBaseAddress = FindContextBase(state.pc, 5);

					UpdateScrollBar();
				}
			}

			Invalidate();
			break;
		}

		case B_ESCAPE:
			fHasSelectedAddress = false;
			fSelectedAddress = 0x0000;

			Invalidate();
			break;

		case B_ENTER:
			if (fHasSelectedAddress) {
				const int32 rows = VisibleDisasmRows();
				const int32 linesBefore = rows / 2;

				fFollowPC = false;
				fBaseAddress = FindContextBase(fSelectedAddress, linesBefore);

				UpdateScrollBar();
				Invalidate();
			}
			break;

		case 's':
		case 'S':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;

			if (nes::cpu::debug_breakpoint_hit()) {
				nes::cpu::debug_skip_breakpoint_once();
				nes::cpu::debug_clear_breakpoint_hit();
			}

			if (fParent) {
				fParent->DebugStepInstruction();
			}

			JumpToCurrentPC();
			break;

		case 'v':
		case 'V':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;

			if (nes::cpu::debug_breakpoint_hit()) {
				nes::cpu::debug_skip_breakpoint_once();
				nes::cpu::debug_clear_breakpoint_hit();
			}

			if (fParent) {
				fParent->DebugStepFrame();
			}

			JumpToCurrentPC();
			break;

		case 'g':
		case 'G':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;

			if (fParent) {
				fParent->DebugResumeExecution();
			}

			JumpToCurrentPC();
			break;

		case 'f':
		case 'F':
			SetFollowPC(!fFollowPC);
			break;

		case 'p':
		case 'P':
			fHasSelectedAddress = false;
			fSelectedAddress = 0x0000;

			if (fFreezeUpdates) {
				fFollowPC = true;
				fBaseAddress = FindContextBase(fFrozenPC, 5);

				UpdateScrollBar();
			} else {
				JumpToCurrentPC();
			}

			Invalidate();
			break;

		case 'b':
		case 'B':
		{
			uint16 address = 0x0000;

			if (fHasSelectedAddress) {
				address = fSelectedAddress;
			} else {
				nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
				address = fFreezeUpdates ? fFrozenPC : state.pc;
			}

			if (nes::cpu::debug_has_execute_breakpoint(address)) {
				nes::cpu::debug_remove_execute_breakpoint(address);
			} else {
				nes::cpu::debug_add_execute_breakpoint(address);
			}

			nes::cpu::debug_clear_breakpoint_hit();

			Invalidate();
			break;
		}

		case 'c':
		case 'C':
			nes::cpu::debug_clear_execute_breakpoints();
			nes::cpu::debug_clear_breakpoint_hit();

			Invalidate();
			break;

		case 'r':
		case 'R':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;
			fFollowPC = false;

			JumpToVector(0xfffc);
			break;

		case 'n':
		case 'N':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;
			fFollowPC = false;

			JumpToVector(0xfffa);
			break;

		case 'i':
		case 'I':
			fFreezeUpdates = false;
			fHaveFrozenSnapshot = false;
			fFollowPC = false;

			JumpToVector(0xfffe);
			break;

		case B_UP_ARROW:
			ScrollLines(-1);
			break;

		case B_DOWN_ARROW:
			ScrollLines(1);
			break;

		case B_PAGE_UP:
			ScrollLines(-12);
			break;

		case B_PAGE_DOWN:
			ScrollLines(12);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// CPUDisasmView::MouseDown
//
// Selects the disassembly row under the mouse.  Selecting a row switches the
// view to manual mode so the selected row does not immediately scroll away while
// follow-PC mode updates.
//
// The selected row becomes the preferred target for breakpoint toggling.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::MouseDown (BPoint where)
{
	MakeFocus(true);

	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		return;
	}

	fHasSelectedAddress = true;
	fSelectedAddress = address;

	fFollowPC = false;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::Draw
//
// Draws the CPU disassembly viewer.  If no ROM is loaded, the scrollbar is
// hidden and the disassembly panel shows the friendly empty-state message.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	const bool hasROM = HasROMLoaded();

	if (fScrollBar) {
		if (hasROM && fScrollBar->IsHidden()) {
			fScrollBar->Show();
			LayoutScrollBar();
			UpdateScrollBar();
		} else if (!hasROM && !fScrollBar->IsHidden()) {
			fScrollBar->Hide();
		}
	}

	DrawHeaderPanel();
	DrawDisasmPanel();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::ResetView
//
// Resets the disassembly view state after a ROM load or emulator reset.
//
// When a ROM is loaded, the disassembler opens at the target of the 6502 RESET
// vector at $FFFC/$FFFD.  The view starts in manual mode so the reset entry
// point remains visible even if the CPU has not yet begun execution or its live
// PC currently points somewhere else.
//
// Pressing P or enabling Follow-PC later reconnects the disassembly view to the
// live CPU program counter.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::ResetView()
{
	fFreezeUpdates = false;
	fFollowPC = false;
	fFrozenPC = 0x0000;

	fHasSelectedAddress = false;
	fSelectedAddress = 0x0000;

	if (HasROMLoaded()) {
		JumpToVector(0xfffc);
		return;
	}

	fBaseAddress = 0x0000;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawHeaderPanel
//
// Draws the CPU disassembly header panel.
//
// The header shows the current display PC, debugger-hit status, accumulated hit
// count for the condition responsible for the current stop, running/frozen/
// follow state, selected row state, register summary, current instruction, and
// debugger controls.
//
// In frozen mode, the displayed PC, CPU registers, processor status, and current
// instruction all come from the captured disassembly snapshot.  This keeps the
// header consistent with the frozen instruction listing while execution
// continues in the emulator.
//
// Execute BreakPoints, READ watchpoints, and WRITE watchpoints maintain separate
// hit counters.  The current debugger break reason therefore determines which
// counter is displayed.
//
// For memory watchpoints, debug_breakpoint_hit_address() identifies the CPU
// instruction responsible for the stop, while debug_memory_break_address()
// identifies the watched memory address whose READ/WRITE hit counter must be
// queried.
//
// The instruction color legend is intentionally not drawn here.  It belongs in
// the instruction panel below the header so the top area stays readable.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawHeaderPanel()
{
	BRect panel = Bounds();
	panel.bottom = 116.0f;

	SetHighColor(235, 235, 235);
	FillRect(panel);

	SetHighColor(170, 170, 170);
	StrokeLine(BPoint(panel.left, panel.bottom), BPoint(panel.right, panel.bottom));

	if (!HasROMLoaded()) {
		SetHighColor(80, 80, 80);
		DrawString("CPU Disassembly: no ROM loaded", BPoint(panel.left + 10.0f, panel.top + 20.0f));

		return;
	}

	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
	uint16 displayPC = state.pc;
	uint8 displayA = state.a;
	uint8 displayX = state.x;
	uint8 displayY = state.y;
	uint8 displayS = state.s;
	uint8 displayP = state.p;

	if (nes::cpu::debug_breakpoint_hit()) {
		displayPC = nes::cpu::debug_breakpoint_hit_address();
	}

	/*
	 * Frozen mode uses the complete CPU state captured with the disassembly
	 * snapshot rather than mixing frozen instructions with live registers.
	 */
	if (fFreezeUpdates && fHaveFrozenSnapshot) {
		displayPC = fFrozenPC;
		displayA = fFrozenA;
		displayX = fFrozenX;
		displayY = fFrozenY;
		displayS = fFrozenS;
		displayP = fFrozenP;
	}

	const bool breakpointHit = nes::cpu::debug_breakpoint_hit();
	const uint16 breakpointHitAddress = nes::cpu::debug_breakpoint_hit_address();

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	BFont normal;
	GetFont(&normal);
	normal.SetSize(11.0f);

	auto drawText = [&](const char *text, float &x, float y, rgb_color color) {
		SetFont(&normal);
		SetHighColor(color);
		DrawString(text, BPoint(x, y));

		x += normal.StringWidth(text);
	};

	auto drawFixed = [&](const char *text, float &x, float y, rgb_color color) {
		SetFont(&fixed);
		SetHighColor(color);
		DrawString(text, BPoint(x, y));

		x += fixed.StringWidth(text);
	};

	BString value;

	float x = panel.left + 10.0f;
	float y = panel.top + 20.0f;

	drawText("PC: ", x, y, rgb_color{35, 35, 35, 255});

	value.SetToFormat("$%04X", displayPC);

	drawFixed(value.String(), x, y, rgb_color{0, 0, 0, 255});

	if (breakpointHit) {
		uint32 hitCount = 0;
		const auto breakReason = nes::cpu::debug_break_reason();

		/*
		 * Execute BreakPoints count hits by instruction address.
		 *
		 * Memory watchpoints count hits by the READ/WRITE memory address,
		 * not by the instruction address responsible for the access.
		 */
		switch (breakReason) {
			case nes::cpu::DEBUG_BREAK_EXECUTE:
				hitCount = nes::cpu::debug_breakpoint_hit_count(breakpointHitAddress);
				break;

			case nes::cpu::DEBUG_BREAK_MEMORY_READ:
				hitCount = nes::cpu::debug_read_watchpoint_hit_count(nes::cpu::debug_memory_break_address());
				break;

			case nes::cpu::DEBUG_BREAK_MEMORY_WRITE:
				hitCount = nes::cpu::debug_write_watchpoint_hit_count(nes::cpu::debug_memory_break_address());
				break;

			case nes::cpu::DEBUG_BREAK_STACK_SP:
			case nes::cpu::DEBUG_BREAK_STACK_WRAP:
			case nes::cpu::DEBUG_BREAK_NONE:
			default:
				break;
		}

		drawText("   BREAK HIT: ", x, y, rgb_color{170, 0, 0, 255});

		value.SetToFormat("$%04X", breakpointHitAddress);
		drawFixed(value.String(), x, y, rgb_color{170, 0, 0, 255});

		/*
		 * Execute BreakPoints and memory watchpoints have accumulated
		 * hit counters.  Stack conditions currently do not.
		 */
		if (breakReason == nes::cpu::DEBUG_BREAK_EXECUTE
			|| breakReason == nes::cpu::DEBUG_BREAK_MEMORY_READ
			|| breakReason == nes::cpu::DEBUG_BREAK_MEMORY_WRITE) {

			drawText("  hits: ", x, y, rgb_color{170, 0, 0, 255});

			value.SetToFormat("%lu", static_cast<unsigned long>(hitCount));
			drawFixed(value.String(), x, y, rgb_color{170, 0, 0, 255});
		}
	}

	drawText("   ", x, y, rgb_color{35, 35, 35, 255});

	if (fFreezeUpdates) {
		drawText("Frozen", x, y, rgb_color{150, 80, 0, 255});
	} else {
		drawText("Live", x, y, rgb_color{0, 100, 0, 255});
	}

	drawText("   ", x, y, rgb_color{35, 35, 35, 255});

	if (fFollowPC) {
		drawText("Follow PC", x, y, rgb_color{0, 100, 0, 255});
	} else {
		drawText("Manual", x, y, rgb_color{110, 110, 110, 255});
	}

	drawText("   Selected: ", x, y, rgb_color{35, 35, 35, 255});

	if (fHasSelectedAddress) {
		value.SetToFormat("$%04X", fSelectedAddress);
		drawFixed(value.String(), x, y, rgb_color{35, 90, 180, 255});
	} else {
		drawText("none", x, y, rgb_color{110, 110, 110, 255});
	}

	y = panel.top + 44.0f;

	DrawRegisterSummary(BPoint(panel.left + 10.0f, y), displayA, displayX, displayY, displayS, displayP);

	y = panel.top + 68.0f;

	cpu_disasm_line_t line = DisassembleAddress(displayPC);

	BString instructionText;

	if (line.operand.Length() > 0) {
		instructionText.SetToFormat("$%04X: %s %s", line.address, line.mnemonic.String(), line.operand.String());
	} else {
		instructionText.SetToFormat("$%04X: %s", line.address, line.mnemonic.String());
	}

	SetFont(&fixed);
	SetHighColor(0, 0, 0);
	DrawString(instructionText.String(), BPoint(panel.left + 10.0f, y));

	SetFont(&normal);
	SetHighColor(70, 70, 70);
	DrawString("Space: freeze   S: step   V: frame   G: run   F: follow   P: PC   B: breakpoint"
				"   C: clear BP   Enter: center   Esc: clear   R: reset N: NMI I: IRQ vectors",
				BPoint(panel.left + 10.0f, panel.top + 98.0f));

	SetFont(&normal);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawDisasmPanel
//
// Draws disassembled CPU instructions.
//
// During normal execution, the current CPU PC is marked with the instruction
// arrow.  When execution is stopped by a debugger condition, the arrow instead
// marks the instruction responsible for that stop.  This is important for
// memory READ/WRITE watchpoints because the CPU PC may already have advanced
// beyond the instruction that performed the memory access.
//
// A frozen disassembly view takes precedence over both live-PC and debugger-hit
// positioning.
//
// When follow-PC mode is active, the visible disassembly range follows the
// effective display PC.
//
// Instruction-row traversal stops at the end of the 16-bit CPU address space.
// An instruction beginning near $FFFF may fetch operand bytes using normal
// 6502 address wrapping, but the disassembly list itself never wraps around and
// begins displaying $0000 as the next sequential row.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawDisasmPanel()
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden()) 
							? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	BRect panel(4.0f, 124.0f, rightEdge, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Instructions");

	if (!HasROMLoaded()) {
		DrawNoROMMessage(panel);
		return;
	}

	DrawInstructionLegend(panel);

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	/*
	 * Normally the instruction marker follows the CPU's live PC.
	 *
	 * A debugger hit is different: for memory watchpoints the CPU PC may
	 * already have advanced, while debug_breakpoint_hit_address() preserves
	 * the starting address of the instruction responsible for the stop.
	 */
	uint16 displayPC = state.pc;

	if (nes::cpu::debug_breakpoint_hit()) {
		displayPC = nes::cpu::debug_breakpoint_hit_address();
	}

	/*
	 * A frozen view intentionally preserves its captured PC regardless of
	 * subsequent CPU or debugger state.
	 */
	if (fFreezeUpdates) {
		displayPC = fFrozenPC;
	}

	if (fFollowPC && !fFreezeUpdates) {
		fBaseAddress = FindContextBase(displayPC, 5);

		UpdateScrollBar();
	}

	const float pcX = 12.0f;
	const float addrX = pcX + 42.0f;
	const float bytesX = addrX + 76.0f;
	const float instrX = bytesX + 92.0f;
	const float commentX = instrX + 108.0f;
	float y = panel.top + 58.0f;

	SetHighColor(80, 80, 80);
	DrawString("PC", BPoint(pcX, y));
	DrawString("Address", BPoint(addrX, y));
	DrawString("Bytes", BPoint(bytesX, y));
	DrawString("Instruction", BPoint(instrX, y));
	DrawString("Comment", BPoint(commentX, y));

	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));

	y += 4.0f;

	const uint32 rows = static_cast<uint32>((panel.bottom - y - 8.0f) / lineH);
	uint16 address = fBaseAddress;

	for (uint32 row = 0; row < rows; row++) {
		const bool active = (address == displayPC);

		DrawDisasmLine(y, address, active);
		cpu_disasm_line_t line = DisassembleAddress(address);

		const uint32 length = (line.length == 0) ? 1 : line.length;
		const uint32 nextAddress = static_cast<uint32>(address) + length;

		/*
		 * The instruction beginning at the current address is valid to
		 * display, but there is no sequential CPU address after $FFFF.
		 *
		 * Do not allow the row traversal to wrap back to $0000.
		 */
		if (nextAddress > 0xffff) {
			break;
		}

		address = static_cast<uint16>(nextAddress);

		y += lineH;
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::AddressForPoint
//
// Converts a mouse position inside the visible disassembly instruction list into
// the CPU address of the row under the pointer.
//
// This follows the same panel geometry and row stepping used by
// DrawDisasmPanel(), including variable instruction lengths.
//
// Row traversal stops at the end of the 16-bit CPU address space so mouse
// selection cannot wrap from an instruction near $FFFF back to a synthetic
// $0000 row.
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the CPU address for the clicked row.
//
// Returns:
//   true if the point maps to a visible disassembly row.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::AddressForPoint (BPoint where, uint16 &address)
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
			? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;
	BRect panel(4.0f, 124.0f, rightEdge, Bounds().bottom - 8.0f);

	if (!panel.Contains(where)) {
		return false;
	}

	if (!HasROMLoaded()) {
		return false;
	}

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	SetFont(&prevFont);

	float y = panel.top + 58.0f;
	y += lineH + 8.0f;
	y += 4.0f;

	const uint32 rows = static_cast<uint32>((panel.bottom - y - 8.0f) / lineH);
	uint16 rowAddress = fBaseAddress;

	for (uint32 row = 0; row < rows; row++) {
		const float rowTop = y - lineH + 2.0f;
		const float rowBottom = y + 4.0f;

		if (where.y >= rowTop && where.y <= rowBottom) {
			address = rowAddress;
			return true;
		}

		cpu_disasm_line_t line = DisassembleAddress(rowAddress);

		const uint32 length = (line.length == 0) ? 1 : line.length;
		const uint32 nextAddress = static_cast<uint32>(rowAddress) + length;

		/*
		 * Keep hit-testing synchronized with DrawDisasmPanel().
		 *
		 * Once the current instruction reaches the end of CPU address
		 * space, no further visible sequential row exists.
		 */
		if (nextAddress > 0xffff) {
			break;
		}

		rowAddress = static_cast<uint16>(nextAddress);
		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::VisibleDisasmRows
//
// Calculates how many disassembly instruction rows fit in the visible
// instruction panel using the same geometry as DrawDisasmPanel() and
// AddressForPoint().
//
// Parameters:
//   None.
//
// Returns:
//   Number of visible instruction rows.
// -----------------------------------------------------------------------------
int32
CPUDisasmView::VisibleDisasmRows() const
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
		? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;
	BRect panel(4.0f, 124.0f, rightEdge, Bounds().bottom - 8.0f);

	BFont prevFont;
	const_cast<CPUDisasmView *>(this)->GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	const_cast<CPUDisasmView *>(this)->SetFont(&fixed);

	font_height fh;
	const_cast<CPUDisasmView *>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const_cast<CPUDisasmView *>(this)->SetFont(&prevFont);

	float y = panel.top + 58.0f;
	y += lineH + 8.0f;
	y += 4.0f;

	const int32 rows = static_cast<int32>((panel.bottom - y - 8.0f) / lineH);

	if (rows < 1) {
		return 1;
	}

	return rows;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawInstructionLegend
//
// Draws a compact color legend for highlighted instruction categories.
//
// Parameters:
//   panel - Instruction panel bounds.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawInstructionLegend (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(10.0f);
	SetFont(&font);

	float x = panel.left + 8.0f;
	const float y = panel.top + 34.0f;

	auto drawItem = [&](const char *label, rgb_color color) {
		SetHighColor(color);
		FillRect(BRect(x, y - 8.0f, x + 8.0f, y));

		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(x + 12.0f, y));

		x += 12.0f + StringWidth(label) + 14.0f;
	};

	drawItem("Breakpoint", rgb_color{170, 0, 0, 255});
	drawItem("PPU", rgb_color{0, 80, 160, 255});
	drawItem("OAM", rgb_color{120, 0, 120, 255});
	drawItem("APU/IO", rgb_color{0, 110, 0, 255});
	drawItem("Flow", rgb_color{170, 85, 0, 255});
	drawItem("Load", rgb_color{40, 80, 170, 255});
	drawItem("Store", rgb_color{150, 60, 30, 255});
	drawItem("Undocumented", rgb_color{95, 65, 145, 255});

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.  The disassembler
// can safely read dummy zero-filled memory without a mapper, but showing a
// friendly message is clearer than displaying pages of BRK instructions.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM/mapper is currently loaded.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawNoROMMessage
//
// Draws a friendly empty-state message inside the instruction panel when no ROM
// is loaded.
//
// Parameters:
//   panel - Instruction panel bounds.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	SetHighColor(80, 80, 80);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to view CPU disassembly.";

	font_height fh;
	GetFontHeight(&fh);

	const float titleWidth = StringWidth(title);
	const float detailWidth = StringWidth(detail);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	DrawString(title, BPoint(centerX - (titleWidth * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - (detailWidth * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawRegisterSummary
//
// Draws a compact CPU register summary.  Labels use the normal UI font, while
// hexadecimal register values use a fixed-width font for easier visual scanning.
//
// Parameters:
//   origin - Top-left baseline point for the summary.
//   a      - CPU accumulator.
//   x      - CPU X register.
//   y      - CPU Y register.
//   s      - CPU stack pointer.
//   p      - CPU processor status.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawRegisterSummary (BPoint origin, uint8 a, uint8 x, uint8 y, uint8 s, uint8 p)
{
	BFont normalFont;
	GetFont(&normalFont);

	BFont fixedFont(be_fixed_font);
	fixedFont.SetSize(11.0f);

	float xPos = origin.x;
	const float yPos = origin.y;

	auto drawRegister = [&](const char *label, uint8 value) {
		BString sValue;
		sValue.SetToFormat("$%02X", value);

		SetFont(&normalFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(xPos, yPos));
		xPos += StringWidth(label) + 3.0f;

		SetFont(&fixedFont);
		SetHighColor(0, 0, 0);
		DrawString(sValue.String(), BPoint(xPos, yPos));
		xPos += StringWidth(sValue.String()) + 14.0f;
	};

	drawRegister("A:", a);
	drawRegister("X:", x);
	drawRegister("Y:", y);
	drawRegister("S:", s);
	drawRegister("P:", p);

	SetFont(&normalFont);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DrawDisasmLine
//
// Draws a single disassembled CPU instruction row. The current PC row is
// highlighted. Instructions that touch PPU registers, OAM DMA,
// APU/controller registers, control-flow instructions, load/store instructions,
// and undocumented opcodes get distinct colors. Hardware labels,
// control-flow comments, and common CPU idiom comments are drawn in a separate
// aligned comment column.
//
// Hardware accesses are indicated by the row background while instruction text
// color describes the instruction itself. This allows a store to remain
// store-colored even when writing to a PPU, OAM, or APU register.
//
// The marker column shows instruction/debug trace state:
//
//   orange badge = current PC
//   red dot      = execute BreakPoint
//   filled dot   = previously executed instruction
//   hollow dot   = not yet executed / possible data
//
// A selected row gets a blue outline. Selection is independent of the current
// PC and is used as the preferred target for BreakPoint toggling.
//
// Opcode and operand bytes are drawn separately so the opcode stands out from
// the instruction operands. Conditional branch comments are colored by the
// branch state represented by the current disassembly state: green when the
// branch would be taken, gray when it would not be taken.
//
// Parameters:
//   y       - Text baseline.
//   address - CPU address to disassemble.
//   active  - Whether this row is the current PC.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::DrawDisasmLine (float y, uint16 address, bool active)
{
	cpu_disasm_line_t line = DisassembleAddress(address);

	const float pcX = 12.0f;
	const float addrX = pcX + 42.0f;
	const float bytesX = addrX + 76.0f;
	const float instrX = bytesX + 92.0f;
	const float commentX = instrX + 108.0f;
	const float byteStep = 24.0f;
	const float rowLeft = 8.0f;
	const float rowRight = (fScrollBar && !fScrollBar->IsHidden())
			? fScrollBar->Frame().left - 12.0f : Bounds().right - 12.0f;

	const bool executed = nes::cpu::debug_instruction_was_executed(line.address);
	const bool breakpoint = nes::cpu::debug_has_execute_breakpoint(line.address);
	const bool selected = fHasSelectedAddress && (line.address == fSelectedAddress);

	const bool ppuWrite = IsPPURegisterWrite(line);
	const bool oamDMA = IsOAMDMAWrite(line);
	const bool apuOrController = IsAPUOrControllerRegister(line);
	const bool controlFlow = IsControlFlowInstruction(line);
	const bool loadInstruction = IsLoadInstruction(line);
	const bool storeInstruction = IsStoreInstruction(line);
	const bool undocumented = IsUndocumentedInstruction(line);
	const bool jam = line.mnemonic == "jam";
	const bool conditionalBranch = IsConditionalBranchInstruction(line);

	/*
	 * Row background describes special debugger/hardware context.
	 */
	if (active) {
		SetHighColor(255, 245, 170);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	} else if (ppuWrite) {
		SetHighColor(220, 238, 255);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	} else if (oamDMA) {
		SetHighColor(242, 224, 250);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	} else if (apuOrController) {
		SetHighColor(226, 244, 226);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	} else if (controlFlow) {
		SetHighColor(255, 238, 214);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	} else if (undocumented) {
		SetHighColor(238, 232, 248);
		FillRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	}

	if (selected) {
		SetHighColor(35, 90, 180);
		StrokeRect(BRect(rowLeft, y - 11.0f, rowRight, y + 3.0f));
	}

	/*
	 * Draw instruction/debug marker.
	 */
	if (active) {
		const float markerX = pcX + 1.0f;
		const float markerY = y - 9.0f;

		SetHighColor(160, 95, 0);
		FillRect(BRect(markerX, markerY, markerX + 14.0f, markerY + 12.0f));

		SetHighColor(255, 255, 255);
		DrawString(">", BPoint(markerX + 4.0f, y));

		if (breakpoint) {
			SetHighColor(170, 0, 0);
			FillEllipse(BRect(markerX + 10.0f, markerY - 2.0f,
				markerX + 16.0f, markerY + 4.0f));
		}
	} else {
		const float markerX = pcX + 4.0f;
		const float markerY = y - 5.0f;

		if (breakpoint) {
			SetHighColor(170, 0, 0);
			FillEllipse(BRect(markerX - 1.0f, markerY - 1.0f,
				markerX + 7.0f, markerY + 7.0f));
		} else if (executed) {
			SetHighColor(0, 135, 0);
			FillEllipse(BRect(markerX, markerY,
				markerX + 6.0f, markerY + 6.0f));
		} else {
			SetHighColor(145, 145, 145);
			StrokeEllipse(BRect(markerX + 1.0f, markerY + 1.0f,
				markerX + 5.0f, markerY + 5.0f));
		}
	}

	BString s;

	/*
	 * Address text uses the same semantic instruction color.
	 *
	 * Store/load classification takes priority over special hardware
	 * destinations. Hardware access is already represented by the row
	 * background.
	 */
	if (jam) {
		SetHighColor(130, 130, 130);
	} else if (storeInstruction && undocumented) {
		SetHighColor(155, 45, 125);
	} else if (loadInstruction && undocumented) {
		SetHighColor(45, 70, 175);
	} else if (storeInstruction) {
		SetHighColor(150, 60, 30);
	} else if (loadInstruction) {
		SetHighColor(40, 80, 170);
	} else if (controlFlow) {
		SetHighColor(170, 85, 0);
	} else if (oamDMA) {
		SetHighColor(120, 0, 120);
	} else if (ppuWrite) {
		SetHighColor(0, 80, 160);
	} else if (apuOrController) {
		SetHighColor(0, 110, 0);
	} else if (undocumented) {
		SetHighColor(95, 65, 145);
	} else {
		SetHighColor((active ? 0 : 80), (active ? 0 : 80), (active ? 0 : 80));
	}

	s.SetToFormat("$%04X", line.address);
	DrawString(s.String(), BPoint(addrX, y));

	/*
	 * Draw opcode and operand bytes.
	 */
	for (uint8 i = 0; i < 3; i++) {
		if (i >= line.length) {
			continue;
		}

		s.SetToFormat("%02X", line.bytes[i]);

		if (i == 0) {
			if (active) {
				SetHighColor(125, 65, 0);
			} else {
				SetHighColor(35, 35, 35);
			}
		} else {
			SetHighColor(105, 105, 105);
		}

		DrawString(s.String(), BPoint(bytesX + byteStep * i, y));
	}

	/*
	 * Instruction text uses the same semantic priority as the address:
	 *
	 *   store -> store color
	 *   load  -> load color
	 *
	 * Special hardware destinations remain visible through their row
	 * background.
	 */
	if (jam) {
		SetHighColor(130, 130, 130);
	} else if (storeInstruction && undocumented) {
		SetHighColor(155, 45, 125);
	} else if (loadInstruction && undocumented) {
		SetHighColor(45, 70, 175);
	} else if (storeInstruction) {
		SetHighColor(150, 60, 30);
	} else if (loadInstruction) {
		SetHighColor(40, 80, 170);
	} else if (controlFlow) {
		SetHighColor(170, 85, 0);
	} else if (oamDMA) {
		SetHighColor(120, 0, 120);
	} else if (ppuWrite) {
		SetHighColor(0, 80, 160);
	} else if (apuOrController) {
		SetHighColor(0, 110, 0);
	} else if (undocumented) {
		SetHighColor(95, 65, 145);
	} else {
		SetHighColor((active ? 0 : 80), (active ? 0 : 80), (active ? 0 : 80));
	}

	BString instr;

	if (line.operand.Length() > 0) {
		instr.SetToFormat("%s %s", line.mnemonic.String(), line.operand.String());
	} else {
		instr.SetTo(line.mnemonic);
	}

	DrawString(instr.String(), BPoint(instrX, y));

	/*
	 * Draw aligned explanatory comment.
	 */
	BString comment;
	BuildCommentForLine(line, comment);

	if (comment.Length() > 0) {
		BString commentText;
		commentText.SetToFormat("; %s", comment.String());

		if (conditionalBranch) {
			if (BranchTakenForLine(line)) {
				SetHighColor(0, 115, 0);
			} else {
				SetHighColor(120, 120, 120);
			}
		} else {
			SetHighColor(90, 90, 90);
		}

		DrawString(commentText.String(), BPoint(commentX, y));
	}
}


// -----------------------------------------------------------------------------
// CPUDisasmView::SetFollowPC
//
// Enables or disables follow-PC mode.
//
// In live mode, enabling follow recenters the disassembly around the current
// CPU PC.  In frozen mode, enabling follow instead recenters around the PC
// captured by the frozen snapshot so the view never jumps back into live CPU
// state while frozen.
//
// Parameters:
//   follow - true to enable follow-PC mode.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::SetFollowPC (bool follow)
{
    fFollowPC = follow;

    if (fFollowPC) {
        uint16 displayPC = 0x0000;

        if (fFreezeUpdates && fHaveFrozenSnapshot) {
			displayPC = fFrozenPC;
        } else {
            nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
			displayPC = state.pc;
        }

        fBaseAddress = FindContextBase(displayPC, 5);
    }

    UpdateScrollBar();
    Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::ScrollLines
//
// Moves the disassembly base address by a signed number of disassembled
// instruction rows.  Manual scrolling disables follow-PC mode.
//
// Backward disassembly is inherently ambiguous on the 6502 because instructions
// have variable lengths.  When viewing cartridge PRG-ROM, scrolling upward from
// $8000 stops at $8000 instead of interpreting bytes below the cartridge ROM
// region as an instruction that happens to cross the $8000 boundary.
//
// Frozen-mode scrolling uses the captured memory snapshot in both directions so
// manual navigation remains consistent with the frozen disassembly state.
//
// Parameters:
//   lines - Number of instruction rows to scroll.  Negative values move upward;
//           positive values move downward.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::ScrollLines (int32 lines)
{
	if (lines == 0) {
		return;
	}

	fFollowPC = false;

	if (lines > 0) {
		for (int32 i = 0; i < lines; i++) {
			cpu_disasm_line_t line = DisassembleAddress(fBaseAddress);

			const uint32 length = (line.length == 0) ? 1 : line.length;
			const uint32 nextAddress = static_cast<uint32>(fBaseAddress) + length;

			if (nextAddress > 0xffff) {
				break;
			}

			fBaseAddress = static_cast<uint16>(nextAddress);
		}
	} else {
		for (int32 i = 0; i < -lines; i++) {
			/*
			 * $8000 is the beginning of cartridge PRG-ROM.
			 *
			 * Do not manufacture a previous instruction from bytes below
			 * $8000 whose decoded length merely happens to cross into the
			 * cartridge code region.
			 */
			if (fBaseAddress == 0x8000) {
				break;
			}

			uint16 best = FindInstructionBefore(fBaseAddress);

			/*
			 * If the current address is in PRG-ROM, do not allow the
			 * backward search to cross below $8000.
			 */
			if ((fBaseAddress >= 0x8000) && (best < 0x8000)) {
				fBaseAddress = 0x8000;
				break;
			}

			fBaseAddress = best;
		}
	}

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsStoreInstruction
//
// Returns whether a disassembled instruction is a store-like instruction that
// writes a CPU register value, or a derived undocumented register value, to
// memory.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if the mnemonic represents a memory store.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsStoreInstruction (const cpu_disasm_line_t &line) const
{
	return line.mnemonic == "sta"
		|| line.mnemonic == "stx"
		|| line.mnemonic == "sty"

		// Undocumented store-like instructions.  Include both common
		// mnemonic names and names matching the emulator opcode classes.
		|| line.mnemonic == "sax"
		|| line.mnemonic == "aax"
		|| line.mnemonic == "sha"
		|| line.mnemonic == "axa"
		|| line.mnemonic == "shx"
		|| line.mnemonic == "sxa"
		|| line.mnemonic == "shy"
		|| line.mnemonic == "sya"
		|| line.mnemonic == "tas"
		|| line.mnemonic == "xas";
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsLoadInstruction
//
// Returns whether a disassembled instruction is a load-like instruction.  These
// instructions read data into A, X, Y, or a related register combination and
// are highlighted separately because they are very useful while tracing program
// state changes.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if the mnemonic represents a memory load.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsLoadInstruction (const cpu_disasm_line_t &line) const
{
	return line.mnemonic == "lda"
		|| line.mnemonic == "ldx"
		|| line.mnemonic == "ldy"

		// Undocumented load-like instructions.
		|| line.mnemonic == "lax"
		|| line.mnemonic == "lar"
		|| line.mnemonic == "las";
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsUndocumentedInstruction
//
// Returns whether a disassembled instruction is one of the undocumented 6502
// opcodes supported by the emulator/disassembler.  These are highlighted so
// unofficial opcode use stands out while debugging ROMs.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if the mnemonic is an undocumented opcode mnemonic.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsUndocumentedInstruction (const cpu_disasm_line_t &line) const
{
	return line.mnemonic == "aac"
		|| line.mnemonic == "aax"
		|| line.mnemonic == "arr"
		|| line.mnemonic == "asr"
		|| line.mnemonic == "axa"
		|| line.mnemonic == "axs"
		|| line.mnemonic == "dcp"
		|| line.mnemonic == "isc"
		|| line.mnemonic == "jam"
		|| line.mnemonic == "lar"
		|| line.mnemonic == "lax"
		|| line.mnemonic == "rla"
		|| line.mnemonic == "rra"
		|| line.mnemonic == "slo"
		|| line.mnemonic == "sre"
		|| line.mnemonic == "sxa"
		|| line.mnemonic == "sya"
		|| line.mnemonic == "xaa"
		|| line.mnemonic == "xas"

		// Common alternate names, in case the disassembler uses them.
		|| line.mnemonic == "alr"
		|| line.mnemonic == "anc"
		|| line.mnemonic == "las"
		|| line.mnemonic == "sax"
		|| line.mnemonic == "sbx"
		|| line.mnemonic == "sha"
		|| line.mnemonic == "shx"
		|| line.mnemonic == "shy"
		|| line.mnemonic == "tas"
		|| line.mnemonic == "isb"
		|| line.mnemonic == "shs"
		|| line.mnemonic == "ane"
		|| line.mnemonic == "lxa";
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsPPURegisterWrite
//
// Returns whether a disassembled instruction appears to write directly to a
// CPU-visible PPU register.
//
// The eight PPU registers at $2000-$2007 are mirrored repeatedly throughout
// $2008-$3FFF, so any absolute operand in the complete $2000-$3FFF range is
// treated as a PPU-register access.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if this instruction writes to a PPU register or one of its mirrors.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsPPURegisterWrite(const cpu_disasm_line_t &line) const
{
    if (!IsStoreInstruction(line)) {
        return false;
    }

    uint16 address = 0;
	if (!ParseOperandAddress(line, address)) {
        return false;
    }

    return ((address >= 0x2000) && (address <= 0x3fff));
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsOAMDMAWrite
//
// Returns whether a disassembled instruction appears to write directly to the
// OAM DMA register at $4014.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if this instruction writes to $4014.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsOAMDMAWrite (const cpu_disasm_line_t &line) const
{
	if (!IsStoreInstruction(line)) {
		return false;
	}

	uint16 address = 0;

	if (!ParseOperandAddress(line, address)) {
		return false;
	}

	return (address == 0x4014);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsAPUOrControllerRegister
//
// Returns whether a disassembled instruction uses a meaningful CPU-visible APU
// or controller register operand.
//
// The unused APU addresses $4009 and $400D are excluded.  OAM DMA at $4014 is
// also excluded because it has its own dedicated disassembly classification.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if this instruction references a functional APU or controller register.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsAPUOrControllerRegister (const cpu_disasm_line_t &line) const
{
    uint16 address = 0;

    if (!ParseOperandAddress(line, address)) {
        return false;
    }

    if (address == 0x4009 || address == 0x400d || address == 0x4014) {

        return false;
    }

    return ((address >= 0x4000) && (address <= 0x4017));
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsConditionalBranchInstruction
//
// Returns whether a disassembled instruction is one of the eight conditional
// 6502 branch instructions.
//
// Keeping this classification in one helper avoids duplicating the complete
// branch-mnemonic list in rendering, control-flow classification, and comment
// generation.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if the instruction is a conditional branch.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsConditionalBranchInstruction (const cpu_disasm_line_t &line) const
{
    return line.mnemonic == "bpl"
		|| line.mnemonic == "bmi"
		|| line.mnemonic == "bvc"
		|| line.mnemonic == "bvs"
		|| line.mnemonic == "bcc"
		|| line.mnemonic == "bcs"
		|| line.mnemonic == "bne"
		|| line.mnemonic == "beq";
}


// -----------------------------------------------------------------------------
// CPUDisasmView::IsControlFlowInstruction
//
// Returns whether a disassembled instruction changes or may change CPU control
// flow.  This includes conditional branches, jumps, subroutine calls, and
// returns.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if the instruction is branch/jump/call/return-like.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsControlFlowInstruction (const cpu_disasm_line_t &line) const
{
	return IsConditionalBranchInstruction(line)
		|| line.mnemonic == "jmp"
		|| line.mnemonic == "jsr"
		|| line.mnemonic == "rts"
		|| line.mnemonic == "rti";
}


// -----------------------------------------------------------------------------
// CPUDisasmView::ParseOperandAddress
//
// Attempts to parse a four-digit hexadecimal address from a disassembled operand
// string.  This handles operands that begin with an absolute address such as
// "$2000", "$C000", "$C000,X", and "($C000)".
//
// Parameters:
//   line    - Disassembled instruction line.
//   address - Receives the parsed address on success.
//
// Returns:
//   true if an address was parsed.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::ParseOperandAddress (const cpu_disasm_line_t &line, uint16 &address) const
{
	address = 0;

	if (line.operand.Length() < 5) {
		return false;
	}

	int32 start = 0;

	if (line.operand[0] == '$') {
		start = 1;
	} else if (line.operand[0] == '(' && line.operand.Length() >= 6
		&& line.operand[1] == '$') {
		start = 2;
	} else {
		return false;
	}

	for (int32 i = 0; i < 4; i++) {
		char c = line.operand[start + i];
		address <<= 4;

		if (c >= '0' && c <= '9') {
			address |= c - '0';
		} else if (c >= 'A' && c <= 'F') {
			address |= c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			address |= c - 'a' + 10;
		} else {
			return false;
		}
	}

	return true;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::HardwareLabelForOperand
//
// Returns a short hardware-register label for CPU-visible IO/register operands.
//
// The PPU registers at $2000-$2007 are mirrored throughout $2008-$3FFF.  PPU
// operands in that complete range are normalized to their canonical register
// before selecting a label.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   Static hardware label string, or nullptr if the operand is not a known
//   hardware register.
// -----------------------------------------------------------------------------
const char*
CPUDisasmView::HardwareLabelForOperand(const cpu_disasm_line_t &line) const
{
    uint16 address = 0;

    if (!ParseOperandAddress(line, address)) {
        return nullptr;
    }

    /*
     * PPU registers $2000-$2007 are mirrored every eight bytes through
     * $3FFF.
     */
    if (address >= 0x2000 && address <= 0x3fff) {
		address = static_cast<uint16>(0x2000 + ((address - 0x2000) & 0x7));
    }

    switch (address) {
        case 0x2000:
            return "PPUCTRL";

        case 0x2001:
            return "PPUMASK";

        case 0x2002:
            return "PPUSTATUS";

        case 0x2003:
            return "OAMADDR";

        case 0x2004:
            return "OAMDATA";

        case 0x2005:
            return "PPUSCROLL";

        case 0x2006:
            return "PPUADDR";

        case 0x2007:
            return "PPUDATA";

        case 0x4000:
            return "SQ1_VOL";

        case 0x4001:
            return "SQ1_SWEEP";

        case 0x4002:
            return "SQ1_TIMER_LO";

        case 0x4003:
            return "SQ1_TIMER_HI";

        case 0x4004:
            return "SQ2_VOL";

        case 0x4005:
            return "SQ2_SWEEP";

        case 0x4006:
            return "SQ2_TIMER_LO";

        case 0x4007:
            return "SQ2_TIMER_HI";

        case 0x4008:
            return "TRI_LINEAR";

        case 0x400A:
            return "TRI_TIMER_LO";

        case 0x400B:
            return "TRI_TIMER_HI";

        case 0x400C:
            return "NOISE_VOL";

        case 0x400E:
            return "NOISE_PERIOD";

        case 0x400F:
            return "NOISE_LENGTH";

        case 0x4010:
            return "DMC_FREQ";

        case 0x4011:
            return "DMC_RAW";

        case 0x4012:
            return "DMC_ADDR";

        case 0x4013:
            return "DMC_LEN";

        case 0x4014:
            return "OAMDMA";

        case 0x4015:
            return "APUSTATUS";

        case 0x4016:
            return "JOY1";

        case 0x4017:
            return "JOY2/APUFRAME";

        default:
            break;
    }

    return nullptr;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::CPUIdiomCommentForLine
//
// Returns a short explanatory comment for common 6502 setup, stack, transfer,
// and flag instructions.  These comments make reset/NMI/IRQ handlers easier to
// scan without changing disassembly behavior.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   Static comment string, or nullptr if no CPU idiom comment applies.
// -----------------------------------------------------------------------------
const char*
CPUDisasmView::CPUIdiomCommentForLine (const cpu_disasm_line_t &line) const
{
	if (line.mnemonic == "sei") {
		return "disable IRQ";
	}

	if (line.mnemonic == "cli") {
		return "enable IRQ";
	}

	if (line.mnemonic == "cld") {
		return "clear decimal";
	}

	if (line.mnemonic == "sed") {
		return "set decimal";
	}

	if (line.mnemonic == "clc") {
		return "clear carry";
	}

	if (line.mnemonic == "sec") {
		return "set carry";
	}

	if (line.mnemonic == "clv") {
		return "clear overflow";
	}

	if (line.mnemonic == "txs") {
		return "set stack pointer";
	}

	if (line.mnemonic == "tsx") {
		return "load stack pointer";
	}

	if (line.mnemonic == "pha") {
		return "push A";
	}

	if (line.mnemonic == "pla") {
		return "pull A";
	}

	if (line.mnemonic == "php") {
		return "push status";
	}

	if (line.mnemonic == "plp") {
		return "pull status";
	}

	if (line.mnemonic == "tax") {
		return "A -> X";
	}

	if (line.mnemonic == "tay") {
		return "A -> Y";
	}

	if (line.mnemonic == "txa") {
		return "X -> A";
	}

	if (line.mnemonic == "tya") {
		return "Y -> A";
	}

	if (line.mnemonic == "inx") {
		return "X++";
	}

	if (line.mnemonic == "iny") {
		return "Y++";
	}

	if (line.mnemonic == "dex") {
		return "X--";
	}

	if (line.mnemonic == "dey") {
		return "Y--";
	}

	if (line.mnemonic == "nop") {
		return "no operation";
	}

	if (line.mnemonic == "brk") {
		return "software interrupt";
	}

	return nullptr;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::BuildCommentForLine
//
// Builds the explanatory comment shown for one disassembled instruction.
//
// Hardware-register labels have the highest priority because they identify
// CPU-visible PPU, APU, controller, and DMA register accesses.
//
// Conditional branches include the branch target, its direction relative to the
// current instruction, and whether the branch condition is satisfied using the
// processor-status flags represented by the current disassembly state.  Live
// mode uses the current CPU flags, while frozen mode uses the processor-status
// value captured with the frozen snapshot.
//
// Jumps, subroutine calls, and returns receive control-flow comments when
// appropriate.  Common 6502 idioms are annotated only when no more specific
// hardware-register or control-flow comment applies.
//
// Parameters:
//   line    - Disassembled CPU instruction line.
//   comment - Receives the generated explanatory comment.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::BuildCommentForLine (const cpu_disasm_line_t &line, BString &comment) const
{
	comment.SetTo("");

	const char *hardwareLabel = HardwareLabelForOperand(line);

	if (hardwareLabel) {
		comment.SetTo(hardwareLabel);
		return;
	}

	uint16 target = 0;
	const bool hasTarget = ParseOperandAddress(line, target);
	const bool branch = IsConditionalBranchInstruction(line);
	
	if (branch) {
		const char *takenText = BranchTakenForLine(line) ? "taken" : "not taken";

		if (hasTarget) {
			if (target < line.address) {
				comment.SetToFormat("branch back -> $%04X  %s", target, takenText);
			} else if (target > line.address) {
				comment.SetToFormat("branch forward -> $%04X  %s", target, takenText);
			} else {
				comment.SetToFormat("branch -> $%04X  %s", target, takenText);
			}
		} else {
			comment.SetToFormat("branch  %s", takenText);
		}

		return;
	}

	if (line.mnemonic == "jsr") {
		if (hasTarget) {
			comment.SetToFormat("call -> $%04X", target);
		} else {
			comment.SetTo("call");
		}

		return;
	}

	if (line.mnemonic == "jmp") {
		if (hasTarget) {
			comment.SetToFormat("jump -> $%04X", target);
		} else {
			comment.SetTo("jump");
		}

		return;
	}

	if (line.mnemonic == "rts") {
		comment.SetTo("return");
		return;
	}

	if (line.mnemonic == "rti") {
		comment.SetTo("interrupt return");
		return;
	}

	const char *idiomComment = CPUIdiomCommentForLine(line);

	if (idiomComment) {
		comment.SetTo(idiomComment);
	}
}


// -----------------------------------------------------------------------------
// CPUDisasmView::BranchTakenForLine
//
// Returns whether a conditional branch instruction would be taken using the
// processor-status flags represented by the current disassembly state.
//
// Live mode uses the current CPU processor-status flags.  Frozen mode uses the
// processor-status value captured with the frozen disassembly snapshot.
//
// Parameters:
//   line - Disassembled CPU instruction line.
//
// Returns:
//   true if the branch condition is satisfied.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::BranchTakenForLine (const cpu_disasm_line_t &line) const
{
	uint8 p = 0x00;

	if (fFreezeUpdates && fHaveFrozenSnapshot) {
		p = fFrozenP;
	} else {
		nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
		p = state.p;
	}

	const bool n = (p & 0x80) != 0;
	const bool v = (p & 0x40) != 0;
	const bool z = (p & 0x02) != 0;
	const bool c = (p & 0x01) != 0;

	if (line.mnemonic == "bpl") {
		return !n;
	}

	if (line.mnemonic == "bmi") {
		return n;
	}

	if (line.mnemonic == "bvc") {
		return !v;
	}

	if (line.mnemonic == "bvs") {
		return v;
	}

	if (line.mnemonic == "bcc") {
		return !c;
	}

	if (line.mnemonic == "bcs") {
		return c;
	}

	if (line.mnemonic == "bne") {
		return !z;
	}

	if (line.mnemonic == "beq") {
		return z;
	}

	return false;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::FindInstructionBefore
//
// Finds the most likely instruction start immediately preceding the supplied
// CPU address.
//
// Because 6502 instructions are at most three bytes long, the function probes
// up to three bytes backward and looks for an instruction whose decoded length
// lands exactly on the requested address.
//
// Backward probing is clamped at $0000 so subtraction can never underflow and
// wrap into the top of the 16-bit CPU address space.
//
// Parameters:
//   address - CPU address whose preceding instruction should be found.
//
// Returns:
//   Address of the best preceding instruction start.
// -----------------------------------------------------------------------------
uint16
CPUDisasmView::FindInstructionBefore (uint16 address) const
{
    if (address == 0x0000) {
        return 0x0000;
    }

    uint16 best = static_cast<uint16>(address - 1);

    for (int32 back = 1; back <= 3; back++) {

        if (back > address) {
            break;
        }

        const uint16 candidate = static_cast<uint16>(address - back);
		cpu_disasm_line_t line = DisassembleAddress(candidate);

        const uint32 length = (line.length == 0) ? 1 : line.length;
		const uint32 nextAddress = static_cast<uint32>(candidate) + length;

        if (nextAddress == static_cast<uint32>(address)) {

            best = candidate;
        }
    }

    return best;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::FindContextBase
//
// Finds a disassembly base address a few instruction rows before the current PC.
// This gives the live disassembly view useful context above and below the active
// instruction.
//
// Parameters:
//   pc          - Current CPU program counter.
//   linesBefore - Desired number of previous instruction rows.
//
// Returns:
//   Starting address for the disassembly panel.
// -----------------------------------------------------------------------------
uint16
CPUDisasmView::FindContextBase (uint16 pc, int32 linesBefore) const
{
	uint16 address = pc;

	for (int32 i = 0; i < linesBefore; i++) {
		address = FindInstructionBefore(address);
	}

	return address;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::ReadVector
//
// Reads a little-endian 6502 vector from CPU memory using the side-effect-free
// debug memory reader.
//
// Parameters:
//   address - Address of the low byte of the vector.
//
// Returns:
//   16-bit vector target.
// -----------------------------------------------------------------------------
uint16
CPUDisasmView::ReadVector (uint16 address) const
{
	uint8 lo = nes::bus::debug_read_memory(address);
	uint8 hi = nes::bus::debug_read_memory(address + 1);

	return static_cast<uint16>(lo | (hi << 8));
}


// -----------------------------------------------------------------------------
// CPUDisasmView::JumpToAddress
//
// Switches to manual disassembly mode, jumps the view to the supplied CPU
// address, and selects that instruction so the destination is visually
// highlighted.
//
// This is used by debugger tools such as the BreakPoint Manager when navigating
// to an execute BreakPoint, to the instruction responsible for a memory
// watchpoint hit, or to the current CPU PC.
//
// Parameters:
//   address - CPU instruction address to display and select.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::JumpToAddress (uint16 address)
{
	fFollowPC = false;
	fBaseAddress = address;

	fHasSelectedAddress = true;
	fSelectedAddress = address;

	UpdateScrollBar();
	Invalidate();
}

// -----------------------------------------------------------------------------
// CPUDisasmView::JumpToCurrentPC
//
// Returns the disassembly view to the PC represented by its current display
// state and enables follow-PC mode.
//
// Live mode uses the current CPU PC.  Frozen mode uses the PC captured by the
// frozen snapshot so navigation cannot escape the frozen CPU state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::JumpToCurrentPC()
{
    uint16 displayPC = 0x0000;

    if (fFreezeUpdates && fHaveFrozenSnapshot) {
		displayPC = fFrozenPC;
    } else {
        nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
		displayPC = state.pc;
    }

    fFollowPC = true;
	fBaseAddress = FindContextBase(displayPC, 5);

    UpdateScrollBar();
    Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::JumpToVector
//
// Reads a CPU vector and jumps the disassembly view to its target address.
//
// Parameters:
//   vectorAddress - Address of the vector low byte.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::JumpToVector (uint16 vectorAddress)
{
	uint16 target = ReadVector(vectorAddress);

	JumpToAddress(target);
}


// -----------------------------------------------------------------------------
// CPUDisasmView::LayoutScrollBar
//
// Positions the vertical disassembly scrollbar along the right edge of the
// instruction panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::LayoutScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const float top = 124.0f;
	const float bottom = Bounds().bottom - 8.0f;
	const float width = B_V_SCROLL_BAR_WIDTH;

	fScrollBar->MoveTo(Bounds().right - width, top);
	fScrollBar->ResizeTo(width, bottom - top);

	UpdateScrollBar();
}

// -----------------------------------------------------------------------------
// CPUDisasmView::UpdateScrollBar
//
// Synchronizes the scrollbar value with the current disassembly base address.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	fUpdatingScrollBar = true;
	fScrollBar->SetValue(static_cast<float>(fBaseAddress));
	fUpdatingScrollBar = false;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::ScrollBarChanged
//
// Handles vertical scrollbar movement.
//
// The scrollbar is intentionally limited to cartridge PRG-ROM space,
// $8000-$FFFF.  This prevents ordinary scrollbar navigation from wandering
// through CPU RAM, mirrored registers, and other non-code regions where the
// disassembler would otherwise decode arbitrary data as instructions.
//
// Explicit jumps performed by other debugger tools are still allowed to target
// addresses below $8000; this restriction applies only to scrollbar navigation.
//
// Parameters:
//   value - CPU address selected by the scrollbar.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::ScrollBarChanged (float value)
{
	if (fUpdatingScrollBar) {
		return;
	}

	if (!HasROMLoaded()) {
		return;
	}

	int32 address = static_cast<int32>(value + 0.5f);

	if (address < 0x8000) {
		address = 0x8000;
	}

	if (address > 0xffff) {
		address = 0xffff;
	}

	fFollowPC = false;

	fBaseAddress = static_cast<uint16>(address);

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUDisasmView::CaptureFrozenSnapshot
//
// Captures the CPU state and complete CPU-visible address space for frozen
// disassembly mode.
//
// Memory is read through the debugger-safe memory path so the snapshot does not
// cause CPU bus side effects or trigger debugger memory watchpoints.  Keeping a
// complete 64 KB image allows the user to navigate through disassembly while
// frozen without later mapper changes altering the displayed instructions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::CaptureFrozenSnapshot()
{
	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	fFrozenPC = state.pc;
	fFrozenA = state.a;
	fFrozenX = state.x;
	fFrozenY = state.y;
	fFrozenS = state.s;
	fFrozenP = state.p;

	for (uint32 address = 0; address <= 0xffff; address++) {
		fFrozenMemory[address] = nes::bus::debug_read_memory(static_cast<uint16>(address));
	}

	fHaveFrozenSnapshot = true;
}


// -----------------------------------------------------------------------------
// CPUDisasmView::DisassembleAddress
//
// Disassembles one CPU address using the memory source represented by the
// current disassembly state.
//
// Live mode reads normally through the CPU disassembler. Frozen mode instead
// decodes bytes captured in the frozen 64 KB memory snapshot so mapper changes,
// RAM writes, and other memory changes that occur after freezing cannot alter
// the displayed disassembly.
//
// Frozen decoding uses DisassembleCPUBytes() so mapper changes that occur after
// the snapshot do not affect interpretation of the captured bytes.
//
// Parameters:
//   address - CPU address to disassemble.
//
// Returns:
//   Decoded disassembly line.
// -----------------------------------------------------------------------------
cpu_disasm_line_t
CPUDisasmView::DisassembleAddress (uint16 address) const
{
	if (!fFreezeUpdates || !fHaveFrozenSnapshot) {
		return DisassembleCPU(address);
	}

	uint8 bytes[3];

	for (uint32 i = 0; i < 3; i++) {
		const uint16 byteAddress = static_cast<uint16>(address + static_cast<uint16>(i));
		bytes[i] = fFrozenMemory[byteAddress];
	}

	return DisassembleCPUBytes(address, bytes);
}

