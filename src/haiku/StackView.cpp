
#include "StackView.h"


// -----------------------------------------------------------------------------
// StackView::StackView
//
// Creates the Stack debugger view.
//
// Parameters:
//   frame  - View frame.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
StackView::StackView (BRect frame, PretendoWindow *parent)
	: BView(frame, "stack view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE),
		fParent(parent)
{
	(void)fParent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// StackView::~StackView
//
// Destroys the Stack debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
StackView::~StackView()
{
}


// -----------------------------------------------------------------------------
// StackView::AttachedToWindow
//
// Initializes the view after it is attached to a window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	if (HasROMLoaded()) {
		CaptureStackSnapshot();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// StackView::Draw
//
// Draws the complete Stack debugger.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::Draw(BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderUI();

	if (!HasROMLoaded()) {
		BRect panel(4.0f, 88.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
		::DrawDebugPanel(this, panel, "CPU Stack");
		DrawNoROMMessage(panel);
		return;
	}

	DrawStackSummaryPanel();
	DrawStackGrid();

	if (fShowPossibleCallStack) {
		DrawPossibleCallStackPanel();
	} else if (fShowStackHistory) {
		DrawStackHistoryPanel();
	} else {
		DrawSelectedBytePanel();
	}
}


// -----------------------------------------------------------------------------
// StackView::KeyDown
//
// Handles keyboard shortcuts for the CPU Stack debugger.
//
// Controls:
//
//   Arrows - Move stack-byte selection.
//   F      - Toggle Follow-SP.
//   G      - Resume normal emulator execution after a debugger break.
//   H      - Toggle stack activity history.
//   K      - Toggle possible call stack.
//   B      - Toggle SP-threshold break.
//   [ / ]  - Adjust SP break threshold.
//   S      - Toggle stack-wrap break.
//   R      - Refresh stack snapshot.
//   C      - Clear stack history.
//   Space  - Freeze/resume StackView snapshots.
//
// Manual selection disables Follow-SP and switches away from the history and
// possible-call-stack panels so the selected stack byte can be inspected.
//
// Parameters:
//   bytes    - Key bytes supplied by BeAPI.
//   numBytes - Number of bytes in the key sequence.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::KeyDown(const char *bytes, int32 numBytes)
{
	if (!bytes || numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			break;

		case B_LEFT_ARROW:
			if (fHasSelectedAddress) {
				MoveSelection(-1);
			}
			fFollowStackPointer = false;
			fShowStackHistory = false;
			fShowPossibleCallStack = false;
			Invalidate();
			break;

		case B_RIGHT_ARROW:
			if (fHasSelectedAddress) {
				MoveSelection(1);
			}
			fFollowStackPointer = false;
			fShowStackHistory = false;
			fShowPossibleCallStack = false;
			Invalidate();
			break;

		case B_UP_ARROW:
			if (fHasSelectedAddress) {
				MoveSelection(-16);
			}
			fFollowStackPointer = false;
			fShowStackHistory = false;
			fShowPossibleCallStack = false;
			Invalidate();
			break;

		case B_DOWN_ARROW:
			if (fHasSelectedAddress) {
				MoveSelection(16);
			}
			fFollowStackPointer = false;
			fShowStackHistory = false;
			fShowPossibleCallStack = false;
			Invalidate();
			break;

		case 'f':
		case 'F':
			fFollowStackPointer = !fFollowStackPointer;

			if (fFollowStackPointer) {
				fHasSelectedAddress = true;
				fSelectedAddress = StackPointerAddress();
			} else {
				fHasSelectedAddress = false;
			}

			Invalidate(Bounds());
			break;

		case 'g':
		case 'G':
			if (fParent) {
				fParent->DebugResumeExecution();
			}

			Invalidate();
			break;

		case 'h':
		case 'H':
			fShowStackHistory = !fShowStackHistory;
			fShowPossibleCallStack = false;

			Invalidate();
			break;

		case 'k':
		case 'K':
			fShowPossibleCallStack = !fShowPossibleCallStack;
			fShowStackHistory = false;

			Invalidate();
			break;

		case 'b':
		case 'B':
		{
			const bool enabled = !nes::cpu::debug_stack_sp_break_enabled();
			const uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			nes::cpu::debug_set_stack_sp_break(enabled, threshold);

			Invalidate();
			break;
		}

		case '[':
		{
			uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			if (threshold > 0x0) {
				threshold--;
			}

			nes::cpu::debug_set_stack_sp_break(
				nes::cpu::debug_stack_sp_break_enabled(),
				threshold
			);

			Invalidate();
			break;
		}

		case ']':
		{
			uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			if (threshold < 0xff) {
				threshold++;
			}

			nes::cpu::debug_set_stack_sp_break(
				nes::cpu::debug_stack_sp_break_enabled(),
				threshold
			);

			Invalidate();
			break;
		}

		case 's':
		case 'S':
			nes::cpu::debug_set_stack_wrap_break(
				!nes::cpu::debug_stack_wrap_break_enabled()
			);

			Invalidate();
			break;

		case 'r':
		case 'R':
			CaptureStackSnapshot();
			Invalidate();
			break;

		case 'c':
		case 'C':
			ClearStackHistory();
			Invalidate();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// StackView::MouseDown
//
// Selects a stack byte for inspection. Clicking the currently selected byte
// again clears the selection.
//
// Manual selection disables Follow-SP mode and returns the bottom area to the
// selected-byte inspector.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::MouseDown(BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	uint16 address = 0x100;

	if (!AddressForPoint(where, address)) {
		return;
	}

	fFollowStackPointer = false;
	fShowStackHistory = false;
	fShowPossibleCallStack = false;

	if (fHasSelectedAddress && (fSelectedAddress == address)) {
		fHasSelectedAddress = false;
		fSelectedAddress = 0x1ff;
	} else {
		fHasSelectedAddress = true;
		fSelectedAddress = address;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// StackView::Pulse
//
// Periodically refreshes the live stack snapshot and updates the selected
// address when Follow-SP mode is enabled.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	if (!fFreezeUpdates) {
		CaptureStackSnapshot();
	}

	if (fFollowStackPointer) {
		fHasSelectedAddress = true;
		fSelectedAddress = StackPointerAddress();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// StackView::DrawHeaderUI
//
// Draws the Stack debugger controls panel.
//
// The top rows summarize mouse and keyboard controls. The Break row shows the
// current SP-threshold break configuration and stack-wrap break state.
//
// The SP threshold itself is rendered in a fixed-width font so the hexadecimal
// value remains visually stable as it changes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawHeaderUI()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		84.0f
	);

	::DrawDebugPanel(
		this,
		panel,
		"Controls"
	);

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(
		fh.ascent
		+ fh.descent
		+ fh.leading
	) + 1.0f;

	BFont normalFont;
	GetFont(&normalFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 64.0f;
	float y = panel.top + 34.0f;

	auto drawKV = [&](const char *label, const char *value) {
		SetFont(&normalFont);

		SetHighColor(80, 80, 80);
		DrawString(
			label,
			BPoint(labelX, y)
		);

		SetHighColor(35, 35, 35);
		DrawString(
			value,
			BPoint(valueX, y)
		);

		y += lineH;
	};

	drawKV(
		"Mouse:",
		"click/clear   R refresh   C clear"
	);

	drawKV(
		"Keys:",
		"Arrows move   F follow   G resume   H history   K calls"
	);

	/*
	 * Break row.
	 *
	 * Draw the label normally, then use the fixed-width font for the
	 * SP threshold indicator itself.
	 */
	SetFont(&normalFont);

	SetHighColor(80, 80, 80);
	DrawString(
		"Break:",
		BPoint(labelX, y)
	);

	const bool spBreakEnabled = nes::cpu::debug_stack_sp_break_enabled();
	const bool spBreakArmed = nes::cpu::debug_stack_sp_break_armed();

	const char *spBreakState
		= !spBreakEnabled ? "OFF" : (spBreakArmed ? "ARMED" : "HIT");

	BString spText;
	spText.SetToFormat(
		"SP<=$%02X",
		nes::cpu::debug_stack_sp_break_threshold()
	);

	SetFont(&mono);
	SetHighColor(35, 35, 35);

	DrawString(
		spText.String(),
		BPoint(valueX, y)
	);

	const float afterSP = valueX + mono.StringWidth(spText.String());

	/*
	 * Return to the normal UI font for state and help text.
	 */
	SetFont(&normalFont);

	BString remainder;
	remainder.SetToFormat(
		" %s   [ ] threshold   S wrap %s",
		spBreakState,
		nes::cpu::debug_stack_wrap_break_enabled() ? "ON" : "OFF"
	);

	DrawString(
		remainder.String(),
		BPoint(afterSP, y)
	);

	SetFont(&normalFont);
}


// -----------------------------------------------------------------------------
// StackView::DrawStackSummaryPanel
//
// Draws the stack summary panel.
//
// The left column shows the current stack-pointer state and Follow-SP mode.
//
// The right column shows stack usage plus a priority status line:
//
//   1. Stack warning, if one is active.
//   2. The exact SP transition that caused a stack breakpoint.
//   3. Peak observed stack depth otherwise.
//
// For an SP-threshold breakpoint, the transition is displayed as:
//
//   Break: SP $ED -> $EC
//
// For a stack-wrap breakpoint:
//
//   Break: wrap $00 -> $FF
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawStackSummaryPanel()
{
	BRect panel(4.0f, 88.0f, Bounds().right - 4.0f, 164.0f);
	::DrawDebugPanel(this, panel, "Stack Summary");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const uint8 sp = StackPointer();
	const uint16 spAddress = StackPointerAddress();
	const int32 usedBytes = (0xff - sp);
	const int32 freeBytes = (sp + 1);

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 72.0f;
	const float rightLabelX = panel.left + 230.0f;
	const float rightValueX = rightLabelX + 92.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BString s;

	auto drawLeftKV = [&](
		const char *label, const char *value, bool monoValue) {
		SetFont(&prevFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetFont(monoValue ? &mono : &prevFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value, bool monoValue) {
		SetFont(&prevFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetFont(monoValue ? &mono : &prevFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	/*
	 * Left column.
	 */
	s.SetToFormat("$%02X", sp);
	drawLeftKV("SP:", s.String(), true);

	s.SetToFormat("$%04X", spAddress);
	drawLeftKV("SP Addr:", s.String(), true);
	drawLeftKV("Follow:", (fFollowStackPointer ? "SP" : "off"), false);

	/*
	 * Right column.
	 */
	s.SetToFormat("%ld", static_cast<long>(usedBytes));
	drawRightKV("Used:", s.String(), false);

	s.SetToFormat("%ld", static_cast<long>(freeBytes));
	drawRightKV("Free:", s.String(), false);

	/*
	 * The third right-column line is used for the most important current
	 * stack status.
	 */
	const char *warning = StackWarningText();

	if (warning != nullptr) {
		drawRightKV("Warning:", warning, false);
	} else if (nes::cpu::debug_breakpoint_hit()) {
		const nes::cpu::DebugBreakReason reason = nes::cpu::debug_break_reason();

		if (reason == nes::cpu::DEBUG_BREAK_STACK_SP) {
			s.SetToFormat("$%02X -> $%02X", nes::cpu::debug_stack_break_old_s(), nes::cpu::debug_stack_break_new_s());
			drawRightKV("Break: SP", s.String(), true);
		} else if (reason == nes::cpu::DEBUG_BREAK_STACK_WRAP) {
			s.SetToFormat("$%02X -> $%02X", nes::cpu::debug_stack_break_old_s(), nes::cpu::debug_stack_break_new_s());
			drawRightKV("Break: wrap", s.String(), true);
		} else {
			s.SetToFormat("%u", static_cast<unsigned>(fPeakStackDepth));
			drawRightKV("Peak:", s.String(), false);
		}
	} else {
		s.SetToFormat("%u", static_cast<unsigned>(fPeakStackDepth));
		drawRightKV("Peak:", s.String(), false);
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// StackView::DrawStackGrid
//
// Draws the 16x16 stack byte grid. The stack is displayed with high addresses
// at the top and low addresses at the bottom so the downward-growing stack is
// visually intuitive.
//
// Used stack bytes receive a subtle background tint. The current stack pointer,
// changed bytes, and selected byte receive stronger overlays.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawStackGrid()
{
	BRect panel(4.0f, 174.0f, Bounds().right - 4.0f, 448.0f);
	::DrawDebugPanel(this, panel, "Stack Page $0100-$01FF");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	const float rowLabelX = panel.left + 10.0f;
	const float firstCellX = panel.left + 52.0f;
	const float firstCellY = panel.top + 54.0f;

	const float cellW = 27.0f;
	const float cellH = 14.0f;

	BString s;

	SetHighColor(80, 80, 80);

	for (int32 col = 0; col < 16; col++) {
		s.SetToFormat("%X", static_cast<unsigned>(col));
		DrawString(s.String(), BPoint(firstCellX + col * cellW + 6.0f, panel.top + 36.0f));
	}

	const uint16 spAddress = StackPointerAddress();

	for (int32 row = 0; row < 16; row++) {
		const uint16 rowBase = static_cast<uint16>(0x1f0 - row * 16);
		const float rowY = firstCellY + row * cellH;

		s.SetToFormat("$%04X", rowBase);

		SetHighColor(80, 80, 80);
		DrawString(s.String(), BPoint(rowLabelX - 4.0f, rowY));

		for (int32 col = 0; col < 16; col++) {
			const uint16 address = static_cast<uint16>(rowBase + col);
			const uint8 index = static_cast<uint8>(address & 0xff);
			const uint8 value = fBytes[index];

			const float x = firstCellX + col * cellW;
			const float y = rowY;

			BRect cellRect(x - 2.0f, y - 11.0f, x + cellW - 4.0f, y + 3.0f);
			const bool selected = (fHasSelectedAddress) && (fSelectedAddress == address);
			const bool spCell = (address == spAddress);
			const bool usedStackArea = (address > spAddress);

			if (usedStackArea) {
				SetHighColor(238, 238, 238);
				FillRect(cellRect);
			}

			if (fChanged[index]) {
				SetHighColor(255, 245, 170);
				FillRect(cellRect);
			}

			if (spCell) {
				SetHighColor(215, 245, 215);
				FillRect(cellRect);

				SetHighColor(60, 150, 60);
				StrokeRect(cellRect);
			}

			if (selected) {
				if (spCell) {
					SetHighColor(205, 230, 245);
				} else {
					SetHighColor(190, 215, 245);
				}

				FillRect(cellRect);

				SetHighColor(70, 120, 180);
				StrokeRect(cellRect);
			}

			SetHighColor(0, 0, 0);

			s.SetToFormat("%02X", value);
			DrawString(s.String(), BPoint(x, y));
		}

		if (spAddress >= rowBase && spAddress <= static_cast<uint16>(rowBase + 15)) {
			SetHighColor(40, 130, 40);
			DrawString("SP", BPoint(firstCellX + 16.0f * cellW + 4.0f,rowY));
		}
	}

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// StackView::DrawSelectedBytePanel
//
// Draws the selected-byte inspector.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawSelectedBytePanel()
{
	BRect panel(4.0f, 458.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Selected Stack Byte");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const float labelX = panel.left + 10.0f;
	const float valueX = labelX + 92.0f;

	const float col2X = panel.left + 260.0f;
	const float col2ValueX = col2X + 80.0f;

	float y = panel.top + 36.0f;

	auto drawKV = [&](const char *label, const char *value, bool monoValue, float lx, float vx) {
		SetFont(&prevFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(lx, y));

		SetFont(monoValue ? &mono : &prevFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(vx, y));
	};

	if (!fHasSelectedAddress) {
		SetFont(&prevFont);
		SetHighColor(90, 90, 90);
		DrawString("Click a stack byte to inspect it.", BPoint(labelX, y));
		return;
	}

	const uint16 address = fSelectedAddress;
	const uint8 index = static_cast<uint8>(address & 0xff);
	const uint8 value = fBytes[index];

	BString s;

	s.SetToFormat("$%04X", address);
	drawKV("Address:", s.String(), true, labelX, valueX);

	s.SetToFormat("$%02X", value);
	drawKV("Hex:", s.String(), true, col2X, col2ValueX);

	y += lineH;

	s.SetToFormat("%u", static_cast<unsigned>(value));
	drawKV("Unsigned:", s.String(), false, labelX, valueX);

	const int32 signedValue = static_cast<int32>(static_cast<int8>(value));
	s.SetToFormat("%ld", static_cast<long>(signedValue));
	drawKV("Signed:", s.String(), false, col2X, col2ValueX);

	y += lineH;

	BString binary;

	for (int32 bit = 7; bit >= 0; bit--) {
		binary << (((value >> bit) & 0x1) ? "1" : "0");

		if (bit == 4) {
			binary << " ";
		}
	}

	drawKV("Binary:", binary.String(), true, labelX, valueX);
	drawKV("State:", (fChanged[index] ? "changed" : "unchanged"), false, col2X, col2ValueX);

	y += lineH;

	if (address == StackPointerAddress()) {
		drawKV("SP:", "current stack pointer", false, labelX, valueX);
	} else if (address > StackPointerAddress()) {
		drawKV("SP:", "used stack area", false, labelX, valueX);
	} else {
		drawKV("SP:", "free stack area", false, labelX, valueX);
	}

	drawKV("Mode:", (fFreezeUpdates ? "frozen" : "live"), false, col2X, col2ValueX);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// StackView::DrawStackHistoryPanel
//
// Draws recent instruction-aware CPU stack activity, including ordinary
// stack instructions and recognized BRK/IRQ/NMI interrupt frames.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawStackHistoryPanel()
{
	BRect panel(4.0f, 458.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Recent Stack Activity");

	BFont previousFont;
	GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;

	const float x = panel.left + 10.0f;
	float y = panel.top + 35.0f;

	if (fStackHistoryCount <= 0) {
		SetHighColor(90, 90, 90);
		DrawString("No stack activity captured yet.", BPoint(x, y));

		SetHighColor(100, 100, 100);
		DrawString("H: selected-byte panel   C: clear history", BPoint(panel.left + 10.0f, panel.bottom - 10.0f));

		SetFont(&previousFont);
		return;
	}

	const int32 linesToDraw = (fStackHistoryCount < kVisibleBottomPanelLines)
			? fStackHistoryCount
			: kVisibleBottomPanelLines;

	for (int32 line = 0; line < linesToDraw; line++) {
		int32 index = (fStackHistoryNext - 1 - line);

		while (index < 0) {
			index += kStackHistoryCapacity;
		}

		index %= kStackHistoryCapacity;

		const StackActivity &activity = fStackHistory[index];

		BString operation;
		BString detail;
		BString text;

		switch (activity.type) {
			case STACK_ACTIVITY_PHA:
				operation.SetTo("PHA");
				detail.SetToFormat("A=$%02X -> $%04X", activity.value, activity.firstAddress);

				SetHighColor(45, 95, 150);
				break;

			case STACK_ACTIVITY_PHP:
				operation.SetTo("PHP");
				detail.SetToFormat("P=$%02X -> $%04X", activity.value, activity.firstAddress);

				SetHighColor(45, 95, 150);
				break;

			case STACK_ACTIVITY_PLA:
				operation.SetTo("PLA");
				detail.SetToFormat("$%04X -> A=$%02X", activity.firstAddress, activity.resultA);

				SetHighColor(55, 125, 65);
				break;

			case STACK_ACTIVITY_PLP:
				operation.SetTo("PLP");
				detail.SetToFormat("$%04X -> P=$%02X", activity.firstAddress, activity.resultP);

				SetHighColor(55, 125, 65);
				break;

			case STACK_ACTIVITY_JSR:
				operation.SetTo("JSR");
				detail.SetToFormat("$%04X -> $%04X", activity.instructionAddress, activity.targetAddress);

				SetHighColor(55, 85, 155);
				break;

			case STACK_ACTIVITY_RTS:
				operation.SetTo("RTS");
				detail.SetToFormat("resume $%04X", activity.resumeAddress);

				SetHighColor(55, 125, 65);
				break;

			case STACK_ACTIVITY_RTI:
				operation.SetTo("RTI");
				detail.SetToFormat("resume $%04X  P=$%02X", activity.resumeAddress, activity.resultP);

				SetHighColor(120, 80, 145);
				break;

			case STACK_ACTIVITY_NMI:
				operation.SetTo("NMI");

				detail.SetToFormat("handler $%04X  stack $%04X-$%04X", activity.resumeAddress, 
									activity.firstAddress, activity.lastAddress);

				SetHighColor(145, 70, 145);
				break;

			case STACK_ACTIVITY_IRQ:
				operation.SetTo("IRQ");

				detail.SetToFormat("handler $%04X  stack $%04X-$%04X", activity.resumeAddress,
									activity.firstAddress, activity.lastAddress);

				SetHighColor(160, 85, 45);
				break;

			case STACK_ACTIVITY_BRK:
				operation.SetTo("BRK");
				detail.SetToFormat("$%04X -> handler $%04X", activity.instructionAddress, activity.resumeAddress);

				SetHighColor(180, 55, 45);
				break;

			case STACK_ACTIVITY_INTERRUPT:
				operation.SetTo("INT");
				detail.SetToFormat("handler $%04X  stack $%04X-$%04X", activity.resumeAddress,
									activity.firstAddress,
									activity.lastAddress);

				SetHighColor(155, 80, 110);
				break;

			case STACK_ACTIVITY_PUSH:
				if (activity.count == 1) {
					operation.SetTo("PUSH");
					detail.SetToFormat("$%04X = $%02X", activity.firstAddress, activity.value);
				} else {
					operation.SetToFormat("PUSH x%u", static_cast<unsigned>(activity.count));
					detail.SetToFormat("$%04X-$%04X", activity.firstAddress, activity.lastAddress);
				}

				SetHighColor(45, 95, 150);
				break;

			case STACK_ACTIVITY_POP:
				if (activity.count == 1) {
					operation.SetTo("POP");
					detail.SetToFormat("$%04X", activity.firstAddress);
				} else {
					operation.SetToFormat("POP x%u", static_cast<unsigned>(activity.count));
					detail.SetToFormat("$%04X-$%04X", activity.firstAddress, activity.lastAddress);
				}

				SetHighColor(55, 125, 65);
				break;

			case STACK_ACTIVITY_WRAP_DOWN:
				operation.SetTo("WRAP DN");
				detail.SetTo("stack pointer wrapped");

				SetHighColor(180, 45, 35);
				break;

			case STACK_ACTIVITY_WRAP_UP:
				operation.SetTo("WRAP UP");
				detail.SetTo("stack pointer wrapped");

				SetHighColor(180, 45, 35);
				break;

			default:
				operation.SetTo("UNKNOWN");
				detail.SetTo("-");

				SetHighColor(90, 90, 90);
				break;
		}

		text.SetToFormat("#%5llu  %-8s  %-37s  SP $%02X->$%02X", static_cast<unsigned long long>(activity.sequence),
						operation.String(), detail.String(), activity.oldSP, activity.newSP);
		DrawString(text.String(), BPoint(x, y));

		y += lineH;
	}

	SetHighColor(100, 100, 100);
	DrawString("H: selected-byte panel   C: clear history", BPoint(panel.left + 10.0f, panel.bottom - 10.0f));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// StackView::DrawPossibleCallStackPanel
//
// Draws heuristic JSR return-address candidates reconstructed from adjacent
// bytes in the currently used stack area.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawPossibleCallStackPanel()
{
	BRect panel(4.0f, 458.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Possible Call Stack");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;
	const float x = panel.left + 10.0f;
	float y = panel.top + 35.0f;

	CallStackCandidate candidates[kCallStackCandidateCapacity];

	const int32 candidateCount = BuildPossibleCallStack(candidates, kCallStackCandidateCapacity);

	if (candidateCount <= 0) {
		SetFont(&prevFont);
		SetHighColor(90, 90, 90);
		DrawString("No plausible JSR return addresses found.", BPoint(x, y));

		y += lineH;

		SetHighColor(120, 120, 120);
		DrawString("Results are heuristic and depend on the current stack snapshot.", BPoint(x, y));

		SetHighColor(100, 100, 100);
		DrawString("K: selected-byte panel   H: stack history", BPoint(panel.left + 10.0f, panel.bottom - 8.0f));

		SetFont(&prevFont);
		return;
	}

	const int32 linesToDraw = (candidateCount < kVisibleBottomPanelLines)
		? candidateCount
		: kVisibleBottomPanelLines;

	for (int32 i = 0; i < linesToDraw; i++) {
		const CallStackCandidate& candidate = candidates[i];
		const char *confidenceText = (candidate.confidence == CALL_STACK_CONFIDENCE_HIGH) ? "HIGH" : "MED";

		if (candidate.confidence == CALL_STACK_CONFIDENCE_HIGH) {
			SetHighColor(45, 120, 65);
		} else {
			SetHighColor(150, 105, 30);
		}

		BString text;

		text.SetToFormat("$%04X-$%04X  raw $%04X  resume $%04X   JSR $%04X  %s",
						candidate.lowByteAddress,
						candidate.highByteAddress,
						candidate.rawReturnAddress,
						candidate.resumeAddress,
						candidate.callSiteAddress,
						confidenceText);
		DrawString(text.String(), BPoint(x, y));
		
		y += lineH;
	}

	SetFont(&prevFont);
	SetHighColor(100, 100, 100);
	DrawString("K: selected-byte panel   H: stack history", BPoint(panel.left + 10.0f, panel.bottom - 8.0f));
}


// -----------------------------------------------------------------------------
// StackView::DrawNoROMMessage
//
// Draws the no-ROM empty state.
//
// Parameters:
//   panel - Panel bounds.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect the CPU stack.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + panel.Width() * 0.5f;
	const float centerY = panel.top + panel.Height() * 0.5f;

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(centerX - StringWidth(title) * 0.5f, centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - StringWidth(detail) * 0.5f, centerY + fh.ascent + 8.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// StackView::CaptureStackSnapshot
//
// Captures all 256 stack-page bytes, records changed bytes, updates stack
// high-water information, processes instruction-aware stack history, and
// maintains stack-depth warnings.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::CaptureStackSnapshot()
{
	static const uint8 kChangeHoldFrames = 8;
	static const uint8 kWrapWarningHoldFrames = 60;

	const uint8 currentSP = StackPointer();

	UpdateStackHighWater(currentSP);

	for (uint32 i = 0; i < 0x100; i++) {
		fPreviousBytes[i] = fBytes[i];
	}

	for (uint32 i = 0; i < 0x100; i++) {
		const uint16 address = static_cast<uint16>(0x100 + i);
		const uint8 value = nes::bus::debug_read_memory(address);

		fBytes[i] = value;

		if (!fHaveSnapshot) {
			fChangeAge[i] = 0;
			fChanged[i] = false;
			continue;
		}

		if (fPreviousBytes[i] != fBytes[i]) {
			fChangeAge[i] = kChangeHoldFrames;
		} else if (fChangeAge[i] > 0) {
			fChangeAge[i]--;
		}

		fChanged[i] = fChangeAge[i] > 0;
	}

	CaptureInstructionStackHistory();

	if (fHasPreviousStackPointer) {
		const int32 rawDelta = (static_cast<int32>(currentSP) - static_cast<int32>(fPreviousStackPointer));

		if (rawDelta < -128 || rawDelta > 128) {
			fStackWrapDetected = true;
		}
	}

	if (fStackWrapDetected) {
		fStackWrapWarningAge = kWrapWarningHoldFrames;
		fStackWrapDetected = false;
	} else if (fStackWrapWarningAge > 0) {
		fStackWrapWarningAge--;
	}

	fPreviousStackPointer = currentSP;
	fHasPreviousStackPointer = true;
	fHaveSnapshot = true;
}


// -----------------------------------------------------------------------------
// StackView::ClearStackHistory
//
// Clears all recorded stack activity and remembered wrap warnings.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::ClearStackHistory()
{
	for (int32 i = 0; i < kStackHistoryCapacity; i++) {
		fStackHistory[i] = StackActivity();
	}

	fStackHistoryCount = 0;
	fStackHistoryNext = 0;
	fStackActivitySequence = 0;

	fStackWrapDetected = false;
	fStackWrapWarningAge = 0;
}


// -----------------------------------------------------------------------------
// StackView::BuildPossibleCallStack
//
// Scans adjacent byte pairs in the currently used stack area for plausible
// 6502 JSR return addresses.
//
// A 6502 JSR pushes the address of the final byte of the JSR instruction.
// The low byte appears at the lower stack address and the high byte appears at
// the next higher stack address. RTS restores this value and then increments
// it, so execution resumes at rawReturnAddress + 1.
//
// Candidates are accepted only when the reconstructed call-site address
// contains a JSR opcode ($20). A candidate is assigned high confidence when
// that call-site address also appears in the retained execution trace.
//
// This remains heuristic: ordinary stack data can coincidentally resemble a
// valid return address.
//
// Parameters:
//   candidates - Receives reconstructed call-stack candidates.
//   capacity   - Maximum number of candidates that may be written.
//
// Returns:
//   Number of candidates written.
// -----------------------------------------------------------------------------
int32
StackView::BuildPossibleCallStack(CallStackCandidate *candidates, int32 capacity) const
{
	if (!candidates || capacity <= 0) {
		return 0;
	}

	const uint8 sp = StackPointer();

	/*
	 * Addresses above the current stack pointer are the currently used
	 * portion of the downward-growing 6502 stack.
	 */
	const uint16 firstUsedAddress = static_cast<uint16>(0x100 + sp + 1);

	int32 candidateCount = 0;

	/*
	 * A candidate requires two adjacent bytes, so the lower address may be
	 * no greater than $01FE.
	 */
	for (uint16 lowAddress = firstUsedAddress; lowAddress <= 0x1fe && candidateCount < capacity; lowAddress++) {
		const uint16 highAddress = static_cast<uint16>(lowAddress + 1);
		const uint8 lowIndex = static_cast<uint8>(lowAddress & 0xff);
		const uint8 highIndex = static_cast<uint8>(highAddress & 0xff);

		const uint16 rawReturnAddress = static_cast<uint16>(fBytes[lowIndex]
			| (static_cast<uint16>(fBytes[highIndex]) << 8));

		/*
		 * raw $FFFF would wrap the resume address to $0000. It is not a
		 * useful normal JSR candidate.
		 */
		if (rawReturnAddress == 0xffff) {
			continue;
		}

		/*
		 * The stacked value is the address of the final operand byte of the
		 * three-byte JSR instruction. The opcode is two bytes earlier.
		 */
		if (rawReturnAddress < 2) {
			continue;
		}

		const uint16 callSiteAddress = static_cast<uint16>(rawReturnAddress - 2);
		const uint8 opcode = nes::bus::debug_read_memory(callSiteAddress);

		if (opcode != 0x20) { // JSR
			continue;
		}

		CallStackCandidate &candidate = candidates[candidateCount];
		candidate.lowByteAddress = lowAddress;
		candidate.highByteAddress = highAddress;
		candidate.rawReturnAddress = rawReturnAddress;
		candidate.resumeAddress = static_cast<uint16>(rawReturnAddress + 1);
		candidate.callSiteAddress = callSiteAddress;

		candidate.confidence = nes::cpu::debug_instruction_was_executed(callSiteAddress)
			? CALL_STACK_CONFIDENCE_HIGH
			: CALL_STACK_CONFIDENCE_MEDIUM;

		candidateCount++;
	}

	return candidateCount;
}


// -----------------------------------------------------------------------------
// StackView::StackWarningText
//
// Returns a warning string for suspicious stack conditions.
//
// Parameters:
//   None.
//
// Returns:
//   Warning text, or nullptr when no warning is active.
// -----------------------------------------------------------------------------
const char*
StackView::StackWarningText() const
{
	const uint8 sp = StackPointer();

	if (fStackWrapWarningAge > 0) {
		return "SP wrapped";
	}

	if (sp <= 0x7) {
		return "critical";
	}

	if (sp <= 0x1f) {
		return "nearly full";
	}

	return nullptr;
}


// -----------------------------------------------------------------------------
// StackView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded
// -----------------------------------------------------------------------------
bool
StackView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// StackView::AddressForPoint
//
// Converts a mouse position over the stack grid into the corresponding
// absolute stack-page address.
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the matching address in the range $0100-$01FF.
//
// Returns:
//   true if the point is inside a stack byte cell; false otherwise.
// -----------------------------------------------------------------------------
bool
StackView::AddressForPoint(BPoint where, uint16& address) const
{
	BRect panel(4.0f, 174.0f, Bounds().right - 4.0f, 448.0f);

	if (!panel.Contains(where)) {
		return false;
	}

	const float firstCellX = panel.left + 52.0f;
	const float firstCellY = panel.top + 54.0f;

	const float cellW = 27.0f;
	const float cellH = 14.0f;

	const float gridLeft = firstCellX - 2.0f;
	const float gridTop = firstCellY - 11.0f;
	const float gridRight = gridLeft + 16.0f * cellW;
	const float gridBottom = gridTop + 16.0f * cellH;

	if (where.x < gridLeft || where.x >= gridRight || where.y < gridTop || where.y >= gridBottom) {
		return false;
	}

	const int32 col = static_cast<int32>((where.x - gridLeft) / cellW);
	const int32 row = static_cast<int32>((where.y - gridTop) / cellH);

	if (col < 0 || col >= 16 || row < 0 || row >= 16) {
		return false;
	}

	const uint16 rowBase = static_cast<uint16>(0x1f0 - row * 16);
	address = static_cast<uint16>(rowBase + col);
	
	return true;
}


// -----------------------------------------------------------------------------
// StackView::MoveSelection
//
// Moves the selected stack address by a signed delta.
//
// Parameters:
//   delta - Signed movement amount.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::MoveSelection(int32 delta)
{
	int32 address = fHasSelectedAddress ? static_cast<int32>(fSelectedAddress) 
										: static_cast<int32>(StackPointerAddress());

	address += delta;

	if (address < 0x100) {
		address = 0x100;
	}

	if (address > 0x1ff) {
		address = 0x1ff;
	}

	fHasSelectedAddress = true;
	fSelectedAddress = static_cast<uint16>(address);

	Invalidate();
}


// -----------------------------------------------------------------------------
// StackView::ChangedByteCount
//
// Counts how many stack-page bytes are currently marked as recently changed.
//
// Parameters:
//   None.
//
// Returns:
//   Number of highlighted changed bytes.
// -----------------------------------------------------------------------------
int32
StackView::ChangedByteCount() const
{
	int32 count = 0;

	for (uint32 i = 0; i < 0x100; i++) {
		if (fChanged[i]) {
			count++;
		}
	}

	return count;
}


// -----------------------------------------------------------------------------
// StackView::StackPointer
//
// Returns the current CPU stack pointer register.
//
// Parameters:
//   None.
//
// Returns:
//   CPU S register.
// -----------------------------------------------------------------------------
uint8
StackView::StackPointer() const
{
	return nes::cpu::debug_s();
}


// -----------------------------------------------------------------------------
// StackView::StackPointerAddress
//
// Returns the current absolute stack address represented by the CPU stack
// pointer register.
//
// Parameters:
//   None.
//
// Returns:
//   Stack address $0100 + S.
// -----------------------------------------------------------------------------
uint16
StackView::StackPointerAddress() const
{
	return static_cast<uint16>(0x100 + StackPointer());
}


// -----------------------------------------------------------------------------
// StackView::UpdateStackHighWater
//
// Tracks maximum observed stack depth relative to the highest normal stack
// pointer observed while the debugger is running.
//
// Because the 6502 stack grows downward, smaller S values represent deeper
// stack usage.
//
// If a higher S value is later observed, it becomes the new baseline. This is
// important during startup/reset, where the debugger may briefly observe an
// uninitialized or transitional S value such as $00.
//
// Peak depth is stored independently, so discovering a new higher baseline
// does not erase a legitimate previously observed peak.
//
// Parameters:
//   sp - Current CPU stack pointer.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::UpdateStackHighWater(uint8 sp)
{
	if (!fHaveStackHighWater) {
		fStackBaselinePointer = sp;
		fLowestStackPointer = sp;
		fPeakStackDepth = 0;
		fHaveStackHighWater = true;
		return;
	}

	/*
	 * A higher SP represents a shallower stack and therefore a better
	 * candidate for the normal/base stack level.
	 *
	 * Start a new depth-measurement range from this baseline, but preserve
	 * any peak depth already recorded.
	 */
	if (sp > fStackBaselinePointer) {
		fStackBaselinePointer = sp;
		fLowestStackPointer = sp;
		return;
	}

	if (sp < fLowestStackPointer) {
		fLowestStackPointer = sp;
	}

	const uint16 depth = static_cast<uint16>(fStackBaselinePointer - sp);

	if (depth > fPeakStackDepth) {
		fPeakStackDepth = depth;
	}
}


// -----------------------------------------------------------------------------
// StackView::CaptureInstructionStackHistory
//
// Examines newly retained CPU trace entries and records instruction-driven
// stack activity and interrupt-entry stack frames.
//
// Each trace entry represents CPU state at the start of its instruction:
//
//   entry[i]     = state before instruction i
//   entry[i + 1] = state after instruction i and any interrupt taken before
//                  the following instruction begins.
//
// This allows ordinary stack instructions and asynchronous IRQ/NMI entry to be
// recognized from the same chronological trace.
//
// The CPU execution-cycle counter may restart when the CPU is reset. If the
// retained trace now contains cycle values earlier than the last cycle processed
// by StackView, the trace is treated as belonging to a new execution epoch and
// the StackView processing cursor is restarted.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::CaptureInstructionStackHistory()
{
	const uint32 traceCount = nes::cpu::debug_cpu_trace_count();

	if (traceCount < 2) {
		return;
	}

	/*
	 * Detect a CPU reset or other trace restart.
	 *
	 * The CPU cycle counter is reset when the CPU is reset. StackView's
	 * fLastProcessedTraceCycle, however, survives for as long as the view
	 * remains open. Without detecting the new trace epoch here, every new
	 * post-reset entry could appear older than the last entry processed
	 * before reset and would therefore be ignored.
	 */
	if (fHaveProcessedTraceCycle) {
		nes::cpu::cpu_trace_entry_t newestEntry;

		if (nes::cpu::debug_cpu_trace_entry(traceCount - 1, newestEntry)) {
			if (newestEntry.cycle
				< fLastProcessedTraceCycle) {
				fHaveProcessedTraceCycle = false;
				fLastProcessedTraceCycle = 0;
			}
		}
	}

	for (uint32 i = 0; i + 1 < traceCount; i++) {
		nes::cpu::cpu_trace_entry_t entry;
		nes::cpu::cpu_trace_entry_t nextEntry;

		if (!nes::cpu::debug_cpu_trace_entry(i, entry)) {
			continue;
		}

		if (!nes::cpu::debug_cpu_trace_entry(i + 1, nextEntry)) {
			continue;
		}

		/*
		 * Skip trace transitions StackView has already processed.
		 */
		if (fHaveProcessedTraceCycle && (entry.cycle <= fLastProcessedTraceCycle)) {
			continue;
		}

		/*
		 * First recognize normal opcode-driven stack operations.
		 */
		RecordInstructionStackActivity(entry, nextEntry);

		/*
		 * Then check whether the transition into the following traced
		 * instruction represents BRK, IRQ, or NMI entry.
		 */
		RecordInterruptStackActivity(entry, nextEntry);
		
		fLastProcessedTraceCycle = entry.cycle;
		fHaveProcessedTraceCycle = true;
	}
}


// -----------------------------------------------------------------------------
// StackView::RecordInterruptStackActivity
//
// Detects a 6502 interrupt-entry stack frame between two chronological CPU
// trace entries.
//
// BRK, IRQ, and NMI each push three bytes:
//
//   PC high
//   PC low
//   processor status
//
// causing S to decrease by three.
//
// BRK is identified directly from opcode $00. Hardware interrupt entry is
// identified by comparing the next executed PC with the current NMI and IRQ
// vector targets.
//
// If both vectors currently point to the same address, the event is labeled
// as a generic interrupt because IRQ and NMI cannot be distinguished from the
// trace transition alone.
//
// Parameters:
//   entry     - CPU state before the previous instruction executes.
//   nextEntry - CPU state at the beginning of the next traced instruction.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::RecordInterruptStackActivity(const nes::cpu::cpu_trace_entry_t &entry,
	const nes::cpu::cpu_trace_entry_t &nextEntry)
{
	/*
	 * All normal 6502 interrupt-entry frames consume three stack bytes.
	 */
	const uint8 expectedInterruptSP = static_cast<uint8>(entry.s - 3);

	if (nextEntry.s != expectedInterruptSP) {
		return;
	}

	const uint16 nmiVector = static_cast<uint16>(nes::bus::debug_read_memory(0xfffa) |
		 					(static_cast<uint16>(nes::bus::debug_read_memory(0xfffb)) << 8));

	const uint16 irqVector = static_cast<uint16>(nes::bus::debug_read_memory(0xfffe) |
							(static_cast<uint16>(nes::bus::debug_read_memory(0xffff)) << 8));

	StackActivity activity;

	activity.oldSP = entry.s;
	activity.newSP = nextEntry.s;

	activity.instructionAddress = entry.pc;
	activity.opcode = entry.bytes[0];

	activity.resumeAddress = nextEntry.pc;

	activity.count = 3;

	/*
	 * Interrupt entry writes to:
	 *
	 *   $0100 + S
	 *   $0100 + S - 1
	 *   $0100 + S - 2
	 *
	 * List them in ascending address order.
	 */
	activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s - 2));
	activity.lastAddress = static_cast<uint16>(0x100 + entry.s);

	/*
	 * BRK is explicit and therefore takes priority over vector matching.
	 */
	if (entry.bytes[0] == 0x00) {
		activity.type = STACK_ACTIVITY_BRK;

	/*
	 * Identical vectors make IRQ/NMI indistinguishable here.
	 */
	} else if (nmiVector == irqVector && nextEntry.pc == nmiVector) {
		activity.type = STACK_ACTIVITY_INTERRUPT;

	} else if (nextEntry.pc == nmiVector) {
		activity.type = STACK_ACTIVITY_NMI;

	} else if (nextEntry.pc == irqVector) {
		activity.type = STACK_ACTIVITY_IRQ;
	} else {
		/*
		 * A three-byte SP movement by itself is not enough evidence. TXS,
		 * unusual execution flow, or another event could have changed S.
		 */
		return;
	}

	activity.sequence = ++fStackActivitySequence;

	fStackHistory[fStackHistoryNext] = activity;

	fStackHistoryNext = (fStackHistoryNext + 1) % kStackHistoryCapacity;

	if (fStackHistoryCount < kStackHistoryCapacity) {
		fStackHistoryCount++;
	}
}


// -----------------------------------------------------------------------------
// StackView::RecordInstructionStackActivity
//
// Records a stack-affecting CPU instruction using two chronological CPU trace
// entries.
//
// entry contains CPU state immediately before the instruction.
// nextEntry contains CPU state at the start of the following instruction,
// which represents the completed result of entry's instruction.
//
// Recognized instructions:
//
//   $48  PHA
//   $08  PHP
//   $68  PLA
//   $28  PLP
//   $20  JSR
//   $60  RTS
//   $40  RTI
//
// Parameters:
//   entry     - CPU state before the instruction.
//   nextEntry - CPU state after the instruction.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
StackView::RecordInstructionStackActivity(const nes::cpu::cpu_trace_entry_t &entry,
										 const nes::cpu::cpu_trace_entry_t &nextEntry)
{
	StackActivity activity;

	activity.oldSP = entry.s;
	activity.newSP = nextEntry.s;

	activity.instructionAddress = entry.pc;
	activity.opcode = entry.bytes[0];

	activity.resumeAddress = nextEntry.pc;
	activity.resultA = nextEntry.a;
	activity.resultP = nextEntry.p;

	bool recognized = true;

	switch (entry.bytes[0]) {
		case 0x48:	// PHA
			activity.type = STACK_ACTIVITY_PHA;
			activity.count = 1;

			activity.firstAddress = static_cast<uint16>(0x100 + entry.s);
			activity.lastAddress = activity.firstAddress;
			activity.value = entry.a;
			break;

		case 0x08:	// PHP
			activity.type = STACK_ACTIVITY_PHP;
			activity.count = 1;

			activity.firstAddress = static_cast<uint16>(0x100 + entry.s);
			activity.lastAddress = activity.firstAddress;
			activity.value = entry.p;
			break;

		case 0x68:	// PLA
			activity.type = STACK_ACTIVITY_PLA;
			activity.count = 1;

			activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s + 1));
			activity.lastAddress = activity.firstAddress;
			break;

		case 0x28:	// PLP
			activity.type = STACK_ACTIVITY_PLP;
			activity.count = 1;

			activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s + 1));
			activity.lastAddress = activity.firstAddress;

			break;

		case 0x20:	// JSR
			activity.type = STACK_ACTIVITY_JSR;
			activity.count = 2;

			activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s - 1));
			activity.lastAddress = static_cast<uint16>(0x100 + entry.s);
			activity.targetAddress = static_cast<uint16>(entry.bytes[1] | 
									(static_cast<uint16>(entry.bytes[2]) << 8));

			break;

		case 0x60:	// RTS
			activity.type = STACK_ACTIVITY_RTS;
			activity.count = 2;

			activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s + 1));
			activity.lastAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s + 2));
			break;

		case 0x40:	// RTI
			activity.type = STACK_ACTIVITY_RTI;
			activity.count = 3;

			activity.firstAddress = static_cast<uint16>(0x100 + static_cast<uint8>(entry.s + 1));
			activity.lastAddress = static_cast<uint16>(0x0100 + static_cast<uint8>(entry.s + 3));
			break;

		default:
			recognized = false;
			break;
	}

	if (!recognized) {
		return;
	}

	/*
	 * Verify the SP transition before recording the event.
	 *
	 * uint8 arithmetic intentionally preserves normal 6502 stack wrapping.
	 */
	uint8 expectedSP = entry.s;

	switch (activity.type) {
		case STACK_ACTIVITY_PHA:
		case STACK_ACTIVITY_PHP:
			expectedSP = static_cast<uint8>(entry.s - 1);
			break;

		case STACK_ACTIVITY_PLA:
		case STACK_ACTIVITY_PLP:
			expectedSP = static_cast<uint8>(entry.s + 1);
			break;

		case STACK_ACTIVITY_JSR:
			expectedSP = static_cast<uint8>(entry.s - 2);
			break;

		case STACK_ACTIVITY_RTS:
			expectedSP = static_cast<uint8>(entry.s + 2);
			break;

		case STACK_ACTIVITY_RTI:
			expectedSP = static_cast<uint8>(entry.s + 3);
			break;

		default:
			break;
	}

	if (nextEntry.s != expectedSP) {
		/*
		 * An interrupt or other asynchronous stack event may have occurred
		 * between the two traced instruction boundaries. Do not attribute
		 * that unexplained SP movement to this instruction.
		 */
		return;
	}

	activity.sequence = ++fStackActivitySequence;

	fStackHistory[fStackHistoryNext] = activity;
	fStackHistoryNext = (fStackHistoryNext + 1) % kStackHistoryCapacity;

	if (fStackHistoryCount < kStackHistoryCapacity) {
		fStackHistoryCount++;
	}
}


