
#include "StackWindow.h"


// -----------------------------------------------------------------------------
// StackWindow::StackWindow
//
// Creates the Stack debugger window.
//
// Parameters:
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
StackWindow::StackWindow(PretendoWindow *parent)
	:
	BWindow(
		BRect(240.0f, 240.0f, 780.0f, 884.0f),
		"Stack",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	),
	fParent(parent)
{
	fView = new StackView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(100000);
}


// -----------------------------------------------------------------------------
// StackWindow::~StackWindow
//
// Destroys the Stack debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
StackWindow::~StackWindow()
{
}


// -----------------------------------------------------------------------------
// StackWindow::QuitRequested
//
// Notifies the parent window that the Stack debugger has closed.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
StackWindow::QuitRequested()
{
	if (fParent) {
		fParent->StackWindowClosed();
	}

	return true;
}

