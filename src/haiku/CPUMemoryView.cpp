
#include "CPUMemoryView.h"


// -----------------------------------------------------------------------------
// IsHexDigit
//
// Returns whether a character is an ASCII hexadecimal digit.
//
// Parameters:
//   c - Character to test.
//
// Returns:
//   true if c is 0-9, a-f, or A-F.
// -----------------------------------------------------------------------------
static bool
IsHexDigit (char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


// -----------------------------------------------------------------------------
// HexDigitValue
//
// Converts one ASCII hexadecimal digit to its numeric value.
//
// Parameters:
//   c - Hexadecimal digit.
//
// Returns:
//   Numeric value 0-15.
// -----------------------------------------------------------------------------
static uint8
HexDigitValue (char c)
{
	if (c >= '0' && c <= '9') {
		return static_cast<uint8>(c - '0');
	}

	if (c >= 'a' && c <= 'f') {
		return static_cast<uint8>(10 + c - 'a');
	}

	if (c >= 'A' && c <= 'F') {
		return static_cast<uint8>(10 + c - 'A');
	}

	return 0;
}


// -----------------------------------------------------------------------------
// StringContains
//
// Returns whether a C string contains a substring.
//
// Parameters:
//   text   - Text to search.
//   needle - Substring to find.
//
// Returns:
//   true if needle appears inside text.
// -----------------------------------------------------------------------------
static bool
StringContains (const char *text, const char *needle)
{
	if (!text || !needle) {
		return false;
	}

	return strstr(text, needle) != nullptr;
}


class CPUMemoryScrollBar : public BScrollBar
{
	public:
		CPUMemoryScrollBar (BRect frame, CPUMemoryView *owner)
			: BScrollBar (frame, "cpu_memory_scrollbar", owner, 0.0f, 4095.0f, B_VERTICAL),
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
		CPUMemoryView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// CPUMemoryView::CPUMemoryView
//
// Creates the CPU memory viewer.  The view is read-only and displays CPU-visible
// memory through the side-effect-safe bus debug read path.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Main emulator window that owns this tool view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUMemoryView::CPUMemoryView (BRect frame, PretendoWindow *parent)
	: BView(frame, "cpu_memory_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fParent = parent;

	SetViewColor(216, 216, 216);
}


// -----------------------------------------------------------------------------
// CPUMemoryView::~CPUMemoryView
//
// Destroys the CPU memory viewer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUMemoryView::~CPUMemoryView()
{
}


// -----------------------------------------------------------------------------
// CPUMemoryView::AttachedToWindow
//
// Creates the vertical scrollbar, lays it out beside the CPU memory panel,
// enables pointer tracking for live byte hover selection, and gives the memory
// view keyboard focus so navigation keys work immediately.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (!fScrollBar) {
		BRect scrollFrame(Bounds().right - B_V_SCROLL_BAR_WIDTH, 202.0f, Bounds().right,Bounds().bottom - 8.0f);

		fScrollBar = new CPUMemoryScrollBar(scrollFrame, this);
		AddChild(fScrollBar);
	}

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	LayoutScrollBar();
	UpdateScrollBar();

	MakeFocus(true);
}


// -----------------------------------------------------------------------------
// CPUMemoryView::FrameResized
//
// Updates the memory scrollbar layout when the viewer is resized.
//
// Parameters:
//   width  - New view width.
//   height - New view height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::FrameResized (float width, float height)
{
	BView::FrameResized(width, height);

	LayoutScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM/mapper is loaded.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::Draw
//
// Draws the CPU memory viewer background, header panel, and memory grid.
//
// Parameters:
//   updateRect - Dirty region supplied by the app_server.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();
	DrawMemoryPanel();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::Pulse
//
// Refreshes the live CPU memory display and keeps transient hover inspection
// synchronized with the mouse position.
//
// The memory display itself continues to refresh whenever a ROM is loaded,
// regardless of whether the CPU Memory window is active.  Hover tracking is
// performed only while the window is active.
//
// When the window becomes inactive, transient hover state is cleared while any
// locked byte remains preserved.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	BWindow *window = Window();

	if (!window) {
		return;
	}

	if (!window->IsActive()) {
		if (fHasHoveredAddress) {
			fHasHoveredAddress = false;
			fHoveredAddress = 0x0000;
		}

		/*
		 * Memory, PC/SP state, operands, and instruction targets must remain
		 * live even while this debugger window is inactive.
		 */
		Invalidate();
		return;
	}

	BPoint where;
	uint32 buttons = 0;

	GetMouse(&where, &buttons, false);

	if (!Bounds().Contains(where)) {
		if (fHasHoveredAddress) {
			fHasHoveredAddress = false;
			fHoveredAddress = 0x0000;
		}

		Invalidate();
		return;
	}

	HoverAddressForPoint(where);

	/*
	 * Keep live memory, PC/SP state, operands, and instruction targets
	 * current while the window is active.
	 */
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::MouseDown
//
// Handles mouse clicks in the CPU memory grid.
//
// Clicking a hexadecimal or ASCII byte locks that address for inspection.
// Clicking the currently locked byte again releases the lock and returns the
// inspector to normal hover behavior.
//
// Mouse inspection is disabled while no ROM is loaded so an address selected
// from the empty memory panel cannot survive into a subsequently loaded ROM.
//
// Parameters:
//   where - Mouse position in this view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::MouseDown (BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		return;
	}

	/*
	 * Clicking the already-locked byte releases the lock while preserving
	 * that byte as the current hover address.
	 */
	if (fHasLockedAddress && fLockedAddress == address) {
		fHasLockedAddress = false;
		fLockedAddress = 0x0000;

		fHoveredAddress = address;
		fHasHoveredAddress = true;

		Invalidate();
		return;
	}

	/*
	 * Lock the newly selected byte.
	 */
	fLockedAddress = address;
	fHasLockedAddress = true;

	fHoveredAddress = address;
	fHasHoveredAddress = true;

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::MouseMoved
//
// Updates the hovered CPU memory byte while the pointer moves over the memory
// grid.  Leaving the view clears hover state but preserves any locked byte.
//
// Hover tracking is ignored while the CPU Memory window is inactive so the byte
// inspector does not behave as though the window still has focus.
//
// Parameters:
//   where       - Mouse position in view coordinates.
//   transit     - BView mouse transit state.
//   dragMessage - Optional drag message; currently unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::MouseMoved (BPoint where, uint32 transit, const BMessage *dragMessage)
{
	(void)dragMessage;

	BWindow *window = Window();

	if (!window || !window->IsActive()) {
		if (fHasHoveredAddress) {
			fHasHoveredAddress = false;
			fHoveredAddress = 0x0000;
			Invalidate();
		}

		return;
	}

	if (transit == B_ENTERED_VIEW) {
		MakeFocus(true);
	}

	if (transit == B_EXITED_VIEW) {
		if (fHasHoveredAddress) {
			fHasHoveredAddress = false;
			fHoveredAddress = 0x0000;
			Invalidate();
		}

		return;
	}

	if (!HasROMLoaded()) {
		return;
	}

	if (HoverAddressForPoint(where)) {
		Invalidate();
	}
}


