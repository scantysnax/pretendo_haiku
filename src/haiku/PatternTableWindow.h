
#ifndef _PATTERNTABLE_WINDOW_H_
#define _PATTERNTABLE_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>

class PretendoWindow;


class PatternTableWindow : public BWindow
{
	public:
			PatternTableWindow(PretendoWindow *parent, uint32 address);
	virtual ~PatternTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	public:
	void DrawTile();
	
	private:
	PretendoWindow *fParent = nullptr;
	
	private:
	uint32 fAddress = 0x0;
	BBitmap *fBitmap = nullptr;
	uint8 *fBitmapBits = nullptr;
};

#endif // _PATTERNTABLE_WINDOW_H_
