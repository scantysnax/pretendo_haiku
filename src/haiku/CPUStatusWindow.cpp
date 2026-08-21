
#include "CPUStatusWindow.h"


// -----------------------------------------------------------------------------
// CPUStatusWindow::CPUStatusWindow
//
// Creates the floating CPU status window and installs the CPU status view.  The
// window is wide enough for decoded instruction text and stack preview values,
// and tall enough for the instruction/timing panel to fit without clipping.
//
// Parameters:
//   parent - Main emulator window that owns this tool window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUStatusWindow::CPUStatusWindow(PretendoWindow* parent)
	:
	BWindow(
		BRect(280.0f, 280.0f, 800.0f, 770.0f), "CPU Status", 
				B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
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
