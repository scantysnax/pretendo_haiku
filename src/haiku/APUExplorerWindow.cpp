
#include "APUExplorerWindow.h"


// -----------------------------------------------------------------------------
// APUExplorerWindow::APUExplorerWindow
//
// Creates the APU Explorer debugger window and installs the APUExplorerView
// child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerWindow::APUExplorerWindow (PretendoWindow *parent)
	: BWindow (BRect(180.0f, 40.0f, 1060.0f, 770.0f),
				"APU Explorer", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;

	fView = new APUExplorerView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// APUExplorerWindow::~APUExplorerWindow
//
// Destroys the APU Explorer debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerWindow::~APUExplorerWindow()
{
}


// -----------------------------------------------------------------------------
// APUExplorerWindow::QuitRequested
//
// Notifies the owning PretendoWindow that the APU Explorer has closed so its
// tool-window and input state can be updated.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUExplorerWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUExplorerWindowClosed();
	}

	return true;
}

