
#include "PPUStatusWindow.h"


// -----------------------------------------------------------------------------
// PPUStatusWindow::PPUStatusWindow
//
// Creates the PPU status debugger window and installs the PPUStatusView child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PPUStatusWindow::PPUStatusWindow(PretendoWindow *parent)
	: BWindow(
		BRect(260.0f, 260.0f, 640.0f, 885.0f),
		"PPU Status", 
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;

	BRect viewFrame = Bounds();

	fView = new PPUStatusView(viewFrame, parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// PPUStatusWindow::~PPUStatusWindow
//
// Destroys the PPU status debugger window.  Child views are owned by the window
// hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PPUStatusWindow::~PPUStatusWindow()
{
}


// -----------------------------------------------------------------------------
// PPUStatusWindow::QuitRequested
//
// Handles close requests for the PPU status debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PPUStatusWindow::QuitRequested()
{
	if (fParent) {
		fParent->PPUStatusWindowClosed();
	}

	return true;
}

