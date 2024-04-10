
#ifndef _VIDEO_SCREEN_H_
#define _VIDEO_SCREEN_H_


#include <WindowScreen.h>

#include "PretendoWindow.h"


const int32 MSG_LEAVE_FULLSCREEN = 'LVFS';


class VideoScreen : public BWindowScreen
{
	public:
			VideoScreen (PretendoWindow *owner);
	virtual ~VideoScreen();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual	bool QuitRequested (void);
	virtual void ScreenConnected (bool connected);
	
	public:
	bool Connected (void) { return fConnected; };
	uint8 *Bits (void) { return fBits; };
	int32 RowBytes (void) { return fRowBytes; };
	int32 PixelWidth (void) { return fPixelWidth; };
	
	private:
	PretendoWindow *fOwner;
	volatile bool fConnected;
	uint8 *fBits;
	int32 fRowBytes;
	int32 fPixelWidth;
};

#endif // _VIDEO_SCREEN_H_