// -----------------------------------------------------------------------------
// CPUMemoryView::HoverAddressForPoint
//
// Updates the hovered byte when a view coordinate maps to a visible memory byte.
// Both the hex column and ASCII column are handled by AddressForPoint().
//
// Parameters:
//   where - Mouse position in this view.
//
// Returns:
//   true if the hovered address changed.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::HoverAddressForPoint (BPoint where)
{
	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		return false;
	}

	if (fHasHoveredAddress && fHoveredAddress == address) {
		return false;
	}

	fHoveredAddress = address;
	fHasHoveredAddress = true;

	return true;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::AddressForPoint
//
// Converts a mouse position in the CPU memory view into a CPU address when the
// click falls inside either the hex byte columns or the ASCII column.
//
// Parameters:
//   where   - Mouse position in this view.
//   address - Receives the CPU address under the mouse when the function returns
//             true.
//
// Returns:
//   true if the mouse position maps to a visible memory byte.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::AddressForPoint (BPoint where, uint16 &address) const
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
							? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;
	BRect panel(4.0f, 202.0f, rightEdge, Bounds().bottom - 8.0f);

	if (!panel.Contains(where)) {
		return false;
	}

	BFont prevFont;
	const_cast<CPUMemoryView *>(this)->GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(10.0f);
	const_cast<CPUMemoryView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<CPUMemoryView *>(this)->GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float addrX = panel.left + 10.0f;
	const float hexX = addrX + 74.0f;
	const float byteStep = 27.0f;
	const float groupGap = 10.0f;
	const float asciiX = hexX + byteStep * 16.0f + groupGap + 10.0f;
	const float asciiStep = mono.StringWidth("M");

	const_cast<CPUMemoryView *>(this)->SetFont(&prevFont);

	float firstRowY = panel.top + 34.0f;
	firstRowY += lineH + 6.0f;
	firstRowY += 4.0f;

	if (where.y < firstRowY - lineH + 4.0f) {
		return false;
	}

	const int32 row = static_cast<int32>((where.y - (firstRowY - lineH + 4.0f)) / lineH);

	if (row < 0) {
		return false;
	}

	const int32 visibleRows = static_cast<int32>((panel.bottom - firstRowY - 8.0f) / lineH);

	if (row >= visibleRows) {
		return false;
	}

	int32 startRow = static_cast<int32>(fBaseAddress >> 4);

	if (startRow < 0) {
		startRow = 0;
	} else if (startRow > 4095) {
		startRow = 4095;
	}

	const int32 memoryRow = startRow + row;

	if (memoryRow < 0 || memoryRow > 4095) {
		return false;
	}

	int32 byteIndex = -1;

	for (int32 i = 0; i < 16; i++) {
		float hexByteX = hexX + i * byteStep;

		if (i >= 8) {
			hexByteX += groupGap;
		}

		BRect hexRect(hexByteX - 4.0f, firstRowY + row * lineH - 13.0f, hexByteX + 19.0f, firstRowY + row * lineH + 4.0f);

		if (hexRect.Contains(where)) {
			byteIndex = i;
			break;
		}
	}

	if (byteIndex < 0) {
		for (int32 i = 0; i < 16; i++) {
			const float asciiCharX = asciiX + i * asciiStep;

			BRect asciiRect(asciiCharX - 2.0f, firstRowY + row * lineH - 12.0f,  
							asciiCharX + asciiStep,firstRowY + row * lineH + 3.0f);

			if (asciiRect.Contains(where)) {
				byteIndex = i;
				break;
			}
		}
	}

	if (byteIndex < 0) {
		return false;
	}

	address = static_cast<uint16>((memoryRow << 4) + byteIndex);
	return true;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::DrawHeaderPanel
