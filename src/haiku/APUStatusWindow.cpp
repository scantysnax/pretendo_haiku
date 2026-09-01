#include "APUStatusWindow.h"

#include "APUStatusView.h"
#include "PretendoWindow.h"


namespace {

constexpr float kWindowWidth = 710.0f;
constexpr float kWindowHeight = 470.0f;

}


// -----------------------------------------------------------------------------
// APUStatusWindow::APUStatusWindow
//
// Creates the APU status debugger window and installs its status view.
//
// Parameters:
//   mainWindow - Main Pretendo emulator window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUStatusWindow::APUStatusWindow(PretendoWindow *mainWindow)
	: BWindow(
		BRect(
			100.0f,
			100.0f,
			100.0f + kWindowWidth,
			100.0f + kWindowHeight),
		"APU Status",
		B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	  fMainWindow(mainWindow)
{
	BRect bounds = Bounds();

	fView = new APUStatusView(bounds);
	AddChild(fView);
}


// -----------------------------------------------------------------------------
// APUStatusWindow::QuitRequested
//
// Notifies the main Pretendo window that the APU status debugger is closing so
// it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUStatusWindow::QuitRequested()
{
	if (fMainWindow) {
		fMainWindow->APUStatusWindowClosed();
	}

	return true;
}
