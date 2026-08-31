
#include "PPUWriteLogWindow.h"


// -----------------------------------------------------------------------------
// PPUWriteLogWindow::PPUWriteLogWindow
//
// Creates the PPU write log debugger window and installs the PPUWriteLogView
// child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PPUWriteLogWindow::PPUWriteLogWindow (PretendoWindow *parent)
	: BWindow (BRect(220.0f, 220.0f, 900.0f, 720.0f),
				"PPU Write Log", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;
	fView = new PPUWriteLogView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// PPUWriteLogWindow::~PPUWriteLogWindow
//
// Destroys the PPU write log debugger window.  Child views are owned by the
// window hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PPUWriteLogWindow::~PPUWriteLogWindow()
{
}


// -----------------------------------------------------------------------------
// PPUWriteLogWindow::QuitRequested
//
// Handles close requests for the PPU write log debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PPUWriteLogWindow::QuitRequested()
{
	if (fParent) {
		fParent->PPUWriteLogWindowClosed();
	}

	return true;
}

