
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
// Handles fullscreen display connection and disconnection.
//
// When connected, configures the 640x480 8-bit display and retrieves the
// framebuffer address, row stride, and pixel size. The owning PretendoWindow
// is notified only after framebuffer initialization succeeds.
//
// Connection failures are reported to the parent window so the fullscreen
// transition can be canceled without relying on a fixed startup delay.
//
// Parameters:
//   connected - true when the fullscreen display has connected.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
VideoScreen::ScreenConnected(bool connected)
{
	fConnected = connected;

	if (connected) {
		const status_t status = SetSpace(B_8_BIT_640x480);

		if (status == B_OK && CardInfo() != nullptr) {
			fBits = reinterpret_cast<uint8 *>(CardInfo()->frame_buffer);

			fRowBytes = CardInfo()->bytes_per_row;
			fBytesPerPixel = CardInfo()->bits_per_pixel / 8;

			if (fBits != nullptr && fRowBytes > 0 && CardInfo()->bits_per_pixel == 8) {
				BMessage ready(PretendoWindow::messages::FULLSCREEN_READY);
				ready.AddPointer("screen", this);
				
				fParent->PostMessage(&ready);
			} else {
				fConnected = false;

				BMessage failed(PretendoWindow::messages::FULLSCREEN_FAILED);

				failed.AddPointer("screen", this);
				fParent->PostMessage(&failed);
			}
		} else {
			fConnected = false;

			BMessage failed(PretendoWindow::messages::FULLSCREEN_FAILED);

			failed.AddPointer("screen", this);
			fParent->PostMessage(&failed);
		}
	} else {
		/*
		 * Invalidate cached framebuffer information.
		 *
		 * A normal fullscreen exit also produces a disconnection,
		 * so do not send FULLSCREEN_FAILED here.
		 */
		fBits = nullptr;
		fRowBytes = 0;
		fBytesPerPixel = 0;
	}

	BWindowScreen::ScreenConnected(connected);
}
