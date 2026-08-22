
#include "CPUDisasmWindow.h"


// -----------------------------------------------------------------------------
// CPUDisasmWindow::CPUDisasmWindow
//
// Creates the floating CPU disassembly window and installs the disassembly view.
// The window is wide enough for address, opcode/operand bytes, instruction text,
// and expanded branch/call comments such as "branch forward -> $C012 not taken"
// without crowding the instruction column.
//
// Parameters:
//   parent - Main emulator window that owns this tool window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUDisasmWindow::CPUDisasmWindow (PretendoWindow *parent)
	: BWindow(BRect(160.0f, 160.0f, 980.0f, 720.0f), 
			"CPU Disassembly", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;
	fView = new CPUDisasmView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// CPUDisasmWindow::~CPUDisasmWindow
//
// Destroys the CPU disassembly debugger window.  Child views are owned by the
// window hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
CPUDisasmWindow::~CPUDisasmWindow()
{
}


// -----------------------------------------------------------------------------
// CPUDisasmWindow::QuitRequested
//
// Notifies the parent window that the CPU disassembly window is closing.  This
// lets the parent resume normal emulation if the disassembler had bootstrapped
// the emulator into debugger-paused mode.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
CPUDisasmWindow::QuitRequested()
{
	if (fParent) {
		fParent->CPUDisasmWindowClosed();
	}

	return true;
}


// -----------------------------------------------------------------------------
// CPUDisasmWindow::JumpToAddress
//
// Jumps the CPU disassembly view to a specific CPU address.
//
// Parameters:
//   address - CPU address to show in the disassembly view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUDisasmWindow::JumpToAddress (uint16 address)
{
	if (fView) {
		fView->JumpToAddress(address);
	}
}

