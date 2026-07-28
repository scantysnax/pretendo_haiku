#include "PPUMemoryWindow.h"

#include "PPUMemoryView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// PPUMemoryWindow::PPUMemoryWindow
//
// Creates the PPU memory debugger window and installs the PPUMemoryView child.
//
// The window is sized to show one full 256-byte PPU memory page: 16 rows of
// 16 bytes, plus the ASCII column and bottom byte inspector.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PPUMemoryWindow::PPUMemoryWindow(PretendoWindow* parent)
	: BWindow(
		BRect(240.0f, 240.0f, 840.0f, 750.0f),
		"PPU Memory",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	)
{
	fParent = parent;

	BRect viewFrame = Bounds();

	fView = new PPUMemoryView(viewFrame, parent);
	AddChild(fView);

	SetPulseRate(16667);
}

// -----------------------------------------------------------------------------
// PPUMemoryWindow::~PPUMemoryWindow
//
// Destroys the PPU memory debugger window.  Child views are owned by the window
// hierarchy and are cleaned up by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PPUMemoryWindow::~PPUMemoryWindow()
{
}


// -----------------------------------------------------------------------------
// PPUMemoryWindow::QuitRequested
//
// Handles close requests for the PPU memory debugger window.  The parent
// PretendoWindow is notified so it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PPUMemoryWindow::QuitRequested()
{
	if (fParent) {
		fParent->PPUMemoryWindowClosed();
	}

	return true;
}

