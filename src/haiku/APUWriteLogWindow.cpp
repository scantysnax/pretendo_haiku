#include "APUWriteLogWindow.h"


// -----------------------------------------------------------------------------
// APUWriteLogWindow::APUWriteLogWindow
//
// Creates the APU write log debugger window and installs the APUWriteLogView
// child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUWriteLogWindow::APUWriteLogWindow (PretendoWindow *parent)
	: BWindow (BRect(220.0f, 220.0f, 900.0f, 720.0f),
				"APU Write Log", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;
	fView = new APUWriteLogView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// APUWriteLogWindow::~APUWriteLogWindow
//
// Destroys the APU write log debugger window.  Child views are owned by the
// window hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUWriteLogWindow::~APUWriteLogWindow()
{
}


// -----------------------------------------------------------------------------
// APUWriteLogWindow::QuitRequested
//
// Handles close requests for the APU write log debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUWriteLogWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUWriteLogWindowClosed();
	}

	return true;
}

