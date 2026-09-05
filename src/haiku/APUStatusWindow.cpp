#include "APUStatusWindow.h"

#include "APUStatusView.h"
#include "PretendoWindow.h"


namespace {

constexpr float kWindowWidth = 850.0f;
constexpr float kWindowHeight = 620.0f;

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
	: BWindow(BRect(100.0f, 100.0f, 100.0f + kWindowWidth, 100.0f + kWindowHeight),"APU Status",
			   B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE), 		  
	  
	  fMainWindow(mainWindow)
{
	BRect bounds = Bounds();

	fView = new APUStatusView(bounds);
	AddChild(fView);
}


// -----------------------------------------------------------------------------
// APUStatusWindow::~APUStatusWindow
//
// Destroys the APU debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUStatusWindow::~APUStatusWindow()
{
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


// -----------------------------------------------------------------------------
// APUStatusWindow::MessageReceived
//
// Handles messages sent to the APU Status window.  Channel enable/disable
// messages from the APU Status checkboxes are forwarded to PretendoWindow so
// the existing audio menu handlers remain the single point responsible for
// muting and unmuting APU channels.
//
// Parameters:
//   message - Message received by the window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case PretendoWindow::messages::ENABLE_SQ1:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_SQ1);
			}
			break;
		
		case PretendoWindow::messages::ENABLE_SQ2:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_SQ2);
			}
			break;
			
		case PretendoWindow::messages::ENABLE_TRI:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_TRI);
			}
			break;
			
		case PretendoWindow::messages::ENABLE_NOISE:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_NOISE);
			}
			break;
		
		case PretendoWindow::messages::ENABLE_DMC:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_DMC);
			}
			break;	
	}	
	
	BWindow::MessageReceived(message);
}

