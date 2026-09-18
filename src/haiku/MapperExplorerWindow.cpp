#include "MapperExplorerWindow.h"

#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// MapperExplorerWindow::MapperExplorerWindow
//
// Creates the Mapper Explorer debugger window and installs the
// MapperExplorerView child.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
MapperExplorerWindow::MapperExplorerWindow (PretendoWindow *parent)
	: BWindow(
		BRect(180.0f, 40.0f, 780.0f, 860.0f),
		"Mapper Explorer",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;

	fView = new MapperExplorerView(Bounds(), parent);
	AddChild(fView);
	
	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// MapperExplorerWindow::~MapperExplorerWindow
//
// Destroys the Mapper Explorer debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
MapperExplorerWindow::~MapperExplorerWindow()
{
}


// -----------------------------------------------------------------------------
// MapperExplorerWindow::QuitRequested
//
// Notifies the owning PretendoWindow that the Mapper Explorer has closed so its
// tool-window and input state can be updated.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
MapperExplorerWindow::QuitRequested()
{
	if (fParent) {
		fParent->MapperExplorerWindowClosed();
	}

	return true;
}

