
#include "CPUDisasmView.h"


class CPUDisasmScrollBar : public BScrollBar
{
	public:
	CPUDisasmScrollBar (BRect frame, CPUDisasmView *owner)
		: BScrollBar(frame, "cpu_disasm_scrollbar", owner, 0x8000, 0xffff, B_VERTICAL)
	{
		fOwner = owner;

		SetSteps(1.0f, 256.0f);
	}

	public:
	virtual void ValueChanged (float value)
	{
		if (fOwner) {
			fOwner->ScrollBarChanged(value);
		}
	}

	private:
	CPUDisasmView *fOwner = nullptr;
};


CPUDisasmView::CPUDisasmView (BRect frame, PretendoWindow *parent)
	: BView(frame, "cpu_disasm_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


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


void
CPUDisasmView::FrameResized (float width, float height)
{
	(void)width;
	(void)height;

	LayoutScrollBar();

	BView::FrameResized(width, height);
}


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
// Space freezes or unfreezes only the disassembly view.
// S enters debugger step mode and advances one CPU instruction.
// V advances one full video frame while debugger-paused.
// G resumes normal emulator execution.
// F toggles follow-PC mode.
// P returns the view to the current PC and clears row selection.
// B toggles an execute breakpoint at the selected row, or at the current/frozen
// PC if no row is selected.
// Enter centers the view around the selected row.
// Esc clears the selected row.
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
				nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

				fFrozenPC = state.pc;
				fFreezeUpdates = true;

				if (fFollowPC) {
					fBaseAddress = FindContextBase(fFrozenPC, 5);
					UpdateScrollBar();
				}
			} else {
				fFreezeUpdates = false;

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
			fFollowPC = false;
			JumpToVector(0xfffc);
			break;

		case 'n':
		case 'N':
			fFreezeUpdates = false;
			fFollowPC = false;
			JumpToVector(0xfffa);
			break;

		case 'i':
		case 'I':
			fFreezeUpdates = false;
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

	if (nes::cpu::debug_breakpoint_hit()) {
		displayPC = nes::cpu::debug_breakpoint_hit_address();
	}

	if (fFreezeUpdates) {
		displayPC = fFrozenPC;
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

	DrawRegisterSummary(BPoint(panel.left + 10.0f, y), state.a, state.x, state.y, state.s, state.p);

	y = panel.top + 68.0f;

	CPUDisasmLine line = DisassembleCPU(displayPC);

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
	DrawString(
		"Space: freeze   S: step   V: frame   G: run   F: follow   P: PC   B: breakpoint"
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

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	SetFont(&mono);

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
		CPUDisasmLine line = DisassembleCPU(address);

		if (line.length == 0) {
			address++;
		} else {
			address += line.length;
		}

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

	BRect panel(4.0f, 124.0f, rightEdge, Bounds().bottom - 8.0);

	if (!panel.Contains(where)) {
		return false;
	}

	if (!HasROMLoaded()) {
		return false;
	}

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	SetFont(&mono);

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

		CPUDisasmLine line = DisassembleCPU(rowAddress);

		if (line.length == 0) {
			rowAddress++;
		} else {
			rowAddress += line.length;
		}

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

	BFont mono(be_fixed_font);
	mono.SetSize(10.0f);
	const_cast<CPUDisasmView *>(this)->SetFont(&mono);

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
// the instruction operands. Conditional branch comments are colored by their
// live branch state: green when the branch would be taken, gray when it would
// not be taken.
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
	CPUDisasmLine line = DisassembleCPU(address);

	const float pcX = 12.0f;
	const float addrX = pcX + 42.0f;
	const float bytesX = addrX + 76.0f;
	const float instrX = bytesX + 92.0f;
	const float commentX = instrX + 108.0f;

	const float byteStep = 24.0f;

	const float rowLeft = 8.0f;

	const float rowRight
		= (fScrollBar && !fScrollBar->IsHidden())
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

	const bool conditionalBranch = line.mnemonic == "bpl"
			|| line.mnemonic == "bmi"
			|| line.mnemonic == "bvc"
			|| line.mnemonic == "bvs"
			|| line.mnemonic == "bcc"
			|| line.mnemonic == "bcs"
			|| line.mnemonic == "bne"
			|| line.mnemonic == "beq";

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
			FillEllipse(BRect(markerX + 10.0f, markerY - 2.0f, markerX + 16.0f, markerY + 4.0f));
		}
	} else {
		const float markerX = pcX + 4.0f;
		const float markerY = y - 5.0f;

		if (breakpoint) {
			SetHighColor(170, 0, 0);
			FillEllipse(BRect(markerX - 1.0f, markerY - 1.0f, markerX + 7.0f, markerY + 7.0f));
		} else if (executed) {
			SetHighColor(0, 135, 0);
			FillEllipse(BRect(markerX, markerY, markerX + 6.0f, markerY + 6.0f));
		} else {
			SetHighColor(145, 145, 145);
			StrokeEllipse(BRect(markerX + 1.0f, markerY + 1.0f, markerX + 5.0f, markerY + 5.0f));
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
// Parameters:
//   follow - true to keep the disassembly based at the live PC.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::SetFollowPC (bool follow)
{
	fFollowPC = follow;

	if (fFollowPC) {
		nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
		fBaseAddress = FindContextBase(state.pc, 5);
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
			CPUDisasmLine line = DisassembleCPU(fBaseAddress);

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
			if (fBaseAddress >= 0x8000
				&& best < 0x8000) {
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
CPUDisasmView::IsStoreInstruction (const CPUDisasmLine &line) const
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
CPUDisasmView::IsLoadInstruction (const CPUDisasmLine &line) const
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
CPUDisasmView::IsUndocumentedInstruction (const CPUDisasmLine &line) const
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
// Returns whether a disassembled instruction appears to write directly to one of
// the CPU-visible PPU registers at $2000-$2007.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if this instruction writes to $2000-$2007.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsPPURegisterWrite (const CPUDisasmLine &line) const
{
	if (!IsStoreInstruction(line)) {
		return false;
	}

	uint16 address = 0;

	if (!ParseOperandAddress(line, address)) {
		return false;
	}

	return address >= 0x2000 && address <= 0x2007;
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
CPUDisasmView::IsOAMDMAWrite (const CPUDisasmLine &line) const
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
// Returns whether a disassembled instruction uses a CPU-visible APU or
// controller register operand.
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   true if this instruction references $4000-$4017, excluding OAM DMA $4014.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::IsAPUOrControllerRegister (const CPUDisasmLine &line) const
{
	uint16 address = 0;

	if (!ParseOperandAddress(line, address)) {
		return false;
	}

	if (address == 0x4014) {
		return false;
	}

	return (address >= 0x4000) && (address <= 0x4017);
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
CPUDisasmView::IsControlFlowInstruction (const CPUDisasmLine &line) const
{
	return line.mnemonic == "bpl"
		|| line.mnemonic == "bmi"
		|| line.mnemonic == "bvc"
		|| line.mnemonic == "bvs"
		|| line.mnemonic == "bcc"
		|| line.mnemonic == "bcs"
		|| line.mnemonic == "bne"
		|| line.mnemonic == "beq"
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
CPUDisasmView::ParseOperandAddress (const CPUDisasmLine &line, uint16 &address) const
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
// This is used to annotate disassembly rows such as "sta $2000 ; PPUCTRL" and
// "sta $4014 ; OAMDMA".
//
// Parameters:
//   line - Disassembled instruction line.
//
// Returns:
//   Static hardware label string, or nullptr if the operand is not a known
//   hardware register.
// -----------------------------------------------------------------------------
const char*
CPUDisasmView::HardwareLabelForOperand (const CPUDisasmLine &line) const
{
	uint16 address = 0;

	if (!ParseOperandAddress(line, address)) {
		return nullptr;
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

		case 0x4009:
			return "TRI_UNUSED";

		case 0x400A:
			return "TRI_TIMER_LO";

		case 0x400B:
			return "TRI_TIMER_HI";

		case 0x400C:
			return "NOISE_VOL";

		case 0x400D:
			return "NOISE_UNUSED";

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
CPUDisasmView::CPUIdiomCommentForLine (const CPUDisasmLine &line) const
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
// Builds the disassembly comment for one instruction.  Hardware register labels
// have highest priority because they identify CPU-visible IO/register accesses.
// Conditional branches include target direction, target address, and whether
// the branch would currently be taken based on the live CPU flags.  Common CPU
// idiom comments are used as a final fallback.
//
// Parameters:
//   line    - Disassembled instruction line.
//   comment - Receives the generated comment text.  Empty if no comment applies.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmView::BuildCommentForLine (const CPUDisasmLine &line, BString &comment) const
{
	comment.SetTo("");

	const char* hardwareLabel = HardwareLabelForOperand(line);

	if (hardwareLabel) {
		comment.SetTo(hardwareLabel);
		return;
	}

	uint16 target = 0;
	const bool hasTarget = ParseOperandAddress(line, target);

	const bool branch = line.mnemonic == "bpl"
		|| line.mnemonic == "bmi"
		|| line.mnemonic == "bvc"
		|| line.mnemonic == "bvs"
		|| line.mnemonic == "bcc"
		|| line.mnemonic == "bcs"
		|| line.mnemonic == "bne"
		|| line.mnemonic == "beq";

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
// Returns whether a conditional branch instruction would currently be taken
// based on the live CPU processor status flags.
//
// Parameters:
//   line - Disassembled CPU instruction line.
//
// Returns:
//   true if the branch condition is currently satisfied.
// -----------------------------------------------------------------------------
bool
CPUDisasmView::BranchTakenForLine (const CPUDisasmLine& line) const
{
	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
	const uint8 p = state.p;

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
// Attempts to find the nearest valid 6502 instruction boundary immediately
// before the supplied address.  Because 6502 instructions are variable-length,
// this checks a small backward window and selects the closest instruction whose
// length lands exactly on the target address.
//
// Parameters:
//   address - Instruction boundary to search before.
//
// Returns:
//   Best previous instruction address.
// -----------------------------------------------------------------------------
uint16
CPUDisasmView::FindInstructionBefore (uint16 address) const
{
	uint16 best = address - 1;

	for (int32 back = 1; back <= 3; back++) {
		uint16 candidate = address - back;
		CPUDisasmLine line = DisassembleCPU(candidate);

		if (line.length == 0) {
			continue;
		}

		if (static_cast<uint16>(candidate + line.length) == address) {
			best = candidate;
			break;
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
// Jumps the disassembly view back to the current CPU PC.  This also re-enables
// follow-PC mode.
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
	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	fFollowPC = true;
	fBaseAddress = FindContextBase(state.pc, 5);

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

