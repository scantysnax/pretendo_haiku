
#include "ZeroPageWindow.h"


// -----------------------------------------------------------------------------
// ZeroPageWindow::ZeroPageWindow
//
// Creates the Zero Page debugger window.
//
// Parameters:
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ZeroPageWindow::ZeroPageWindow(PretendoWindow* parent)
	:
	BWindow(
		BRect(220.0f, 220.0f, 760.0f, 810.0f),
		"Zero Page",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	),
	fParent(parent)
{
	fView = new ZeroPageView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(100000);
}


// -----------------------------------------------------------------------------
// ZeroPageWindow::~ZeroPageWindow
//
// Destroys the Zero Page debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ZeroPageWindow::~ZeroPageWindow()
{
}


// -----------------------------------------------------------------------------
// ZeroPageWindow::QuitRequested
//
// Notifies the parent window that the Zero Page debugger has closed.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
ZeroPageWindow::QuitRequested()
{
	if (fParent) {
		fParent->ZeroPageWindowClosed();
	}

	return true;
}

