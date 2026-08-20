
#include "BreakPointView.h"

#include <cmath>

#include "Cart.h"
#include "Cpu.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// BreakPointView::BreakPointView
//
// Creates the BreakPoint Manager debugger view.
//
// Parameters:
//   frame  - View frame.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
BreakPointView::BreakPointView (BRect frame, PretendoWindow *parent)
	: BView(frame, "breakpoint view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE),
	
	fParent(parent)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// BreakPointView::~BreakPointView
//
// Destroys the BreakPoint Manager debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
BreakPointView::~BreakPointView()
{
}


// -----------------------------------------------------------------------------
// BreakPointView::AttachedToWindow
//
// Initializes the view after attachment to its window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);
	Invalidate();
}


// -----------------------------------------------------------------------------
// BreakPointView::Draw
//
// Draws the complete BreakPoint Manager.
//
// Parameters:
//   updateRect - Dirty region supplied by BeAPI.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderPanel();

	if (!HasROMLoaded()) {
		BRect panel(4.0f, 76.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
		::DrawDebugPanel(this, panel, "Breakpoints");

		DrawNoROMMessage(panel);
		return;
	}

	DrawConditionPanel();
	DrawReadWatchPointPanel();
	DrawWriteWatchPointPanel();
	DrawExecuteBreakPointPanel();
}