//
// Draws the CPU memory viewer title, current base address, region label, CPU
// register summary, current decoded instruction, current instruction target,
// stack pointer summary, interrupt vector targets, inspected-byte information,
// keyboard shortcuts, and a compact color legend.  Debug values are drawn with
// a fixed-width font so hexadecimal values align clearly.
//
// The 6502 stack pointer points to the next free stack slot.  The byte most
// recently pushed is normally at SP + 1 within page $0100, so the stack summary
// shows both the raw SP slot and the next four stack bytes above it.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 194.0f);
	::DrawDebugPanel(this, panel, "CPU Memory");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(10.0f);

	BFont uiFont = prevFont;
	uiFont.SetSize(11.0f);

	const float textX = panel.left + 10.0f;
	const float statusY = panel.top + 36.0f;
	const float instrY = panel.top + 54.0f;
	const float targetY = panel.top + 72.0f;
	const float vectorY = panel.top + 90.0f;
	const float stackY = panel.top + 108.0f;
	const float selectedY = panel.top + 126.0f;
	const float helpY = panel.top + 154.0f;
	const float legendY = panel.top + 174.0f;

	BString s;

	SetFont(&mono);
	SetHighColor(0, 0, 0);

	if (HasROMLoaded()) {
		nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
		const uint16 stackSlotAddress = static_cast<uint16>(0x100 | state.s);

		cpu_disasm_line_t pcLine = DisassembleCPU(state.pc);
		uint16 instructionLength = pcLine.length;

		if (instructionLength == 0) {
			instructionLength = 1;
		} else if (instructionLength > 3) {
			instructionLength = 3;
		}

		s.SetToFormat("Base:$%04X  %-15s  PC:$%04X  Len:%u  SP:$%02X($%04X)  A:$%02X  X:$%02X  Y:$%02X  P:$%02X",
					  	fBaseAddress, RegionLabel(fBaseAddress), state.pc, instructionLength, state.s, stackSlotAddress,
						state.a, state.x, state.y, state.p);
		DrawString(s.String(), BPoint(textX, statusY));

		BString byteText;

		if (instructionLength == 1) {
			byteText.SetToFormat("%02X      ", pcLine.bytes[0]);
		} else if (instructionLength == 2) {
			byteText.SetToFormat("%02X %02X   ", pcLine.bytes[0], pcLine.bytes[1]);
		} else {
			byteText.SetToFormat("%02X %02X %02X", pcLine.bytes[0], pcLine.bytes[1], pcLine.bytes[2]);
		}

		s.SetToFormat("Instr:  $%04X  %-8s  %s", pcLine.address, byteText.String(), pcLine.text.String());

		SetHighColor(40, 40, 40);
		DrawString(s.String(), BPoint(textX, instrY));

		DrawInstructionTargetInfo(textX, targetY);

		const uint16 nmiVector = ReadVector(0xfffa);
		const uint16 resetVector = ReadVector(0xfffc);
		const uint16 irqVector = ReadVector(0xfffe);

		s.SetToFormat("Vectors: NMI:$%04X  RESET:$%04X  IRQ/BRK:$%04X", nmiVector, resetVector, irqVector);

		SetHighColor(60, 60, 60);
		DrawString(s.String(), BPoint(textX, vectorY));

		const uint8 stackIndex1 = static_cast<uint8>(state.s + 1);
		const uint8 stackIndex2 = static_cast<uint8>(state.s + 2);
		const uint8 stackIndex3 = static_cast<uint8>(state.s + 3);
		const uint8 stackIndex4 = static_cast<uint8>(state.s + 4);

		const uint16 stackAddress1 = static_cast<uint16>(0x100 | stackIndex1);
		const uint16 stackAddress2 = static_cast<uint16>(0x100 | stackIndex2);
		const uint16 stackAddress3 = static_cast<uint16>(0x100 | stackIndex3);
		const uint16 stackAddress4 = static_cast<uint16>(0x100 | stackIndex4);

		const uint8 stackValue1 = nes::bus::debug_read_memory(stackAddress1);
		const uint8 stackValue2 = nes::bus::debug_read_memory(stackAddress2);
		const uint8 stackValue3 = nes::bus::debug_read_memory(stackAddress3);
		const uint8 stackValue4 = nes::bus::debug_read_memory(stackAddress4);

		s.SetToFormat("Stack:  Slot:$%04X  Top:$%04X  +1:$%02X  +2:$%02X  +3:$%02X  +4:$%02X",
						stackSlotAddress, stackAddress1, stackValue1, stackValue2, stackValue3, stackValue4);

		SetHighColor(70, 60, 40);
		DrawString(s.String(), BPoint(textX, stackY));

		DrawSelectedByteInfo(textX, selectedY);
	} else {
		s.SetToFormat("Base:$%04X  %-15s", fBaseAddress, RegionLabel(fBaseAddress));

		DrawString(s.String(), BPoint(textX, statusY));
		DrawInstructionTargetInfo(textX, targetY);
		DrawSelectedByteInfo(textX, selectedY);
	}

	SetFont(&uiFont);
	SetHighColor(90, 90, 90);

	DrawString("Arrows: scroll   PageUp/PageDown: page   C: PC   E: target   K:"
				" stack ptr   Z: zero   S: stack page   R: RAM   P: PPU   A: APU   V: vectors", BPoint(textX, helpY));

	DrawString("Legend: orange=PC/opcode   pale orange=operand   blue=SP   green=target   gray=hover   black=locked",
				BPoint(textX, legendY));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUMemoryView::DrawNoROMMessage
//
// Draws a friendly message when no ROM is loaded.
//
// Parameters:
//   panel - Memory panel bounds.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawNoROMMessage (BRect panel)
{
	SetHighColor(90, 90, 90);
	DrawString("No ROM loaded.", BPoint(panel.left + 12.0f, panel.top + 36.0f));
	DrawString("Load a ROM to view CPU memory.", BPoint(panel.left + 12.0f, panel.top + 56.0f));
}


// -----------------------------------------------------------------------------
// CPUMemoryView::DrawByteCell
//
// Draws one hexadecimal byte in the memory grid.  The current PC opcode byte is
// highlighted strongly, the remaining bytes of the current instruction are
// highlighted with a distinct lighter orange, and the current stack pointer byte
// is highlighted blue.
//
// Parameters:
//   x           - Left position for the byte text.
//   y           - Baseline position for the byte text.
//   address     - CPU address represented by this byte.
//   value       - Byte value to draw.
//   isPC        - true if this byte is the current program counter.
//   isPCOperand - true if this byte is part of the current instruction operand.
//   isSP        - true if this byte is at the current stack pointer address.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawByteCell (float x, float y, uint16 address, uint8 value, bool isPC, bool isPCOperand, bool isSP)
{
	(void)address;

	const BRect cellRect(x - 3.0f, y - 12.0f, x + 18.0f, y + 3.0f);

	if (isPC || isPCOperand || isSP) {
		if (isPC) {
			SetHighColor(255, 190, 95);
		} else if (isPCOperand) {
			SetHighColor(255, 220, 150);
		} else {
			SetHighColor(195, 220, 255);
		}

		FillRect(cellRect);

		if (isPC) {
			SetHighColor(150, 80, 0);
		} else if (isPCOperand) {
			SetHighColor(180, 110, 20);
		} else {
			SetHighColor(50, 100, 170);
		}

		StrokeRect(cellRect);
	}

	BString byteText;
	byteText.SetToFormat("%02X", value);

	if (isPC) {
		SetHighColor(90, 45, 0);
	} else if (isPCOperand) {
		SetHighColor(120, 70, 0);
	} else if (isSP) {
		SetHighColor(0, 60, 130);
	} else {
		SetHighColor(20, 20, 20);
	}

	DrawString(byteText.String(), BPoint(x, y));
}


