
#include "VideoScreen.h"
#include "PretendoWindow.h"

static status_t error;


VideoScreen::VideoScreen (PretendoWindow *parent)
	: BWindowScreen ("Pretendo Fullscreen", B_8_BIT_640x480, &error)
{
	if (error != B_OK) {
		PostMessage (B_QUIT_REQUESTED, this);
	}
	
	fParent = parent;
}


VideoScreen::~VideoScreen()
{
	Hide();
	Sync();
}


void
VideoScreen::MessageReceived (BMessage *message)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
		
		if ((message->FindInt8("byte", 0, &key) == B_OK) && key == B_ESCAPE) {
			fParent->PostMessage(MSG_LEAVE_FULLSCREEN);
		}
	}
		
	BWindowScreen::MessageReceived (message);
}


bool
VideoScreen::QuitRequested()
{
	fConnected = false;
	return true;
}


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