// -----------------------------------------------------------------------------
// BreakPointView::Pulse
//
// Refreshes Breakpoint state while a ROM is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// BreakPointView::KeyDown
//
// Handles keyboard controls for the BreakPoint Manager.
//
// Normal controls:
//
//   R                - Begin entering a READ-watchpoint address.
//   W                - Begin entering a WRITE-watchpoint address.
//   E                - Begin entering an execute-BreakPoint address.
//   Up/Down          - Move selection through READ, WRITE, and execute
//                      debugger conditions.
//   Delete/Backspace - Remove the selected BreakPoint/watchpoint.
//   C                - Clear all execute, READ, and WRITE conditions.
//   H                - Reset all execute, READ, and WRITE hit counts.
//   B                - Toggle the stack SP-threshold BreakPoint.
//   [ / ]            - Adjust the stack SP-threshold value.
//   S                - Toggle stack-wrap breaking.
//   G                - Resume execution after a debugger break.
//   D / Enter        - Open the CPU disassembler. If an execute BreakPoint is
//                      selected, jump to that instruction. If the selected
//                      READ/WRITE watchpoint caused the current debugger stop,
//                      jump to the instruction responsible for the access.
//                      Otherwise jump to the current CPU PC, or to the RESET
//                      vector target while CPU reset is still in progress.
//
// Address-entry controls:
//
//   0-9 / A-F        - Append one hexadecimal address digit.
//   Backspace        - Remove the last entered digit.
//   Enter            - Add the entered BreakPoint/watchpoint.
//   Escape           - Cancel address entry.
//
// Address-entry mode gets first chance at keyboard input so A-F remain
// available as hexadecimal digits.
//
// Parameters:
//   bytes    - Key bytes received from BeAPI.
//   numBytes - Number of key bytes supplied.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::KeyDown (const char *bytes, int32 numBytes)
{
	if (!bytes || numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	const char key = bytes[0];

	/*
	 * Address-entry mode gets first chance at the key because A-F are
	 * hexadecimal digits while entering an address.
	 */
	if (fEntryMode != BREAKPOINT_ENTRY_NONE) {
		if (key == B_ESCAPE) {
			fEntryMode = BREAKPOINT_ENTRY_NONE;
			fPendingBreakPointAddress = 0x0000;
			fPendingBreakPointDigits = 0;

			Invalidate();
			return;
		}

		if (key == B_BACKSPACE || key == B_DELETE) {
			if (fPendingBreakPointDigits > 0) {
				fPendingBreakPointAddress >>= 4;
				fPendingBreakPointDigits--;
			}

			Invalidate();
			return;
		}

		if (key == B_ENTER) {
			if (fPendingBreakPointDigits > 0) {
				const uint16 address = fPendingBreakPointAddress;

				switch (fEntryMode) {
					case BREAKPOINT_ENTRY_EXECUTE:
						nes::cpu::debug_add_execute_breakpoint(address);

						fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
						fSelectedWatchPointAddress = 0x0000;
						fHasSelectedExecuteBreakPoint = true;
						fSelectedExecuteBreakPoint = address;
						break;

					case BREAKPOINT_ENTRY_READ:
						nes::cpu::debug_add_read_watchpoint(address);

						fHasSelectedExecuteBreakPoint = false;
						fSelectedExecuteBreakPoint = 0x0000;
						fSelectedWatchPointType = WATCHPOINT_SELECTION_READ;
						fSelectedWatchPointAddress = address;
						break;

					case BREAKPOINT_ENTRY_WRITE:
						nes::cpu::debug_add_write_watchpoint(address);

						fHasSelectedExecuteBreakPoint = false;
						fSelectedExecuteBreakPoint = 0x0000;
						fSelectedWatchPointType = WATCHPOINT_SELECTION_WRITE;
						fSelectedWatchPointAddress = address;
						break;

					case BREAKPOINT_ENTRY_NONE:
					default:
						break;
				}
			}

			fEntryMode = BREAKPOINT_ENTRY_NONE;
			fPendingBreakPointAddress = 0x0000;
			fPendingBreakPointDigits = 0;

			Invalidate();
			return;
		}

		int32 digit = -1;

		if (key >= '0' && key <= '9') {
			digit = key - '0';

		} else if (key >= 'a' && key <= 'f') {
			digit = key - 'a' + 10;

		} else if (key >= 'A' && key <= 'F') {
			digit = key - 'A' + 10;
		}

		if (digit >= 0) {
			if (fPendingBreakPointDigits < 4) {
				fPendingBreakPointAddress = static_cast<uint16>((fPendingBreakPointAddress << 4) | digit);
				fPendingBreakPointDigits++;
			}

			Invalidate();
			return;
		}

		return;
	}

	switch (key) {
		case B_UP_ARROW:
			MoveBreakPointSelection(-1);
			return;

		case B_DOWN_ARROW:
			MoveBreakPointSelection(1);
			return;

		case 'e':
		case 'E':
			fEntryMode = BREAKPOINT_ENTRY_EXECUTE;
			fPendingBreakPointAddress = 0x0000;
			fPendingBreakPointDigits = 0;

			Invalidate();
			return;

		case 'r':
		case 'R':
			fEntryMode = BREAKPOINT_ENTRY_READ;
			fPendingBreakPointAddress = 0x0000;
			fPendingBreakPointDigits = 0;

			Invalidate();
			return;

		case 'w':
		case 'W':
			fEntryMode = BREAKPOINT_ENTRY_WRITE;
			fPendingBreakPointAddress = 0x0000;
			fPendingBreakPointDigits = 0;

			Invalidate();
			return;

		case B_DELETE:
		case B_BACKSPACE:
		{
			WatchPointSelectionType watchType = WATCHPOINT_SELECTION_NONE;
			uint16 address = 0x0000;

			/*
			 * Remove a selected READ or WRITE watchpoint first.
			 */
			if (SelectedWatchPoint(watchType, address)) {
				bool removedHitWatchPoint = false;

				switch (watchType) {
					case WATCHPOINT_SELECTION_READ:
						removedHitWatchPoint = nes::cpu::debug_breakpoint_hit()
							&& (nes::cpu::debug_break_reason()
								== nes::cpu::DEBUG_BREAK_MEMORY_READ)
							&& (nes::cpu::debug_memory_break_address() == address);

						nes::cpu::debug_remove_read_watchpoint(address);
						break;

					case WATCHPOINT_SELECTION_WRITE:
						removedHitWatchPoint = nes::cpu::debug_breakpoint_hit()
							&& (nes::cpu::debug_break_reason()
								== nes::cpu::DEBUG_BREAK_MEMORY_WRITE)
							&& (nes::cpu::debug_memory_break_address() == address);

						nes::cpu::debug_remove_write_watchpoint(address);
						break;

					case WATCHPOINT_SELECTION_NONE:
					default:
						break;
				}

				fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
				fSelectedWatchPointAddress = 0x0000;

				if (removedHitWatchPoint && fParent) {
					fParent->DebugResumeExecution();
				}

				Invalidate();
				return;
			}

			/*
			 * Otherwise remove the selected execute BreakPoint.
			 */
			if (!SelectedExecuteBreakPoint(address)) {
				return;
			}

			const bool removedHitBreakPoint = nes::cpu::debug_breakpoint_hit()
				&& (nes::cpu::debug_break_reason()
					== nes::cpu::DEBUG_BREAK_EXECUTE)
				&& (nes::cpu::debug_breakpoint_hit_address() == address);

			nes::cpu::debug_remove_execute_breakpoint(address);
			
			fHasSelectedExecuteBreakPoint = false;
			fSelectedExecuteBreakPoint = 0x0000;

			if (removedHitBreakPoint && fParent) {
				fParent->DebugResumeExecution();
			}

			Invalidate();
			return;
		}

		case 'b':
		case 'B':
		{
			const bool enabled = !nes::cpu::debug_stack_sp_break_enabled();
			const uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			nes::cpu::debug_set_stack_sp_break(enabled, threshold);

			Invalidate();
			return;
		}

		case '[':
		{
			uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			if (threshold > 0x00) {
				threshold--;
			}

			nes::cpu::debug_set_stack_sp_break(nes::cpu::debug_stack_sp_break_enabled(), threshold);

			Invalidate();
			return;
		}

		case ']':
		{
			uint8 threshold = nes::cpu::debug_stack_sp_break_threshold();

			if (threshold < 0xff) {
				threshold++;
			}

			nes::cpu::debug_set_stack_sp_break(nes::cpu::debug_stack_sp_break_enabled(), threshold);

			Invalidate();
			return;
		}

		case 's':
		case 'S':
			nes::cpu::debug_set_stack_wrap_break(!nes::cpu::debug_stack_wrap_break_enabled());

			Invalidate();
			return;

		case 'c':
		case 'C':
		{
			/*
			 * Remember whether the debugger is currently stopped by one
			 * of the address conditions we are about to remove.
			 */
			const auto reason = nes::cpu::debug_break_reason();

			const bool addressConditionHit = nes::cpu::debug_breakpoint_hit()
				&& (
					reason == nes::cpu::DEBUG_BREAK_EXECUTE
					|| reason
						== nes::cpu::DEBUG_BREAK_MEMORY_READ
					|| reason
						== nes::cpu::DEBUG_BREAK_MEMORY_WRITE);

			nes::cpu::debug_clear_execute_breakpoints();
			nes::cpu::debug_clear_read_watchpoints();
			nes::cpu::debug_clear_write_watchpoints();

			fHasSelectedExecuteBreakPoint = false;
			fSelectedExecuteBreakPoint = 0x0000;
			fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
			fSelectedWatchPointAddress = 0x0000;

			/*
			 * If an address condition caused the current debugger stop,
			 * clearing all address conditions removes that reason to remain
			 * paused.
			 *
			 * SP-threshold and stack-wrap stops are intentionally unaffected.
			 */
			if (addressConditionHit && fParent) {
				fParent->DebugResumeExecution();
			}

			Invalidate();
			return;
		}

		case 'h':
		case 'H':
			/*
			 * Reset accumulated hit counts without removing any configured
			 * execute BreakPoints or memory watchpoints.
			 */
			nes::cpu::debug_clear_breakpoint_hit_counts();
			nes::cpu::debug_clear_all_read_watchpoint_hit_counts();
			nes::cpu::debug_clear_all_write_watchpoint_hit_counts();

			Invalidate();
			return;

		case 'g':
		case 'G':
			if (fParent) {
				fParent->DebugResumeExecution();
			}

			Invalidate();
			return;

		case 'd':
		case 'D':
		case B_ENTER:
		{
			if (!fParent) {
				return;
			}

			uint16 address = 0x0000;

			/*
			 * An execute BreakPoint directly identifies an instruction
			 * address, so use it as the preferred disassembly target.
			 */
			if (SelectedExecuteBreakPoint(address)) {
				fParent->JumpCPUDisasmToAddress(address);
				return;
			}

			/*
			 * A READ or WRITE watchpoint identifies a memory address rather
			 * than an instruction address. If the selected watchpoint caused
			 * the current debugger stop, use the saved instruction-start
			 * address associated with that hit.
			 */
			WatchPointSelectionType watchType = WATCHPOINT_SELECTION_NONE;

			if (SelectedWatchPoint(watchType, address) && nes::cpu::debug_breakpoint_hit()) {
				const auto reason = nes::cpu::debug_break_reason();

				const bool readHit = watchType == WATCHPOINT_SELECTION_READ
					&& reason == nes::cpu::DEBUG_BREAK_MEMORY_READ
					&& nes::cpu::debug_memory_break_address() == address;

				const bool writeHit = watchType == WATCHPOINT_SELECTION_WRITE
					&& reason == nes::cpu::DEBUG_BREAK_MEMORY_WRITE
					&& nes::cpu::debug_memory_break_address() == address;

				if (readHit || writeHit) {
					fParent->JumpCPUDisasmToAddress(nes::cpu::debug_breakpoint_hit_address());

					return;
				}
			}

			/*
			 * No usable debugger condition is selected.
			 *
			 * While reset is still being processed, PC has not necessarily
			 * been loaded from the RESET vector yet. In that case, use the
			 * RESET-vector target instead of the temporary CPU PC.
			 *
			 * Once reset has completed, the live CPU PC is authoritative.
			 */
			uint16 targetAddress = 0x0000;

			if (nes::cpu::debug_reset_in_progress()) {
				const uint8 lo = nes::bus::debug_read_memory(0xfffc);
				const uint8 hi = nes::bus::debug_read_memory(0xfffd);

				targetAddress = static_cast<uint16>(lo | (static_cast<uint16>(hi) << 8));
			} else {
				const nes::cpu::cpu_state_t state = nes::cpu::debug_cpu_state();

				targetAddress = state.pc;
			}

			fParent->JumpCPUDisasmToAddress(targetAddress);
			return;
		}

		default:
			break;
	}

	BView::KeyDown(bytes, numBytes);
}


// -----------------------------------------------------------------------------
// BreakPointView::MouseDown
//
// Selects an execute BreakPoint or memory watchpoint row.
//
// Only one debugger condition is visually selected at a time. Selecting an
// execute BreakPoint clears any memory-watchpoint selection. Selecting a memory
// watchpoint clears the execute-BreakPoint selection.
//
// Clicking the currently selected row again clears the selection.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::MouseDown (BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	uint16 address = 0x0000;

	/*
	 * Execute BreakPoint.
	 */
	if (ExecuteBreakPointAddressForPoint(where, address)) {
		fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
		fSelectedWatchPointAddress = 0x0000;

		if (fHasSelectedExecuteBreakPoint && (fSelectedExecuteBreakPoint == address)) {
			fHasSelectedExecuteBreakPoint = false;
			fSelectedExecuteBreakPoint = 0x0000;
		} else {
			fHasSelectedExecuteBreakPoint = true;
			fSelectedExecuteBreakPoint = address;
		}

		Invalidate();
		return;
	}

	/*
	 * READ watchpoint.
	 */
	if (ReadWatchPointAddressForPoint(where, address)) {
		fHasSelectedExecuteBreakPoint = false;
		fSelectedExecuteBreakPoint = 0x0000;

		if (fSelectedWatchPointType == WATCHPOINT_SELECTION_READ
			&& (fSelectedWatchPointAddress == address)) {
			fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
			fSelectedWatchPointAddress = 0x0000;
		} else {
			fSelectedWatchPointType = WATCHPOINT_SELECTION_READ;
			fSelectedWatchPointAddress = address;
		}

		Invalidate();
		return;
	}

	/*
	 * WRITE watchpoint.
	 */
	if (WriteWatchPointAddressForPoint(where, address)) {
		fHasSelectedExecuteBreakPoint = false;
		fSelectedExecuteBreakPoint = 0x0000;

		if (fSelectedWatchPointType == WATCHPOINT_SELECTION_WRITE
			&& (fSelectedWatchPointAddress == address)) {
			fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
			fSelectedWatchPointAddress = 0x0000;
		} else {
			fSelectedWatchPointType = WATCHPOINT_SELECTION_WRITE;
			fSelectedWatchPointAddress = address;
		}

		Invalidate();
		return;
	}
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawHeaderPanel
//
// Draws the BreakPoint Manager controls/header panel.
//
// In normal mode, the header shows READ watchpoint, WRITE watchpoint, execute
// BreakPoint, selection, hit-count, stack BreakPoint, resume, and disassembly
// controls.
//
// The address-condition controls are presented in the same natural order as the
// lower debugger panels:
//
//   READ -> WRITE -> Execute
//
// While entering a new condition, the normal controls are replaced by an
// address-entry display identifying the type being added.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::DrawHeaderPanel()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 76.0f);
	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	BFont normalFont;
	GetFont(&normalFont);

	BFont fixedFont(be_fixed_font);
	fixedFont.SetSize(11.0f);

	const float left = panel.left + 8.0f;
	float y = panel.top + 34.0f;

	/*
	 * Address-entry mode.
	 */
	if (fEntryMode != BREAKPOINT_ENTRY_NONE) {
		const char *entryLabel = "Add execute BreakPoint:";

		switch (fEntryMode) {
			case BREAKPOINT_ENTRY_READ:
				entryLabel = "Add READ watchpoint:";
				break;

			case BREAKPOINT_ENTRY_WRITE:
				entryLabel = "Add WRITE watchpoint:";
				break;

			case BREAKPOINT_ENTRY_EXECUTE:
			case BREAKPOINT_ENTRY_NONE:
			default:
				entryLabel = "Add execute BreakPoint:";
				break;
		}

		SetFont(&normalFont);
		SetHighColor(80, 80, 80);

		DrawString(entryLabel, BPoint(left, y));

		const float x = left + normalFont.StringWidth(entryLabel) + 6.0f;
		BString addressText;

		switch (fPendingBreakPointDigits) {
			case 0:
				addressText.SetTo("$____");
				break;

			case 1:
				addressText.SetToFormat("$%01X___", fPendingBreakPointAddress);
				break;

			case 2:
				addressText.SetToFormat("$%02X__", fPendingBreakPointAddress);
				break;

			case 3:
				addressText.SetToFormat("$%03X_", fPendingBreakPointAddress);
				break;

			default:
				addressText.SetToFormat("$%04X", fPendingBreakPointAddress);
				break;
		}

		SetFont(&fixedFont);
		SetHighColor(35, 90, 180);

		DrawString(addressText.String(), BPoint(x, y));

		y += 18.0f;

		SetFont(&normalFont);
		SetHighColor(70, 70, 70);
		DrawString("0-9/A-F: enter   Backspace: edit   Enter: add   Esc: cancel", BPoint(left, y));

		SetFont(&normalFont);
		return;
	}

	/*
	 * Normal controls.
	 */
	SetFont(&normalFont);
	SetHighColor(35, 35, 35);
	DrawString("R: read   W: write   E: execute   Click/Up/Down: select   Delete: remove", BPoint(left, y));

	y += 18.0f;

	DrawString("C: clear all   H: reset hits   B: SP break   [/]: threshold   S: wrap   G: resume   D/Enter: disassemble",
		BPoint(left, y));

	SetFont(&normalFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawConditionPanel
//
// Draws debugger-wide BreakPoint conditions and current break state.
//
// This includes the SP-threshold BreakPoint and stack-wrap BreakPoint added by
// the Stack debugger.
//
// CPU execution state is derived from PretendoWindow:
//
//   STOPPED - ROM may be loaded, but the emulator session is not running.
//   PAUSED  - Emulator session is active but currently paused.
//   RUNNING - Emulator session is active and executing normally.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::DrawConditionPanel()
{
	BRect panel(4.0f, 76.0f, Bounds().right - 4.0f, 170.0f);

	::DrawDebugPanel(this, panel, "Break Conditions");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 2.0f;

	BFont previousFont;
	GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const float leftLabelX = panel.left + 10.0f;
	const float leftValueX = leftLabelX + 102.0f;
	const float rightLabelX = panel.left + 280.0f;
	const float rightValueX = rightLabelX + 92.0f;
	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	auto drawLeftKV = [&](const char *label, const char *value, bool monoValue) {
		SetFont(&previousFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetFont(monoValue ? &mono : &previousFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value, bool monoValue) {
		SetFont(&previousFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetFont(monoValue ? &mono : &previousFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	const bool spBreakEnabled = nes::cpu::debug_stack_sp_break_enabled();
	const bool spBreakArmed = nes::cpu::debug_stack_sp_break_armed();
	const char *spState = !spBreakEnabled ? "OFF" : (spBreakArmed ? "ARMED" : "HIT");

	BString s;
	s.SetToFormat("$%02X", nes::cpu::debug_stack_sp_break_threshold());

	drawLeftKV("SP threshold:", s.String(), true);
	drawLeftKV("SP state:", spState, false);
	drawLeftKV("Wrap break:", nes::cpu::debug_stack_wrap_break_enabled() ? "ON" : "OFF", false);

	s.SetToFormat("%ld", static_cast<long>(ExecuteBreakPointCount()));
	drawRightKV("Execute:", s.String(), false);

	const char *cpuState = "STOPPED";

	if (fParent && fParent->IsEmulatorRunning()) {
		cpuState = fParent->IsEmulatorPaused() ? "PAUSED" : "RUNNING";
	}

	drawRightKV("CPU state:", cpuState, false);
	drawRightKV("Reason:", CurrentBreakReasonText(), false);

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawExecuteBreakPointPanel
//
// Draws active execute BreakPoints and their accumulated hit counts.
//
// The Execute BreakPoint panel occupies the right side of the BreakPoint
// Manager's lower area. Memory READ and WRITE watchpoints occupy the left side.
//
// Each row displays the BreakPoint address, accumulated hit count, and current
// state.
//
// The selected BreakPoint is highlighted in blue. A BreakPoint responsible for
// the currently latched debugger stop is identified as HIT.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::DrawExecuteBreakPointPanel()
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middle = Bounds().left + (Bounds().Width() * 0.5f);

	BRect panel(middle + 3.0f, lowerTop, Bounds().right - 4.0f, lowerBottom);
	::DrawDebugPanel(this, panel, "Execute Breakpoints");

	BFont previousFont;
	GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const float addressX = panel.left + 12.0f;
	const float hitsX = panel.left + 112.0f;
	const float stateX = panel.left + 202.0f;
	float y = panel.top + 38.0f;

	SetHighColor(80, 80, 80);

	DrawString("Address", BPoint(addressX, y));
	DrawString("Hits", BPoint(hitsX, y));
	DrawString("State", BPoint(stateX, y));

	y += lineH;

	ExecuteBreakPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 capturedCount = CaptureExecuteBreakPoints(entries, kMaximumDisplayedBreakPoints);
	const int32 totalCount = ExecuteBreakPointCount();

	if (capturedCount <= 0) {
		SetFont(&previousFont);
		SetHighColor(90, 90, 90);
		DrawString("No execute Breakpoints configured.", BPoint(panel.left + 12.0f, y + 4.0f));

		return;
	}

	for (int32 i = 0; i < capturedCount; i++) {
		const ExecuteBreakPointEntry &entry = entries[i];
		const bool selected = fHasSelectedExecuteBreakPoint && (fSelectedExecuteBreakPoint == entry.address);
		const bool currentHit = nes::cpu::debug_breakpoint_hit()
			&& (nes::cpu::debug_break_reason()
				== nes::cpu::DEBUG_BREAK_EXECUTE)
			&& (nes::cpu::debug_breakpoint_hit_address() == entry.address);

		const BRect rowRect(panel.left + 6.0f, y - 12.0f, panel.right - 6.0f, y + 4.0f);

		if (selected) {
			SetHighColor(190, 215, 245);
			FillRect(rowRect);

			SetHighColor(70, 120, 180);
			StrokeRect(rowRect);
		}

		if (currentHit) {
			SetHighColor(150, 60, 40);
		} else {
			SetHighColor(0, 0, 0);
		}

		BString s;
		s.SetToFormat("$%04X", entry.address);
		DrawString(s.String(), BPoint(addressX, y));

		s.SetToFormat("%lu", static_cast<unsigned long>(entry.hitCount));
		DrawString(s.String(), BPoint(hitsX, y));
		DrawString(currentHit ? "HIT" : "ON", BPoint(stateX, y));

		y += lineH;
	}

	if (totalCount > capturedCount) {
		SetFont(&previousFont);
		SetHighColor(100, 100, 100);

		BString s;
		s.SetToFormat("... %ld more breakpoint%s", static_cast<long>(totalCount - capturedCount),
						((totalCount - capturedCount) == 1) ? "" : "s");
		DrawString(s.String(), BPoint(panel.left + 12.0f, panel.bottom - 12.0f));
	}

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::CaptureExecuteBreakPoints
//
// Collects active execute BreakPoints in ascending CPU-address order.
//
// Parameters:
//   entries  - Destination array.
//   capacity - Maximum number of entries that may be written.
//
// Returns:
//   Number of entries written.
// -----------------------------------------------------------------------------
int32
BreakPointView::CaptureExecuteBreakPoints (ExecuteBreakPointEntry *entries, int32 capacity) const
{
	if (!entries || capacity <= 0) {
		return 0;
	}

	int32 count = 0;

	for (uint32 address = 0; address <= 0xffff && count < capacity; address++) {
		const uint16 cpuAddress = static_cast<uint16>(address);

		if (!nes::cpu::debug_has_execute_breakpoint(cpuAddress)) {
			continue;
		}

		entries[count].address = cpuAddress;
		entries[count].hitCount = nes::cpu::debug_breakpoint_hit_count(cpuAddress);

		count++;
	}

	return count;
}


// -----------------------------------------------------------------------------
// BreakPointView::ExecuteBreakPointCount
//
// Counts active execute BreakPoints.
//
// Parameters:
//   None.
//
// Returns:
//   Number of enabled execute BreakPoints.
// -----------------------------------------------------------------------------
int32
BreakPointView::ExecuteBreakPointCount() const
{
	int32 count = 0;

	for (uint32 address = 0; address <= 0xffff; address++) {
		if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
			count++;
		}
	}

	return count;
}


// -----------------------------------------------------------------------------
// BreakPointView::CurrentBreakReasonText
//
// Returns readable text describing the currently latched debugger stop reason.
//
// Parameters:
//   None.
//
// Returns:
//   Static break-reason string.
// -----------------------------------------------------------------------------
const char*
BreakPointView::CurrentBreakReasonText() const
{
	if (!nes::cpu::debug_breakpoint_hit()) {
		return "none";
	}

	switch (nes::cpu::debug_break_reason()) {
		case nes::cpu::DEBUG_BREAK_EXECUTE:
			return "execute";

		case nes::cpu::DEBUG_BREAK_MEMORY_READ:
			return "memory read";

		case nes::cpu::DEBUG_BREAK_MEMORY_WRITE:
			return "memory write";

		case nes::cpu::DEBUG_BREAK_STACK_SP:
			return "SP threshold";

		case nes::cpu::DEBUG_BREAK_STACK_WRAP:
			return "stack wrap";

		case nes::cpu::DEBUG_BREAK_NONE:
		default:
			return "unknown";
	}
}


// -----------------------------------------------------------------------------
// BreakPointView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded.
// -----------------------------------------------------------------------------
bool
BreakPointView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawNoROMMessage
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
BreakPointView::DrawNoROMMessage (BRect panel)
{
	BFont previousFont;
	GetFont(&previousFont);

	BFont font = previousFont;

	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect debugger Breakpoints.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + panel.Width() * 0.5f;
	const float centerY = panel.top + panel.Height() * 0.5f;

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(centerX - StringWidth(title) * 0.5f, centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - StringWidth(detail) * 0.5f, centerY + fh.ascent + 8.0f));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawReadWatchPointPanel
//
// Draws active CPU memory READ watchpoints.
//
// READ and WRITE watchpoints are grouped together on the left side of the
// BreakPoint Manager. READ watchpoints occupy the upper half.
//
// Each row displays the watched address, accumulated hit count, and current
// state. A watchpoint responsible for the currently latched debugger stop is
// shown with HIT state.
//
// The currently selected READ watchpoint is highlighted.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::DrawReadWatchPointPanel()
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middleX = Bounds().left + (Bounds().Width() * 0.5f);
	const float middleY = lowerTop + ((lowerBottom - lowerTop) * 0.5f);

	BRect panel(4.0f, lowerTop, middleX - 3.0f, middleY - 3.0f);
	::DrawDebugPanel(this, panel, "READ Watchpoints");

	BFont previousFont;
	GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const float addressX = panel.left + 12.0f;
	const float hitsX = panel.right - 122.0f;
	const float stateX = panel.right - 62.0f;
	float y = panel.top + 38.0f;

	SetHighColor(80, 80, 80);
	DrawString("Address", BPoint(addressX, y));
	DrawString("Hits", BPoint(hitsX, y));
	DrawString("State", BPoint(stateX, y));

	y += lineH;

	ReadWatchPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 capturedCount = CaptureReadWatchPoints(entries, kMaximumDisplayedBreakPoints);
	const int32 totalCount = ReadWatchPointCount();

	if (capturedCount <= 0) {
		SetFont(&previousFont);
		SetHighColor(90, 90, 90);
		DrawString("No READ watchpoints configured.", BPoint(panel.left + 12.0f, y + 4.0f));

		return;
	}

	for (int32 i = 0; i < capturedCount; i++) {
		const ReadWatchPointEntry &entry = entries[i];

		const bool currentHit = nes::cpu::debug_breakpoint_hit()
			&& (nes::cpu::debug_break_reason()
				== nes::cpu::DEBUG_BREAK_MEMORY_READ)
			&& (nes::cpu::debug_memory_break_address()
				== entry.address);
		const bool selected = (fSelectedWatchPointType == WATCHPOINT_SELECTION_READ)
			&& (fSelectedWatchPointAddress == entry.address);
		const BRect rowRect(panel.left + 6.0f, y - 12.0f, panel.right - 6.0f, y + 4.0f);

		if (selected) {
			SetHighColor(190, 215, 245);
			FillRect(rowRect);

			SetHighColor(70, 120, 180);
			StrokeRect(rowRect);
		}

		if (currentHit) {
			SetHighColor(150, 60, 40);
		} else {
			SetHighColor(0, 0, 0);
		}

		BString s;
		s.SetToFormat("$%04X", entry.address);
		DrawString(s.String(), BPoint(addressX, y));

		s.SetToFormat("%lu", static_cast<unsigned long>(entry.hitCount));
		DrawString(s.String(), BPoint(hitsX, y));
		DrawString((currentHit ? "HIT" : "ON"), BPoint(stateX, y));

		y += lineH;
	}

	if (totalCount > capturedCount) {
		SetFont(&previousFont);
		SetHighColor(100, 100, 100);

		BString s;
		s.SetToFormat("... %ld more", static_cast<long>(totalCount - capturedCount));
		DrawString(s.String(), BPoint(panel.left + 12.0f, panel.bottom - 10.0f));
	}

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::DrawWriteWatchPointPanel
//
// Draws active CPU memory WRITE watchpoints.
//
// READ and WRITE watchpoints are grouped together on the left side of the
// BreakPoint Manager. WRITE watchpoints occupy the lower half.
//
// Each row displays the watched address, accumulated hit count, and current
// state. A watchpoint responsible for the currently latched debugger stop is
// shown with HIT state.
//
// The currently selected WRITE watchpoint is highlighted.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::DrawWriteWatchPointPanel()
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middleX = Bounds().left + (Bounds().Width() * 0.5f);
	const float middleY = lowerTop + ((lowerBottom - lowerTop) * 0.5f);

	BRect panel(4.0f, middleY + 3.0f, middleX - 3.0f, lowerBottom);
	::DrawDebugPanel(this, panel, "WRITE Watchpoints");

	BFont previousFont;
	GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	font_height fh;
	GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const float addressX = panel.left + 12.0f;
	const float hitsX = panel.right - 122.0f;
	const float stateX = panel.right - 62.0f;
	float y = panel.top + 38.0f;

	SetHighColor(80, 80, 80);
	DrawString("Address", BPoint(addressX, y));
	DrawString("Hits", BPoint(hitsX, y));
	DrawString("State", BPoint(stateX, y));

	y += lineH;

	WriteWatchPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 capturedCount = CaptureWriteWatchPoints(entries, kMaximumDisplayedBreakPoints);
	const int32 totalCount = WriteWatchPointCount();

	if (capturedCount <= 0) {
		SetFont(&previousFont);
		SetHighColor(90, 90, 90);
		DrawString("No WRITE watchpoints configured.", BPoint(panel.left + 12.0f, y + 4.0f));

		return;
	}

	for (int32 i = 0; i < capturedCount; i++) {
		const WriteWatchPointEntry &entry = entries[i];
		const bool currentHit
			= nes::cpu::debug_breakpoint_hit()
			&& (nes::cpu::debug_break_reason()
				== nes::cpu::DEBUG_BREAK_MEMORY_WRITE)
			&& (nes::cpu::debug_memory_break_address() == entry.address);
		const bool selected = (fSelectedWatchPointType == WATCHPOINT_SELECTION_WRITE)
			&& (fSelectedWatchPointAddress == entry.address);

		const BRect rowRect(panel.left + 6.0f, y - 12.0f, panel.right - 6.0f, y + 4.0f);

		if (selected) {
			SetHighColor(190, 215, 245);
			FillRect(rowRect);

			SetHighColor(70, 120, 180);
			StrokeRect(rowRect);
		}

		if (currentHit) {
			SetHighColor(150, 60, 40);
		} else {
			SetHighColor(0, 0, 0);
		}

		BString s;
		s.SetToFormat("$%04X", entry.address);
		DrawString(s.String(), BPoint(addressX, y));

		s.SetToFormat("%lu", static_cast<unsigned long>(entry.hitCount));
		DrawString(s.String(), BPoint(hitsX, y));
		DrawString((currentHit ? "HIT" : "ON"), BPoint(stateX, y));

		y += lineH;
	}

	if (totalCount > capturedCount) {
		SetFont(&previousFont);
		SetHighColor(100, 100, 100);

		BString s;
		s.SetToFormat("... %ld more", static_cast<long>(totalCount - capturedCount));
		DrawString(s.String(), BPoint(panel.left + 12.0f, panel.bottom - 10.0f));
	}

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// BreakPointView::ExecuteBreakPointAddressForPoint
//
// Converts a mouse position inside the execute-BreakPoint table into the CPU
// address represented by that row.
//
// The row geometry mirrors DrawExecuteBreakPointPanel() so drawing and mouse
// hit-testing remain synchronized.
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the selected execute-BreakPoint address.
//
// Returns:
//   true if the mouse position corresponds to a visible BreakPoint row.
// -----------------------------------------------------------------------------
bool
BreakPointView::ExecuteBreakPointAddressForPoint (BPoint where, uint16 &address) const
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middle = Bounds().left + (Bounds().Width() * 0.5f);
	BRect panel(middle + 3.0f, lowerTop, Bounds().right - 4.0f, lowerBottom);

	if (!panel.Contains(where)) {
		return false;
	}

	BFont previousFont;
	const_cast<BreakPointView *>(this)->GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	const_cast<BreakPointView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<BreakPointView *>(this)->GetFontHeight(&fh);

	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const_cast<BreakPointView *>(this)->SetFont(&previousFont);
	float y = panel.top + 38.0f;

	/*
	 * Skip the column-heading row.
	 */
	y += lineH;

	ExecuteBreakPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 count = CaptureExecuteBreakPoints(entries, kMaximumDisplayedBreakPoints);

	for (int32 i = 0; i < count; i++) {
		const float rowTop = y - 12.0f;
		const float rowBottom = y + 4.0f;

		if (where.y >= rowTop && (where.y <= rowBottom)) {
			address = entries[i].address;

			return true;
		}

		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// BreakPointView::SelectedExecuteBreakPoint
//
// Returns the currently selected execute breakpoint.
//
// The CPU breakpoint table is checked before returning the address so stale
// selections are not treated as valid after another debugger window removes
// the breakpoint.
//
// Parameters:
//   address - Receives the selected CPU address.
//
// Returns:
//   true if a valid execute breakpoint is currently selected.
// -----------------------------------------------------------------------------
bool
BreakPointView::SelectedExecuteBreakPoint (uint16 &address) const
{
	if (!fHasSelectedExecuteBreakPoint) {
		return false;
	}

	if (!nes::cpu::debug_has_execute_breakpoint(fSelectedExecuteBreakPoint)) {
		return false;
	}

	address = fSelectedExecuteBreakPoint;
	return true;
}


// -----------------------------------------------------------------------------
// BreakPointView::MoveBreakPointSelection
//
// Moves the current debugger-condition selection upward or downward across
// READ watchpoints, WRITE watchpoints, and execute BreakPoints.
//
// Conditions are traversed in this order:
//
//   READ watchpoints
//   WRITE watchpoints
//   Execute BreakPoints
//
// Within each group, addresses are visited in ascending CPU-address order.
//
// If no valid condition is currently selected:
//
//   direction > 0 - selects the first condition.
//   direction < 0 - selects the last condition.
//
// Selection stops at the beginning or end rather than wrapping around.
//
// Parameters:
//   direction - Negative to move upward; positive to move downward.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
BreakPointView::MoveBreakPointSelection (int32 direction)
{
	if (direction == 0) {
		return;
	}

	enum SelectionGroup {
		SELECTION_GROUP_NONE = 0,
		SELECTION_GROUP_READ,
		SELECTION_GROUP_WRITE,
		SELECTION_GROUP_EXECUTE
	};

	SelectionGroup currentGroup = SELECTION_GROUP_NONE;
	uint16 currentAddress = 0x0000;

	/*
	 * Determine whether the current selection is still valid.
	 */
	WatchPointSelectionType watchType = WATCHPOINT_SELECTION_NONE;
	uint16 watchAddress = 0x0000;

	if (SelectedWatchPoint(watchType, watchAddress)) {
		currentAddress = watchAddress;

		if (watchType == WATCHPOINT_SELECTION_READ) {
			currentGroup = SELECTION_GROUP_READ;
		} else if (watchType == WATCHPOINT_SELECTION_WRITE) {
			currentGroup = SELECTION_GROUP_WRITE;
		}

	} else {
		uint16 executeAddress = 0x0000;

		if (SelectedExecuteBreakPoint(executeAddress)) {
			currentGroup = SELECTION_GROUP_EXECUTE;
			currentAddress = executeAddress;
		}
	}

	/*
	 * Helper used whenever READ becomes the selected group.
	 */
	auto selectRead = [&](uint16 address) {
		fHasSelectedExecuteBreakPoint = false;
		fSelectedExecuteBreakPoint = 0x0000;
		fSelectedWatchPointType = WATCHPOINT_SELECTION_READ;
		fSelectedWatchPointAddress = address;

		Invalidate();
	};

	/*
	 * Helper used whenever WRITE becomes the selected group.
	 */
	auto selectWrite = [&](uint16 address) {
		fHasSelectedExecuteBreakPoint = false;
		fSelectedExecuteBreakPoint = 0x0000;

		fSelectedWatchPointType = WATCHPOINT_SELECTION_WRITE;
		fSelectedWatchPointAddress = address;

		Invalidate();
	};

	/*
	 * Helper used whenever Execute becomes the selected group.
	 */
	auto selectExecute = [&](uint16 address) {
		fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
		fSelectedWatchPointAddress = 0x0000;
		fHasSelectedExecuteBreakPoint = true;
		fSelectedExecuteBreakPoint = address;

		Invalidate();
	};

	/*
	 * No valid selection.
	 *
	 * Down begins at the first READ/WRITE/Execute condition.
	 * Up begins at the final Execute/WRITE/READ condition.
	 */
	if (currentGroup == SELECTION_GROUP_NONE) {
		if (direction > 0) {
			for (uint32 address = 0x0000; address <= 0xffff; address++) {
				if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
					selectRead(static_cast<uint16>(address));

					return;
				}
			}

			for (uint32 address = 0x0000; address <= 0xffff; address++) {
				if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
					selectWrite(static_cast<uint16>(address));

					return;
				}
			}

			for (uint32 address = 0x0000; address <= 0xffff; address++) {
				if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
					selectExecute(static_cast<uint16>(address));

					return;
				}
			}
		} else {
			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
					selectExecute(static_cast<uint16>(address));

					return;
				}
			}

			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
					selectWrite(static_cast<uint16>(address));

					return;
				}
			}

			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
					selectRead(static_cast<uint16>(address));

					return;
				}
			}
		}

		/*
		 * No debugger conditions exist.
		 */
		fHasSelectedExecuteBreakPoint = false;
		fSelectedExecuteBreakPoint = 0x0000;
		fSelectedWatchPointType = WATCHPOINT_SELECTION_NONE;
		fSelectedWatchPointAddress = 0x0000;

		Invalidate();
		return;
	}

	/*
	 * Move downward.
	 */
	if (direction > 0) {
		switch (currentGroup) {
			case SELECTION_GROUP_READ:
				if (currentAddress < 0xffff) {
					for (uint32 address = static_cast<uint32>(currentAddress) + 1; address <= 0xffff; address++) {
						if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
							selectRead(static_cast<uint16>(address));

							return;
						}
					}
				}

				/*
				 * End of READ list: move to first WRITE.
				 */
				for (uint32 address = 0x0000; address <= 0xffff; address++) {
					if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
						selectWrite(static_cast<uint16>(address));

						return;
					}
				}

				/*
				 * No WRITE entries: move to first Execute.
				 */
				for (uint32 address = 0x0000; address <= 0xffff; address++) {
					if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
						selectExecute(static_cast<uint16>(address));

						return;
					}
				}

				return;

			case SELECTION_GROUP_WRITE:
				if (currentAddress < 0xffff) {
					for (uint32 address = static_cast<uint32>(currentAddress) + 1; address <= 0xffff; address++) {
						if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
							selectWrite(static_cast<uint16>(address));

							return;
						}
					}
				}

				/*
				 * End of WRITE list: move to first Execute.
				 */
				for (uint32 address = 0x0000; address <= 0xffff; address++) {
					if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
						selectExecute(static_cast<uint16>(address));

						return;
					}
				}

				return;

			case SELECTION_GROUP_EXECUTE:
				if (currentAddress < 0xffff) {
					for (uint32 address = static_cast<uint32>(currentAddress) + 1; address <= 0xffff; address++) {
						if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
							selectExecute(static_cast<uint16>(address));

							return;
						}
					}
				}

				/*
				 * Already at the final condition.
				 */
				return;

			case SELECTION_GROUP_NONE:
			default:
				return;
		}
	}

	/*
	 * Move upward.
	 */
	switch (currentGroup) {
		case SELECTION_GROUP_EXECUTE:
			if (currentAddress > 0x0000) {
				for (int32 address = static_cast<int32>(currentAddress) - 1; address >= 0x0000; address--) {
					if (nes::cpu::debug_has_execute_breakpoint(static_cast<uint16>(address))) {
						selectExecute(static_cast<uint16>(address));

						return;
					}
				}
			}

			/*
			 * Beginning of Execute list: move to final WRITE.
			 */
			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
					selectWrite(static_cast<uint16>(address));

					return;
				}
			}

			/*
			 * No WRITE entries: move to final READ.
			 */
			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
					selectRead(static_cast<uint16>(address));

					return;
				}
			}

			return;

		case SELECTION_GROUP_WRITE:
			if (currentAddress > 0x0000) {
				for (int32 address = static_cast<int32>(currentAddress) - 1; address >= 0x0000; address--) {
					if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
						selectWrite(static_cast<uint16>(address));

						return;
					}
				}
			}

			/*
			 * Beginning of WRITE list: move to final READ.
			 */
			for (int32 address = 0xffff; address >= 0x0000; address--) {
				if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
					selectRead(static_cast<uint16>(address));

					return;
				}
			}

			return;

		case SELECTION_GROUP_READ:
			if (currentAddress > 0x0000) {
				for (int32 address = static_cast<int32>(currentAddress) - 1; address >= 0x0000; address--) {
					if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
						selectRead(static_cast<uint16>(address));

						return;
					}
				}
			}

			/*
			 * Already at the first condition.
			 */
			return;

		case SELECTION_GROUP_NONE:
		default:
			return;
	}
}