// -----------------------------------------------------------------------------
// CPUMemoryView::DrawMemoryPanel
//
// Draws a hexadecimal dump of CPU-visible memory beginning at fBaseAddress.
//
// Bytes are read through nes::bus::debug_read_memory() so inspection does not
// trigger normal memory/register side effects.
//
// The current PC opcode byte, current instruction operand bytes, stack-pointer
// address, current instruction target, hovered byte, and locked byte are
// highlighted when visible.
//
// Instruction-byte addresses are calculated with 16-bit wrapping so an
// instruction beginning near $FFFF correctly continues at $0000.
//
// Each memory row receives a soft region-based background color first.  PC/SP
// row highlights are drawn afterward, followed by individual opcode, operand,
// SP, target, hover, and lock indicators.
//
// This function uses 16-byte memory row numbers instead of incrementing a
// uint16 row address.  Row 0 is $0000 and row 4095 is $FFF0, preventing the
// visible memory grid itself from wrapping from $FFFF back to zero page.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawMemoryPanel()
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
			? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	BRect panel(4.0f, 202.0f, rightEdge, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Memory");

	SetHighColor(216, 216, 216);
	FillRect(BRect(panel.left + 6.0f, panel.top + 24.0f, panel.right - 6.0f, panel.bottom - 6.0f));

	if (!HasROMLoaded()) {
		DrawNoROMMessage(panel);
		return;
	}

	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	const uint16 pcAddress = state.pc;
	const uint16 spAddress = static_cast<uint16>(0x100 | state.s);

	cpu_disasm_line_t pcLine = DisassembleCPU(pcAddress);

	uint16 pcInstructionLength = pcLine.length;

	if (pcInstructionLength == 0) {
		pcInstructionLength = 1;
	} else if (pcInstructionLength > 3) {
		pcInstructionLength = 3;
	}

	/*
	 * Determine the actual 16-bit addresses occupied by the instruction.
	 *
	 * Using uint16 arithmetic here intentionally allows:
	 *
	 *     $FFFF + 1 -> $0000
	 *     $FFFF + 2 -> $0001
	 *
	 * which matches the CPU's address-bus wrapping.
	 */
	const uint16 pcOperandAddress1 = static_cast<uint16>(pcAddress + 1);
	const uint16 pcOperandAddress2 = static_cast<uint16>(pcAddress + 2);

	auto isInstructionOperand = [&](uint16 address) {
		if ((pcInstructionLength >= 2) && (address == pcOperandAddress1)) {
			return true;
		}

		if ((pcInstructionLength >= 3) && (address == pcOperandAddress2)) {
			return true;
		}

		return false;
	};

	uint16 instructionTargetAddress = 0x0000;

	const bool hasInstructionTarget = CurrentInstructionTarget(instructionTargetAddress);

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(10.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float addrX = panel.left + 10.0f;
	const float hexX = addrX + 74.0f;
	const float byteStep = 27.0f;
	const float groupGap = 10.0f;
	const float asciiX = hexX + byteStep * 16.0f + groupGap + 10.0f;
	const float asciiStep = mono.StringWidth("M");
	const float regionX = asciiX + asciiStep * 16.0f + 20.0f;
	float y = panel.top + 34.0f;

	SetHighColor(80, 80, 80);
	DrawString("Address", BPoint(addrX, y));
	DrawString("Hex bytes", BPoint(hexX, y));
	DrawString("ASCII", BPoint(asciiX, y));
	DrawString("Region", BPoint(regionX, y));

	y += lineH + 6.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));

	y += 4.0f;

	const int32 rows = static_cast<int32>((panel.bottom - y - 8.0f) / lineH);
	int32 startRow = static_cast<int32>(fBaseAddress >> 4);

	if (startRow < 0) {
		startRow = 0;
	} else if (startRow > 4095) {
		startRow = 4095;
	}

	for (int32 row = 0; row < rows; row++) {
		const int32 memoryRow = startRow + row;

		if (memoryRow > 4095) {
			break;
		}

		const uint16 address = static_cast<uint16>(memoryRow << 4);
		const uint16 rowStart = address;
		const uint16 rowEnd = static_cast<uint16>(address + 15);

		/*
		 * Check whether any byte belonging to the current instruction lies
		 * inside this memory row.  Testing explicit wrapped addresses avoids
		 * the $FFFF->$0000 boundary problem of a linear numeric range.
		 */
		bool pcInRow = false;

		if ((pcAddress >= rowStart) && (pcAddress <= rowEnd)) {
			pcInRow = true;
		}

		if ((!pcInRow) && (pcInstructionLength >= 2) && (pcOperandAddress1 >= rowStart)
			&& (pcOperandAddress1 <= rowEnd)) {
			pcInRow = true;
		}

		if ((!pcInRow) && (pcInstructionLength >= 3) && (pcOperandAddress2 >= rowStart)
			&& (pcOperandAddress2 <= rowEnd)) {
			pcInRow = true;
		}

		const bool spInRow = (spAddress >= rowStart) && (spAddress <= rowEnd);
		const bool stackRow = (address >= 0x100) && (address <= 0x1ff);
		const bool ppuRegisterRow = (address >= 0x2000) && (address <= 0x3fff);
		const bool apuRegisterRow = (address >= 0x4000) && (address <= 0x401f);
		const bool prgRow = (address >= 0x8000);

		SetRegionBackgroundColor(address);

		FillRect(BRect(panel.left + 6.0f, y - lineH + 4.0f, panel.right - 6.0f, y + 3.0f));

		if (pcInRow) {
			SetHighColor(255, 245, 220);
			FillRect(BRect(panel.left + 6.0f, y - lineH + 4.0f, panel.right - 6.0f, y + 3.0f));
		} else if (spInRow) {
			SetHighColor(230, 240, 255);
			FillRect(BRect(panel.left + 6.0f, y - lineH + 4.0f, panel.right - 6.0f, y + 3.0f));
		}

		BString s;
		s.SetToFormat("$%04X", address);

		if (pcInRow) {
			SetHighColor(120, 60, 0);
		} else if (spInRow) {
			SetHighColor(0, 60, 130);
		} else {
			SetHighColor(0, 0, 0);
		}

		DrawString(s.String(), BPoint(addrX, y));

		for (int32 i = 0; i < 16; i++) {
			const uint16 byteAddress = static_cast<uint16>(address + i);
			const uint8 value = nes::bus::debug_read_memory(byteAddress);
			const bool isPC = (byteAddress == pcAddress);
			const bool isPCOperand = isInstructionOperand(byteAddress);
			const bool isSP = (byteAddress == spAddress);

			const bool isHovered = fHasHoveredAddress && byteAddress == fHoveredAddress;
			const bool isLocked = fHasLockedAddress && byteAddress == fLockedAddress;
			const bool isInstructionTarget = hasInstructionTarget && byteAddress == instructionTargetAddress;

			float hexByteX = hexX + i * byteStep;

			if (i >= 8) {
				hexByteX += groupGap;
			}

			DrawByteCell(hexByteX, y, byteAddress, value, isPC, isPCOperand, isSP);

			if (isInstructionTarget) {
				SetHighColor(0, 130, 0);

				BRect targetRect(hexByteX - 3.0f, y - 12.0f, hexByteX + 18.0f, y + 3.0f);
				StrokeRect(targetRect);
			}

			if (isHovered) {
				SetHighColor(90, 90, 90);

				BRect hoverRect(hexByteX - 4.0f, y - 13.0f, hexByteX + 19.0f, y + 3.0f);
				StrokeRect(hoverRect);
			}

			if (isLocked) {
				SetHighColor(0, 0, 0);

				const float left = hexByteX - 5.0f;
				const float top = y - 14.0f;
				const float right = hexByteX + 20.0f;
				const float bottom = y + 3.0f;

				StrokeLine(BPoint(left, top), BPoint(right, top));
				StrokeLine(BPoint(right, top), BPoint(right, bottom));
				StrokeLine(BPoint(right, bottom), BPoint(left, bottom));
				StrokeLine(BPoint(left, bottom), BPoint(left, top));
			}

			const char asciiChar = value >= 32 && value <= 126 ? static_cast<char>(value) : '.';

			BString asciiText;
			asciiText << asciiChar;

			const float asciiCharX = asciiX + i * asciiStep;

			if (isInstructionTarget) {
				SetHighColor(0, 130, 0);
				StrokeRect(BRect(asciiCharX - 2.0f, y - 12.0f, asciiCharX + asciiStep, y + 3.0f));
			}

			if (isHovered) {
				SetHighColor(245, 245, 245);
				FillRect(BRect(asciiCharX - 2.0f, y - 12.0f, asciiCharX + asciiStep, y + 3.0f));

				SetHighColor(90, 90, 90);
				StrokeRect(BRect(asciiCharX - 2.0f, y - 12.0f, asciiCharX + asciiStep, y + 3.0f));
			}

			if (isLocked) {
				const float left = asciiCharX - 3.0f;
				const float top = y - 13.0f;
				const float right = asciiCharX + asciiStep + 1.0f;
				const float bottom = y + 3.0f;

				SetHighColor(255, 255, 255);
				FillRect(BRect(left, top, right, bottom));

				SetHighColor(0, 0, 0);
				StrokeLine(BPoint(left, top), BPoint(right, top));
				StrokeLine(BPoint(right, top), BPoint(right, bottom));
				StrokeLine(BPoint(right, bottom), BPoint(left, bottom));
				StrokeLine(BPoint(left, bottom), BPoint(left, top));
			}

			/*
			 * ASCII text follows the same row-color convention as before.
			 */
			if (pcInRow) {
				SetHighColor(120, 60, 0);
			} else if (spInRow) {
				SetHighColor(0, 60, 130);
			} else {
				SetHighColor(70, 70, 70);
			}

			DrawString(asciiText.String(), BPoint(asciiCharX, y));
		}

		if (ppuRegisterRow || apuRegisterRow) {
			SetHighColor(70, 90, 120);
		} else if (stackRow) {
			SetHighColor(110, 80, 40);
		} else if (prgRow) {
			SetHighColor(50, 100, 50);
		} else {
			SetHighColor(90, 90, 90);
		}

		BString region;
		region.SetTo(RegionLabel(address));

		if (ppuRegisterRow || apuRegisterRow) {
			region << " safe";
		}

		if (address == 0xfff0) {
			region << " / NMI RESET IRQ";
		} else {
			const char *vectorLabel = VectorLabel(address);

			if (vectorLabel) {
				region << " / ";
				region << vectorLabel;
			}
		}

		DrawString(region.String(), BPoint(regionX, y));

		y += lineH;
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUMemoryView::RegionLabel
//
// Returns a short label describing the broad CPU address-space region containing
// the supplied address.
//
// Parameters:
//   address - CPU address to classify.
//
// Returns:
//   Static region label string.
// -----------------------------------------------------------------------------
const char*
CPUMemoryView::RegionLabel (uint16 address) const
{
	if (address <= 0xff) {
		return "Zero page";
	}

	if (address <= 0x1ff) {
		return "Stack";
	}

	if (address <= 0x7ff) {
		return "Internal RAM";
	}

	if (address <= 0x1fff) {
		return "RAM mirror";
	}

	if (address <= 0x3fff) {
		return "PPU regs/mirrors";
	}

	if (address <= 0x401f) {
		return "APU/input regs";
	}

	if (address <= 0x5fff) {
		return "Expansion";
	}

	if (address <= 0x7fff) {
		return "SRAM/mapper";
	}

	return "PRG ROM/mapper";
}


// -----------------------------------------------------------------------------
// CPUMemoryView::SetRegionBackgroundColor
//
// Selects a soft background color for a CPU address region.  These colors are
// intentionally pale so PC, operand, SP, stack, hardware, and PRG highlights can
// still be drawn over them clearly.
//
// Parameters:
//   address - CPU address whose memory region should determine the color.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::SetRegionBackgroundColor (uint16 address)
{
	if (address <= 0xff) {
		// Zero page - slightly stronger purple/blue so it stands out.
		SetHighColor(218, 225, 250);
	} else if (address <= 0x1ff) {
		// Stack
		SetHighColor(245, 235, 210);
	} else if (address <= 0x7ff) {
		// Internal RAM
		SetHighColor(232, 240, 246);
	} else if (address <= 0x1fff) {
		// RAM mirrors
		SetHighColor(238, 238, 238);
	} else if (address <= 0x3fff) {
		// PPU registers and mirrors
		SetHighColor(225, 235, 245);
	} else if (address <= 0x401f) {
		// APU and controller registers
		SetHighColor(235, 228, 245);
	} else if (address <= 0x5fff) {
		// Expansion area
		SetHighColor(232, 240, 232);
	} else if (address <= 0x7fff) {
		// SRAM / mapper RAM
		SetHighColor(240, 240, 220);
	} else {
		// PRG ROM / mapper
		SetHighColor(230, 240, 230);
	}
}


// -----------------------------------------------------------------------------
// CPUMemoryView::VectorLabel
//
// Returns a short interrupt-vector label for addresses in the CPU vector area.
//
// Parameters:
//   address - CPU address to classify.
//
// Returns:
//   Static vector label string, or nullptr if the address is not a vector row.
// -----------------------------------------------------------------------------
const char*
CPUMemoryView::VectorLabel (uint16 address) const
{
	if (address <= 0xfff9) {
		return nullptr;
	}

	if (address <= 0xfffb) {
		return "NMI vector";
	}

	if (address <= 0xfffd) {
		return "RESET vector";
	}

	return "IRQ/BRK vector";
}


// -----------------------------------------------------------------------------
// CPUMemoryView::ReadVector
//
// Reads a little-endian CPU vector through the side-effect-safe bus debug read
// path.
//
// Parameters:
//   address - Low-byte address of the vector.
//
// Returns:
//   16-bit vector target address.
// -----------------------------------------------------------------------------
uint16
CPUMemoryView::ReadVector (uint16 address) const
{
	const uint8 low = nes::bus::debug_read_memory(address);
	const uint8 high = nes::bus::debug_read_memory(static_cast<uint16>(address + 1));

	return static_cast<uint16>(low | (high << 8));
}

// -----------------------------------------------------------------------------
// CPUMemoryView::ReadZeroPageVector
//
// Reads a little-endian pointer from zero page, wrapping the high-byte read
// within zero page.  This matches 6502 zero-page indirect addressing behavior.
//
// Parameters:
//   address - Zero-page pointer address.
//
// Returns:
//   16-bit target address.
// -----------------------------------------------------------------------------
uint16
CPUMemoryView::ReadZeroPageVector (uint8 address) const
{
	const uint8 low = nes::bus::debug_read_memory(address);
	const uint8 high = nes::bus::debug_read_memory(static_cast<uint8>(address + 1));

	return static_cast<uint16>(low | (high << 8));
}


// -----------------------------------------------------------------------------
// CPUMemoryView::Read6502IndirectVector
//
// Reads a little-endian absolute indirect JMP vector, including the original
// 6502 page-wrap behavior where ($xxFF) reads the high byte from $xx00.
//
// Parameters:
//   address - Low-byte address of the absolute indirect vector.
//
// Returns:
//   16-bit target address.
// -----------------------------------------------------------------------------
uint16
CPUMemoryView::Read6502IndirectVector (uint16 address) const
{
	const uint8 low = nes::bus::debug_read_memory(address);
	const uint16 highAddress = static_cast<uint16>((address & 0xff00) | ((address + 1) & 0xff));
	const uint8 high = nes::bus::debug_read_memory(highAddress);

	return static_cast<uint16>(low | (high << 8));
}


// -----------------------------------------------------------------------------
// CPUMemoryView::Clear
//
// Clears ROM-specific CPU memory inspection state.
//
// Hovered and locked addresses are discarded so a selection made while viewing
// one ROM does not remain active after that ROM is unloaded.  The current base
// address is intentionally preserved so the user's memory-view position remains
// unchanged.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::Clear()
{
	fHasHoveredAddress = false;
	fHoveredAddress = 0x0000;

	fHasLockedAddress = false;
	fLockedAddress = 0x0000;

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::ParseOperandAddress
//
// Extracts the first hexadecimal address from a disassembly operand string.
// Immediate operands are intentionally ignored because they do not refer to a
// memory target.
//
// Examples:
//   "$20"       -> value $0020, digits 2
//   "$2000,X"   -> value $2000, digits 4
//   "($20),Y"   -> value $0020, digits 2
//   "#$10"      -> false
//
// Parameters:
//   operand - Operand text from cpu_disasm_line_t.
//   value   - Receives the parsed address value.
//   digits  - Receives the number of hex digits parsed.
//
// Returns:
//   true if an address-like operand was found.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::ParseOperandAddress (const BString &operand, uint16 &value, int32 &digits) const
{
	const char *text = operand.String();

	if (!text || text[0] == '\0') {
		return false;
	}

	if (text[0] == '#') {
		return false;
	}

	const char *dollar = strchr(text, '$');

	if (!dollar) {
		return false;
	}

	value = 0;
	digits = 0;

	const char *p = dollar + 1;

	while (*p && IsHexDigit(*p) && digits < 4) {
		value = static_cast<uint16>((value << 4) | HexDigitValue(*p));
		digits++;
		p++;
	}

	return digits > 0;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::CurrentInstructionTarget
//
// Attempts to compute the effective memory or control-flow target referenced by
// the current CPU instruction.
//
// This handles zero-page, zero-page indexed, absolute, absolute indexed,
// indexed-indirect, indirect-indexed, JSR/JMP absolute, relative branch targets
// as emitted by the disassembler, and JMP absolute-indirect addressing.
//
// Original 6502 JMP indirect page-wrap behavior is handled by
// Read6502IndirectVector().
//
// Parameters:
//   address - Receives the resolved target/effective CPU address.
//
// Returns:
//   true if the current instruction has a useful target address.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::CurrentInstructionTarget (uint16 &address) const
{
	if (!HasROMLoaded()) {
		return false;
	}

	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
	cpu_disasm_line_t line = DisassembleCPU(state.pc);

	uint16 baseAddress = 0x0000;
	int32 digits = 0;

	if (!ParseOperandAddress(line.operand, baseAddress, digits)) {
		return false;
	}

	const char *mnemonic = line.mnemonic.String();
	const char *operand = line.operand.String();

	const bool hasX = StringContains(operand, ",X") || StringContains(operand, ",x");
	const bool hasY = StringContains(operand, ",Y") || StringContains(operand, ",y");
	const bool isIndirect = StringContains(operand, "(") && StringContains(operand, ")");

	/*
	 * Absolute-indirect JMP:
	 *
	 *     JMP ($1234)
	 *
	 * The emulator's disassembler normally uses lowercase mnemonics, but
	 * accept either case here so target resolution is not dependent on
	 * presentation formatting.
	 */
	const bool isJMP = strcmp(mnemonic, "jmp") == 0 || strcmp(mnemonic, "JMP") == 0;

	if (isJMP && isIndirect && digits > 2) {
		address = Read6502IndirectVector(baseAddress);

		return true;
	}

	/*
	 * Zero-page indirect addressing:
	 *
	 *     ($20,X)
	 *     ($20),Y
	 */
	if (isIndirect && digits <= 2) {
		uint8 pointer = static_cast<uint8>(baseAddress);

		if (hasX) {
			pointer = static_cast<uint8>(pointer + state.x);
			address = ReadZeroPageVector(pointer);

			return true;
		}

		address = ReadZeroPageVector(pointer);

		if (hasY) {
			address = static_cast<uint16>(address + state.y);
		}

		return true;
	}

	/*
	 * Zero-page and zero-page indexed addressing.
	 *
	 * uint8 arithmetic intentionally provides 6502 zero-page wrapping.
	 */
	if (digits <= 2) {
		uint8 target = static_cast<uint8>(baseAddress);

		if (hasX) {
			target = static_cast<uint8>(target + state.x);
		} else if (hasY) {
			target = static_cast<uint8>(target + state.y);
		}

		address = target;

		return true;
	}

	/*
	 * Absolute addressing, absolute indexing, JSR/JMP absolute, and
	 * absolute branch targets already resolved by the disassembler.
	 */
	address = baseAddress;

	if (hasX) {
		address = static_cast<uint16>(address + state.x);
	} else if (hasY) {
		address = static_cast<uint16>(address + state.y);
	}

	return true;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::JumpToAddress
//
// Moves the memory viewer to a specific CPU address, aligned to a 16-byte row.
// The target row is clamped so the visible memory grid cannot extend beyond
// $FFFF and wrap back to zero page.
//
// Parameters:
//   address - CPU address to place near the top of the memory grid.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::JumpToAddress (uint16 address)
{
	const int32 visibleRows = VisibleMemoryRows();
	int32 maxTopRow = 4096 - visibleRows;

	if (maxTopRow < 0) {
		maxTopRow = 0;
	}

	int32 row = static_cast<int32>(address >> 4);

	if (row > maxTopRow) {
		row = maxTopRow;
	}

	fBaseAddress = static_cast<uint16>(row << 4);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::ScrollLines
//
// Scrolls the memory viewer by a signed number of 16-byte rows.
//
// Parameters:
//   lines - Number of rows to scroll.  Negative values scroll upward.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::ScrollLines (int32 lines)
{
	const int32 visibleRows = VisibleMemoryRows();
	int32 maxTopRow = 4096 - visibleRows;

	if (maxTopRow < 0) {
		maxTopRow = 0;
	}

	int32 row = static_cast<int32>(fBaseAddress >> 4);
	row += lines;

	if (row < 0) {
		row = 0;
	} else if (row > maxTopRow) {
		row = maxTopRow;
	}

	fBaseAddress = static_cast<uint16>(row << 4);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::KeyDown
//
// Handles CPU memory viewer navigation hotkeys.
//
// Parameters:
//   bytes    - Key bytes supplied by the app_server.
//   numBytes - Number of key bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	switch (bytes[0]) {
		case B_UP_ARROW:
			ScrollLines(-1);
			break;

		case B_DOWN_ARROW:
			ScrollLines(1);
			break;

		case B_PAGE_UP:
			ScrollLines(-16);
			break;

		case B_PAGE_DOWN:
			ScrollLines(16);
			break;

		case 'c':
		case 'C':
			if (HasROMLoaded()) {
				nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
				JumpToAddress(state.pc);
			}
			break;

		case 'k':
		case 'K':
			if (HasROMLoaded()) {
				nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();
				JumpToAddress(static_cast<uint16>(0x100 | state.s));
			}
			break;

		case 'z':
		case 'Z':
			JumpToAddress(0x0000);
			break;

		case 's':
		case 'S':
			JumpToAddress(0x100);
			break;

		case 'r':
		case 'R':
			JumpToAddress(0x200);
			break;

		case 'p':
		case 'P':
			JumpToAddress(0x2000);
			break;

		case 'a':
		case 'A':
			JumpToAddress(0x4000);
			break;

		case 'v':
		case 'V':
			JumpToAddress(0xfff0);
			break;
			
		case 'e':
		case 'E':
		{
			uint16 targetAddress = 0x0000;

			if (CurrentInstructionTarget(targetAddress)) {
				JumpToAddress(targetAddress);

				fLockedAddress = targetAddress;
				fHasLockedAddress = true;

				fHoveredAddress = targetAddress;
				fHasHoveredAddress = true;

				Invalidate();
			}
		} break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// CPUMemoryView::LayoutScrollBar
//
// Positions the vertical scrollbar beside the memory panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::LayoutScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	BRect frame(Bounds().right - B_V_SCROLL_BAR_WIDTH, 202.0f, Bounds().right, Bounds().bottom - 8.0f);
	fScrollBar->MoveTo(frame.LeftTop());
	fScrollBar->ResizeTo(frame.Width(), frame.Height());

	UpdateScrollBar();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::UpdateScrollBar
//
// Synchronizes the scrollbar value and range with the current base address.
// Each scrollbar unit represents one 16-byte memory row.  The maximum value is
// reduced by the number of visible rows so the view cannot scroll past $FFFF and
// wrap back to zero page.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const int32 visibleRows = VisibleMemoryRows();
	int32 maxTopRow = 4096 - visibleRows;

	if (maxTopRow < 0) {
		maxTopRow = 0;
	}
 
	const int32 currentRow = static_cast<int32>(fBaseAddress >> 4);

	if (currentRow > maxTopRow) {
		fBaseAddress = static_cast<uint16>(maxTopRow << 4);
	}

	fUpdatingScrollBar = true;

	fScrollBar->SetRange(0.0f, static_cast<float>(maxTopRow));
	fScrollBar->SetSteps(1.0f, 16.0f);
	fScrollBar->SetProportion(static_cast<float>(visibleRows) / 4096.0f);
	fScrollBar->SetValue(static_cast<float>(fBaseAddress >> 4));

	fUpdatingScrollBar = false;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::ScrollBarChanged
//
// Handles user movement of the CPU memory scrollbar.
//
// Parameters:
//   value - New scrollbar value, in 16-byte rows.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::ScrollBarChanged (float value)
{
	if (fUpdatingScrollBar) {
		return;
	}

	const int32 visibleRows = VisibleMemoryRows();
	int32 maxTopRow = 4096 - visibleRows;

	if (maxTopRow < 0) {
		maxTopRow = 0;
	}

	int32 row = static_cast<int32>(value);

	if (row < 0) {
		row = 0;
	} else if (row > maxTopRow) {
		row = maxTopRow;
	}

	fBaseAddress = static_cast<uint16>(row << 4);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUMemoryView::VisibleMemoryRows
//
// Returns the number of 16-byte memory rows that fit in the memory panel.  This
// is used to keep the scrollbar from positioning the view beyond the end of the
// CPU address space.
//
// Parameters:
//   None.
//
// Returns:
//   Number of visible memory rows.
// -----------------------------------------------------------------------------
int32
CPUMemoryView::VisibleMemoryRows() const
{
	BRect panel(4.0f, 202.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);

	BFont prevFont;
	const_cast<CPUMemoryView *>(this)->GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(10.0f);
	const_cast<CPUMemoryView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<CPUMemoryView *>(this)->GetFontHeight(&fh);
	const_cast<CPUMemoryView *>(this)->SetFont(&prevFont);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	float y = panel.top + 34.0f;
	
	y += lineH + 6.0f;
	y += 4.0f;

	const int32 rows = static_cast<int32>((panel.bottom - y - 8.0f) / lineH);

	if (rows < 1) {
		return 1;
	}

	return rows;
}


// -----------------------------------------------------------------------------
// CPUMemoryView::DrawInstructionTargetInfo
//
// Draws one fixed-width status line describing the current instruction target,
// if the instruction references a useful memory or control-flow address.
//
// Parameters:
//   x - Left position for the target info text.
//   y - Baseline position for the target info text.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawInstructionTargetInfo (float x, float y)
{
	BString s;

	if (!HasROMLoaded()) {
		s.SetTo("Target: none");
		SetHighColor(90, 90, 90);
		DrawString(s.String(), BPoint(x, y));
		return;
	}

	uint16 address = 0x0000;

	if (!CurrentInstructionTarget(address)) {
		s.SetTo("Target: none");
		SetHighColor(90, 90, 90);
		DrawString(s.String(), BPoint(x, y));
		return;
	}

	const uint8 value = nes::bus::debug_read_memory(address);

	BString asciiText;

	if (value >= 32 && value <= 126) {
		asciiText.SetToFormat("'%c'", static_cast<char>(value));
	} else {
		asciiText.SetTo(".");
	}

	s.SetToFormat("Target: $%04X  Hex:$%02X  Dec:%3u  ASCII:%-3s  %s",
					address, value, value, asciiText.String(), RegionLabel(address));

	SetHighColor(0, 100, 0);
	DrawString(s.String(), BPoint(x, y));
}


// -----------------------------------------------------------------------------
// CPUMemoryView::ActiveInspectAddress
//
// Returns the address currently shown in the selected-byte information line.
// A locked byte takes priority over the hovered byte.
//
// Parameters:
//   address - Receives the active inspect address.
//
// Returns:
//   true if there is either a locked or hovered address.
// -----------------------------------------------------------------------------
bool
CPUMemoryView::ActiveInspectAddress (uint16 &address) const
{
	if (fHasLockedAddress) {
		address = fLockedAddress;
		return true;
	}

	if (fHasHoveredAddress) {
		address = fHoveredAddress;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// CPUMemoryView::DrawSelectedByteInfo
//
// Draws one fixed-width status line describing the currently inspected memory
// byte.  A clicked locked byte takes priority over the hovered byte.
//
// Parameters:
//   x - Left position for the selected-byte info text.
//   y - Baseline position for the selected-byte info text.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUMemoryView::DrawSelectedByteInfo (float x, float y)
{
	BString s;

	if (!HasROMLoaded()) {
		s.SetTo("Inspect: none");
		SetHighColor(90, 90, 90);
		DrawString(s.String(), BPoint(x, y));
		return;
	}

	uint16 address = 0x0000;

	if (!ActiveInspectAddress(address)) {
		s.SetTo("Inspect: none");
		SetHighColor(90, 90, 90);
		DrawString(s.String(), BPoint(x, y));
		return;
	}

	const uint8 value = nes::bus::debug_read_memory(address);

	BString asciiText;

	if (value >= 32 && value <= 126) {
		asciiText.SetToFormat("'%c'", static_cast<char>(value));
	} else {
		asciiText.SetTo(".");
	}

	const char *mode = fHasLockedAddress ? "Locked" : "Hover";

	s.SetToFormat("%s:   $%04X  Hex:$%02X  Dec:%3u  ASCII:%-3s  %s",
		mode, address, value, value, asciiText.String(), RegionLabel(address));

	if (fHasLockedAddress) {
		SetHighColor(0, 0, 0);
	} else {
		SetHighColor(70, 70, 70);
	}

	DrawString(s.String(), BPoint(x, y));
}

