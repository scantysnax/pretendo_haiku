
#ifndef _VIDEO_SCREEN_H_
#define _VIDEO_SCREEN_H_

#include <WindowScreen.h>


class PretendoWindow;


class VideoScreen : public BWindowScreen
{
	public:
			VideoScreen (PretendoWindow *parent);
	virtual ~VideoScreen();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual	bool QuitRequested();
	virtual void ScreenConnected (bool connected);
	
	public:
	bool Connected() const {
		return fConnected;
	}
	
	uint8 *Bits() const { 
		return fBits;
	}
	
	int32 RowBytes() const {
		return fRowBytes;
	}
	
	int32 PixelWidth() const { 
		return fPixelWidth;
	}
	
	private:
	PretendoWindow *fParent = nullptr;
	volatile bool fConnected = false;
	uint8 *fBits = nullptr;
	int32 fRowBytes = 0;
	int32 fPixelWidth = 0;
};

#endif // _VIDEO_SCREEN_H_
