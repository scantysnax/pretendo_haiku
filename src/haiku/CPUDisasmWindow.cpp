
#include "CPUDisasmWindow.h"

#include "CPUDisasmView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// CPUDisasmWindow::CPUDisasmWindow
//
// Creates the CPU disassembly debugger window and installs the CPUDisasmView
// child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
CPUDisasmWindow::CPUDisasmWindow (PretendoWindow *parent)
	: BWindow(
	BRect(260.0f, 260.0f, 780.0f, 720.0f),
		"CPU Disassembly",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	)
{
	fParent = parent;

	BRect viewFrame = Bounds();

	fView = new CPUDisasmView(viewFrame, parent);
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
// Handles close requests for the CPU disassembly debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
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

