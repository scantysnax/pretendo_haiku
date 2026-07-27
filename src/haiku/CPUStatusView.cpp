
#include "CPUStatusView.h"

#include "Cart.h"
#include "Cpu.h"
#include "CPUDisasm.h"
#include "DebugHelpers.h"
#include "Ppu.h"
#include "PretendoWindow.h"

#include <cmath>


static void
SetStateColor (BView *view, bool active)
{
	if (active) {
		view->SetHighColor(0, 120, 0);
	} else {
		view->SetHighColor(120, 120, 120);
	}
}


CPUStatusView::CPUStatusView (BRect frame, PretendoWindow *parent)
	: BView(frame, "cpu_status_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


CPUStatusView::~CPUStatusView()
{
}


void
CPUStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
}


// -----------------------------------------------------------------------------
// CPUStatusView::Pulse
//
// Refreshes the CPU status viewer while a ROM is loaded.  If no ROM is loaded,
// the empty-state view is static and does not need continuous redraws.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// CPUStatusView::Draw
//
// Draws the CPU status viewer.  If no ROM is loaded, the header remains visible
// and the body shows a friendly empty-state message instead of stale/default CPU
// state.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(
			4.0f,
			74.0f,
			Bounds().right - 4.0f,
			Bounds().bottom - 8.0f
		);

		::DrawDebugPanel(this, panel, "CPU State");
		DrawNoROMMessage(panel);
		return;
	}

	DrawRegisterPanel();
	DrawFlagsPanel();
	DrawTimingPanel();
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawHeaderPanel
//
// Draws the title/help panel for the CPU status debugger.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawHeaderPanel()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		62.0f
	);

	::DrawDebugPanel(this, panel, "CPU Status");

	SetFontSize(11.0f);

	SetHighColor(35, 35, 35);
	DrawString(
		"Live 6502 register, flag, instruction, and cycle state.",
		BPoint(panel.left + 8.0f, panel.top + 40.0f)
	);
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawRegisterPanel
//
// Draws the core 6502 public registers: PC, A, X, Y, S, and P.  The stack
// pointer is also shown as a full CPU address in page $0100 so it can be checked
// directly against the CPU Memory window's stack page.
//
// The processor status byte is additionally decoded into a compact flag string
// using the common 6502 order:
//
//   N V - B D I Z C
//
// Uppercase letters indicate set flags, lowercase letters indicate clear flags,
// and '-' represents the unused/reserved status bit.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawRegisterPanel()
{
	BRect panel(
		4.0f,
		74.0f,
		Bounds().right - 4.0f,
		168.0f
	);

	::DrawDebugPanel(this, panel, "Registers");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 52.0f;

	const float rightLabelX = panel.left + 180.0f;
	const float rightValueX = rightLabelX + 52.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BString s;
	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	auto drawLeftKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(&mono);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(&mono);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	s.SetToFormat("$%04X", state.pc);
	drawLeftKV("PC:", s.String());

	s.SetToFormat("$%02X", state.a);
	drawLeftKV("A:", s.String());

	s.SetToFormat("$%02X", state.x);
	drawLeftKV("X:", s.String());

	const uint16 stackAddress = static_cast<uint16>(0x0100 | state.s);

	s.SetToFormat("$%04X", stackAddress);
	drawLeftKV("Stack:", s.String());

	s.SetToFormat("$%02X", state.y);
	drawRightKV("Y:", s.String());

	s.SetToFormat("$%02X", state.s);
	drawRightKV("S:", s.String());

	s.SetToFormat("$%02X", state.p);
	drawRightKV("P:", s.String());

	const uint8 p = state.p;

	BString flags;
	flags.SetToFormat(
		"%c%c-%c%c%c%c%c",
		(p & 0x80) ? 'N' : 'n',
		(p & 0x40) ? 'V' : 'v',
		(p & 0x10) ? 'B' : 'b',
		(p & 0x08) ? 'D' : 'd',
		(p & 0x04) ? 'I' : 'i',
		(p & 0x02) ? 'Z' : 'z',
		(p & 0x01) ? 'C' : 'c'
	);

	drawRightKV("Flags:", flags.String());

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawFlagsPanel
//
// Draws the decoded 6502 processor status flags.  Active flags are shown in
// green; inactive flags are shown in gray.  The unused/reserved status bit is
// displayed as '-' to match common 6502 debugger notation:
//
//   N V - B D I Z C
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawFlagsPanel()
{
	BRect panel(
		4.0f,
		180.0f,
		Bounds().right - 4.0f,
		324.0f
	);

	::DrawDebugPanel(this, panel, "Processor Flags");

	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	const uint8 p = state.p;

	const bool n = (p & 0x80) != 0;
	const bool v = (p & 0x40) != 0;
	const bool r = (p & 0x20) != 0;
	const bool b = (p & 0x10) != 0;
	const bool d = (p & 0x08) != 0;
	const bool i = (p & 0x04) != 0;
	const bool z = (p & 0x02) != 0;
	const bool c = (p & 0x01) != 0;

	const float boxW = 38.0f;
	const float boxH = 32.0f;
	const float gap = 8.0f;

	const float totalFlagW = (boxW * 8.0f) + (gap * 7.0f);
	float x = panel.left + ((panel.Width() - totalFlagW) * 0.5f);
	const float y = panel.top + 36.0f;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "N", n);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "V", v);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "-", r);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "B", b);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "D", d);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "I", i);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "Z", z);
	x += boxW + gap;

	DrawFlag(BRect(x, y, x + boxW, y + boxH), "C", c);

	SetFontSize(10.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftX = panel.left + 24.0f;
	const float midX = panel.left + 196.0f;
	float legendY = y + boxH + 22.0f;

	auto drawLegend = [&](float labelX, float textX, const char* flag,
		const char* text, bool active) {
		SetStateColor(this, active);
		DrawString(flag, BPoint(labelX, legendY));

		SetHighColor(80, 80, 80);
		DrawString(text, BPoint(textX, legendY));
	};

	drawLegend(leftX, leftX + 18.0f, "N", "Negative", n);
	drawLegend(midX, midX + 18.0f, "D", "Decimal", d);
	legendY += lineH;

	drawLegend(leftX, leftX + 18.0f, "V", "Overflow", v);
	drawLegend(midX, midX + 18.0f, "I", "IRQ Disable", i);
	legendY += lineH;

	drawLegend(leftX, leftX + 18.0f, "-", "Reserved", r);
	drawLegend(midX, midX + 18.0f, "Z", "Zero", z);
	legendY += lineH;

	drawLegend(leftX, leftX + 18.0f, "B", "Break", b);
	drawLegend(midX, midX + 18.0f, "C", "Carry", c);
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawTimingPanel
//
// Draws the current decoded instruction, emulator run/pause state, instruction
// boundary state, stack preview, current instruction latch, current instruction
// cycle, and total executed CPU cycles.
//
// The 6502 stack pointer points to the next free stack slot.  The byte most
// recently pushed is normally at SP + 1 within page $0100, so the stack preview
// shows the top stack address and nearby stack bytes above SP.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawTimingPanel()
{
	BRect panel(
		4.0f,
		336.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Instruction / Timing");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono = *be_fixed_font;
	mono.SetSize(11.0f);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 108.0f;

	float y = panel.top + 38.0f;

	BString s;
	nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

	auto drawKV = [&](const char* label, const char* value) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, y));

		SetHighColor(0, 0, 0);
		SetFont(&mono);
		DrawString(value, BPoint(leftValueX, y));

		y += lineH;
	};

	CPUDisasmLine line = DisassembleCPU(state.pc);

	BString byteText;

	if (line.length == 1) {
		byteText.SetToFormat(
			"%02X      ",
			line.bytes[0]
		);
	} else if (line.length == 2) {
		byteText.SetToFormat(
			"%02X %02X   ",
			line.bytes[0],
			line.bytes[1]
		);
	} else {
		byteText.SetToFormat(
			"%02X %02X %02X",
			line.bytes[0],
			line.bytes[1],
			line.bytes[2]
		);
	}

	s.SetToFormat(
		"$%04X  %-8s  %s",
		line.address,
		byteText.String(),
		line.text.String()
	);
	drawKV("Decoded:", s.String());

	if (nes::ppu::system_paused) {
		drawKV("Emulator:", "Paused");
	} else {
		drawKV("Emulator:", "Running");
	}

	if (nes::cpu::debug_instruction_boundary()) {
		drawKV("Boundary:", "Yes");
	} else {
		drawKV("Boundary:", "No");
	}

	const uint8 stackIndex0 = static_cast<uint8>(state.s + 1);
	const uint8 stackIndex1 = static_cast<uint8>(state.s + 2);
	const uint8 stackIndex2 = static_cast<uint8>(state.s + 3);
	const uint8 stackIndex3 = static_cast<uint8>(state.s + 4);

	const uint16 stackAddress0 = static_cast<uint16>(0x0100 | stackIndex0);
	const uint16 stackAddress1 = static_cast<uint16>(0x0100 | stackIndex1);
	const uint16 stackAddress2 = static_cast<uint16>(0x0100 | stackIndex2);
	const uint16 stackAddress3 = static_cast<uint16>(0x0100 | stackIndex3);

	const uint8 stackValue0 = nes::bus::debug_read_memory(stackAddress0);
	const uint8 stackValue1 = nes::bus::debug_read_memory(stackAddress1);
	const uint8 stackValue2 = nes::bus::debug_read_memory(stackAddress2);
	const uint8 stackValue3 = nes::bus::debug_read_memory(stackAddress3);

	s.SetToFormat(
		"Top $%04X:$%02X  +1:$%02X  +2:$%02X  +3:$%02X",
		stackAddress0,
		stackValue0,
		stackValue1,
		stackValue2,
		stackValue3
	);
	drawKV("Stack:", s.String());

	s.SetToFormat("$%02X", state.instruction & 0xff);
	drawKV("Instruction:", s.String());

	s.SetToFormat("%d", state.cycle);
	drawKV("Cycle:", s.String());

	s.SetToFormat("%llu", static_cast<unsigned long long>(state.executed_cycles));
	drawKV("Total Cycles:", s.String());

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawFlag
//
// Draws one processor status flag box.
//
// Parameters:
//   rect   - Flag box rectangle.
//   name   - Single-character flag name.
//   active - Whether the flag is currently set.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawFlag (BRect rect, const char *name, bool active)
{
	SetHighColor(245, 245, 245);
	FillRect(rect);

	SetStateColor(this, active);
	StrokeRect(rect);

	SetFontSize(14.0f);
	SetStateColor(this, active);

	float textW = StringWidth(name);
	float x = rect.left + ((rect.Width() - textW) * 0.5f);
	float y = rect.top + 22.0f;

	DrawString(name, BPoint(x, y));

	SetFontSize(9.0f);
	SetHighColor(80, 80, 80);
	DrawString(active ? "1" : "0", BPoint(rect.left + 4.0f, rect.bottom - 5.0f));
}


// -----------------------------------------------------------------------------
// CPUStatusView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM/mapper is currently loaded.
// -----------------------------------------------------------------------------
bool
CPUStatusView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// CPUStatusView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the CPU Status window is opened
// without a loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUStatusView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect CPU state.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80);
	DrawString(
		title,
		BPoint(
			centerX - (StringWidth(title) * 0.5f),
			centerY - 8.0f
		)
	);

	SetHighColor(120, 120, 120);
	DrawString(
		detail,
		BPoint(
			centerX - (StringWidth(detail) * 0.5f),
			centerY + fh.ascent + 8.0f
		)
	);

	SetFont(&prevFont);
}