// -----------------------------------------------------------------------------
// BreakPointView::CaptureReadWatchPoints
//
// Captures the currently configured READ watchpoints for display in the
// BreakPoint Manager.
//
// Each captured entry contains both the watched CPU address and its accumulated
// hit count.
//
// Parameters:
//   entries  - Destination array for captured READ-watchpoint entries.
//   capacity - Maximum number of entries that may be written.
//
// Returns:
//   Number of READ watchpoints captured.
// -----------------------------------------------------------------------------
int32
BreakPointView::CaptureReadWatchPoints (ReadWatchPointEntry *entries, int32 capacity) const
{
	if (!entries || capacity <= 0) {
		return 0;
	}

	int32 count = 0;

	for (uint32 address = 0x0000; address <= 0xffff && (count < capacity); address++) {
		const uint16 watchAddress = static_cast<uint16>(address);

		if (!nes::cpu::debug_has_read_watchpoint(watchAddress)) {
			continue;
		}

		entries[count].address = watchAddress;
		entries[count].hitCount = nes::cpu::debug_read_watchpoint_hit_count(watchAddress);

		count++;
	}

	return count;
}

// -----------------------------------------------------------------------------
// BreakPointView::CaptureWriteWatchPoints
//
// Captures the currently configured WRITE watchpoints for display in the
// BreakPoint Manager.
//
// Each captured entry contains both the watched CPU address and its accumulated
// hit count.
//
// Parameters:
//   entries  - Destination array for captured WRITE-watchpoint entries.
//   capacity - Maximum number of entries that may be written.
//
// Returns:
//   Number of WRITE watchpoints captured.
// -----------------------------------------------------------------------------
int32
BreakPointView::CaptureWriteWatchPoints (WriteWatchPointEntry *entries, int32 capacity) const
{
	if (!entries || capacity <= 0) {
		return 0;
	}

	int32 count = 0;

	for (uint32 address = 0x0000; address <= 0xffff && (count < capacity); address++) {
		const uint16 watchAddress = static_cast<uint16>(address);

		if (!nes::cpu::debug_has_write_watchpoint(watchAddress)) {
			continue;
		}

		entries[count].address = watchAddress;
		entries[count].hitCount = nes::cpu::debug_write_watchpoint_hit_count(watchAddress);

		count++;
	}

	return count;
}


