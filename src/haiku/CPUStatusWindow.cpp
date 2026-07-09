
#include "CPUStatusWindow.h"

#include "CPUStatusView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// CPUStatusWindow::CPUStatusWindow
//
// Creates the CPU status debugger window and installs the CPUStatusView child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
CPUStatusWindow::CPUStatusWindow (PretendoWindow *parent)
	:
	BWindow(
		BRect(280.0f, 280.0f, 660.0f, 720.0f),
		"CPU Status",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	)
{
	fParent = parent;

	BRect viewFrame = Bounds();

	fView = new CPUStatusView(viewFrame, parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// CPUStatusWindow::~CPUStatusWindow
//
// Destroys the CPU status debugger window.  Child views are owned by the window
// hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
CPUStatusWindow::~CPUStatusWindow()
{
}


// -----------------------------------------------------------------------------
// CPUStatusWindow::QuitRequested
//
// Handles close requests for the CPU status debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
CPUStatusWindow::QuitRequested()
{
	if (fParent) {
		fParent->CPUStatusWindowClosed();
	}
	
	return true;
}
