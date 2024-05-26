
#include "VideoScreen.h"
#include "PretendoWindow.h"
#include <cstdio>

static status_t error;


VideoScreen::VideoScreen (PretendoWindow *owner)
	: BWindowScreen ("Pretendo Fullscreen", B_8_BIT_640x480, &error),
	fOwner (owner),
	fBits(NULL),
	fRowBytes(0)
{
	if (error != B_OK) {
		PostMessage (B_QUIT_REQUESTED, this);
	}
}


VideoScreen::~VideoScreen()
{
	Hide();
	Sync();
}


void
VideoScreen::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case B_KEY_DOWN:
			int8 key;
			
			if ((message->FindInt8 ("byte", 0, &key) == B_OK) && key == B_ESCAPE) {
				fOwner->PostMessage (MSG_LEAVE_FULLSCREEN);
			}
			break;
	}
	
	BWindowScreen::MessageReceived (message);
}


bool
VideoScreen::QuitRequested (void)
{
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