// -----------------------------------------------------------------------------
// BreakPointView::ReadWatchPointCount
//
// Counts active CPU memory READ watchpoints.
//
// Parameters:
//   None.
//
// Returns:
//   Number of enabled READ watchpoints.
// -----------------------------------------------------------------------------
int32
BreakPointView::ReadWatchPointCount() const
{
	int32 count = 0;

	for (uint32 address = 0; address <= 0xffff; address++) {
		if (nes::cpu::debug_has_read_watchpoint(static_cast<uint16>(address))) {
			count++;
		}
	}

	return count;
}


// -----------------------------------------------------------------------------
// BreakPointView::WriteWatchPointCount
//
// Counts active CPU memory WRITE watchpoints.
//
// Parameters:
//   None.
//
// Returns:
//   Number of enabled WRITE watchpoints.
// -----------------------------------------------------------------------------
int32
BreakPointView::WriteWatchPointCount() const
{
	int32 count = 0;

	for (uint32 address = 0; address <= 0xffff; address++) {
		if (nes::cpu::debug_has_write_watchpoint(static_cast<uint16>(address))) {
			count++;
		}
	}

	return count;
}


// -----------------------------------------------------------------------------
// BreakPointView::ReadWatchPointAddressForPoint
//
// Converts a mouse position inside the READ-watchpoint table into the CPU
// address represented by that row.
//
// The row geometry mirrors DrawReadWatchPointPanel().
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the selected READ-watchpoint address.
//
// Returns:
//   true if the mouse position corresponds to a visible READ-watchpoint row.
// -----------------------------------------------------------------------------
bool
BreakPointView::ReadWatchPointAddressForPoint (BPoint where, uint16 &address) const
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middleX = Bounds().left + (Bounds().Width() * 0.5f);
	const float middleY = lowerTop + ((lowerBottom - lowerTop) * 0.5f);
	BRect panel(4.0f, lowerTop, middleX - 3.0f, middleY - 3.0f);

	if (!panel.Contains(where)) {
		return false;
	}

	BFont previousFont;
	const_cast<BreakPointView *>(this)->GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	const_cast<BreakPointView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<BreakPointView *>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const_cast<BreakPointView *>(this)->SetFont(&previousFont);
	float y = panel.top + 38.0f;

	/*
	 * Skip column headings.
	 */
	y += lineH;

	ReadWatchPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 count = CaptureReadWatchPoints(entries, kMaximumDisplayedBreakPoints);

	for (int32 i = 0; i < count; i++) {
		const float rowTop = y - 12.0f;
		const float rowBottom = y + 4.0f;

		if ((where.y >= rowTop) && (where.y <= rowBottom)) {
			address = entries[i].address;

			return true;
		}

		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// BreakPointView::WriteWatchPointAddressForPoint
//
// Converts a mouse position inside the WRITE-watchpoint table into the CPU
// address represented by that row.
//
// The row geometry mirrors DrawWriteWatchPointPanel().
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the selected WRITE-watchpoint address.
//
// Returns:
//   true if the mouse position corresponds to a visible WRITE-watchpoint row.
// -----------------------------------------------------------------------------
bool
BreakPointView::WriteWatchPointAddressForPoint (BPoint where, uint16 &address) const
{
	const float lowerTop = 182.0f;
	const float lowerBottom = Bounds().bottom - 8.0f;
	const float middleX = Bounds().left + (Bounds().Width() * 0.5f);
	const float middleY = lowerTop + ((lowerBottom - lowerTop) * 0.5f);
	BRect panel(4.0f, middleY + 3.0f, middleX - 3.0f, lowerBottom);

	if (!panel.Contains(where)) {
		return false;
	}

	BFont previousFont;
	const_cast<BreakPointView *>(this)->GetFont(&previousFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	const_cast<BreakPointView *>(this)->SetFont(&mono);

	font_height fh;
	const_cast<BreakPointView *>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;
	const_cast<BreakPointView *>(this)->SetFont(&previousFont);
	float y = panel.top + 38.0f;

	/*
	 * Skip column headings.
	 */
	y += lineH;

	WriteWatchPointEntry entries[kMaximumDisplayedBreakPoints];
	const int32 count = CaptureWriteWatchPoints(entries, kMaximumDisplayedBreakPoints);

	for (int32 i = 0; i < count; i++) {
		const float rowTop = y - 12.0f;
		const float rowBottom = y + 4.0f;

		if ((where.y >= rowTop) && (where.y <= rowBottom)) {
			address = entries[i].address;

			return true;
		}

		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// BreakPointView::SelectedWatchPoint
//
// Returns the currently selected memory watchpoint.
//
// The shared CPU watchpoint tables are checked before returning the selection so
// stale selections are not treated as valid if another debugger view removes
// the condition.
//
// Parameters:
//   type    - Receives the selected watchpoint type.
//   address - Receives the selected CPU address.
//
// Returns:
//   true if a valid READ or WRITE watchpoint is selected.
// -----------------------------------------------------------------------------
bool
BreakPointView::SelectedWatchPoint (WatchPointSelectionType &type, uint16 &address) const
{
	if (fSelectedWatchPointType == WATCHPOINT_SELECTION_NONE) {
		return false;
	}

	switch (fSelectedWatchPointType) {
		case WATCHPOINT_SELECTION_READ:
			if (!nes::cpu::debug_has_read_watchpoint(fSelectedWatchPointAddress)) {
				return false;
			}
			break;

		case WATCHPOINT_SELECTION_WRITE:
			if (!nes::cpu::debug_has_write_watchpoint(fSelectedWatchPointAddress)) {
				return false;
			}
			break;

		case WATCHPOINT_SELECTION_NONE:
		default:
			return false;
	}

	type = fSelectedWatchPointType;
	address = fSelectedWatchPointAddress;

	return true;
}

