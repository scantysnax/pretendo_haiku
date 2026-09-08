#include "APUScopeWindow.h"

#include "APUScopeView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// APUScopeWindow::APUScopeWindow
//
// Constructs the APU Scope debugger window.
//
// Parameters:
//   parent - Main Pretendo application window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUScopeWindow::APUScopeWindow (PretendoWindow *parent)
	: BWindow(BRect(200.0f, 120.0f, 980.0f, 700.0f), "APU Scope", B_FLOATING_WINDOW_LOOK,
	 		  B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fParent(parent)
{
	fView = new APUScopeView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// APUScopeWindow::~APUScopeWindow
//
// Destroys the APU Scope debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUScopeWindow::~APUScopeWindow()
{
}


// -----------------------------------------------------------------------------
// APUScopeWindow::QuitRequested
//
// Notifies the main Pretendo window that the APU Scope window has closed.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUScopeWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUScopeWindowClosed();
	}

	return true;
}

