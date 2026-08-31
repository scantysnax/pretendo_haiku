
#include "VideoScreen.h"
#include "PretendoWindow.h"

static status_t error;


// -----------------------------------------------------------------------------
// VideoScreen::VideoScreen
//
// Creates the fullscreen video screen and stores the owning PretendoWindow.
//
// If the BWindowScreen cannot be created successfully, the fullscreen window is
// asked to quit immediately.
//
// Parameters:
//   parent - Owning PretendoWindow.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VideoScreen::VideoScreen (PretendoWindow *parent)
	: BWindowScreen ("Pretendo Fullscreen", B_8_BIT_640x480, &error)
{
	if (error != B_OK) {
		PostMessage (B_QUIT_REQUESTED, this);
		
	}
	
	fParent = parent;
}


// -----------------------------------------------------------------------------
// VideoScreen::~VideoScreen
//
// Hides the fullscreen screen and synchronizes pending screen operations before
// destruction.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
VideoScreen::~VideoScreen()
{
	Hide();
	Sync();
}


// -----------------------------------------------------------------------------
// VideoScreen::MessageReceived
//
// Handles fullscreen window messages.
//
// Escape leaves fullscreen mode by posting LEAVE_FULLSCREEN to the owning
// PretendoWindow.  All messages are then passed to BWindowScreen for normal
// processing.
//
// Parameters:
//   message - Message received by the fullscreen window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VideoScreen::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case B_KEY_DOWN: {
			int8 key;
			
			if (message->FindInt8("byte", 0, &key) == B_OK) {
				switch (key) {
					case B_ESCAPE:
					fParent->PostMessage(PretendoWindow::messages::LEAVE_FULLSCREEN);
				}
			}
		} break;
		
		
		
	}
		
	BWindowScreen::MessageReceived (message);
}


// -----------------------------------------------------------------------------
// VideoScreen::QuitRequested
//
// Marks the fullscreen screen as disconnected and allows the window to close.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the fullscreen window to quit.
// -----------------------------------------------------------------------------
bool
VideoScreen::QuitRequested()
{
	fConnected = false;
	return true;
}


// -----------------------------------------------------------------------------
// VideoScreen::ScreenConnected
//
// Handles connection and disconnection of the fullscreen display.
//
// When connected, the screen is configured for 640x480 8-bit video and the
// framebuffer address, row-byte count, and pixel width are cached for direct
// fullscreen rendering.  If the requested display mode cannot be established,
// the fullscreen window is asked to quit.
//
// The final connection state is also forwarded to BWindowScreen.
//
// Parameters:
//   connected - true when the fullscreen display has been connected.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VideoScreen::ScreenConnected (bool connected)
{
	fConnected = connected;
	
	if (fConnected == true) {
		status_t error;
		error = SetSpace (B_8_BIT_640x480);
		
		if (error != B_OK) {
			PostMessage (B_QUIT_REQUESTED, this);
		}
		
		fBits = reinterpret_cast<uint8 *>(CardInfo()->frame_buffer);
		fRowBytes = CardInfo()->bytes_per_row;
		fPixelWidth = CardInfo()->bits_per_pixel;
	}
		
	BWindowScreen::ScreenConnected (fConnected);
}


