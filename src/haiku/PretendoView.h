// PretendoView.h

#ifndef _PRETENDO_VIEW_H_
#define _PRETENDO_VIEW_H_

#include <Entry.h>
#include <Path.h>

#include "PretendoWindow.h"


class PretendoView : public BView
{
	public:
	PretendoView (BRect frame, PretendoWindow *parent);
	virtual ~PretendoView();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void KeyUp (const char *bytes, int32 numBytes);
	virtual void MouseDown (BPoint where);
	
	public:
	void SetDisplayBitmap (BBitmap *bitmap);
	void CaptureLastFrame (BBitmap *source);
	void ClearLastFrame();
	
	private:
	PretendoWindow *fParent = nullptr;
	
	private:
	BBitmap *fDisplayBitmap = nullptr;   // Not owned.
	BBitmap *fLastFrameBitmap = nullptr; // Owned.
		
};

#endif // _PRETENDO_VIEW_H_
