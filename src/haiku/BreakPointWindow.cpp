
#include "BreakPointWindow.h"
#include "BreakPointView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// BreakPointWindow::BreakPointWindow
//
// Creates the Breakpoint Manager floating debugger window.
//
// Parameters:
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
BreakPointWindow::BreakPointWindow(PretendoWindow *parent)
	: BWindow(BRect(260.0f, 220.0f, 800.0f, 700.0f),
		"Breakpoints",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		fParent(parent)
{
	fView = new BreakPointView(Bounds(), parent);

	AddChild(fView);

	SetPulseRate(100000);
}


// -----------------------------------------------------------------------------
// BreakPointWindow::~BreakPointWindow
//
// Destroys the BreakPoint Manager window and notifies PretendoWindow that this
// debugger window no longer exists.
//
// Clearing the corresponding pointer in PretendoWindow prevents the main window
// from retaining a stale pointer after the user closes the BreakPoint Manager.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
BreakPointWindow::~BreakPointWindow()
{
	if (fParent) {
		fParent->BreakPointWindowClosed();
	}
}

