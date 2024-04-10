// PretendoView.h
#ifndef _PRETENDO_VIEW_H_
#define _PRETENDO_VIEW_H_

#include <View.h>
#include <Bitmap.h>

#include "PretendoWindow.h"

class PretendoView : public BView
{
	public:
	PretendoView (BRect frame, PretendoWindow *parent);
	virtual ~PretendoView();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual void DrawBitmap (BBitmap *bitmap);
	virtual void MouseMoved (BPoint point, uint32 transit, BMessage *message);
	
	private:
	PretendoWindow *fParent;	
};

#endif // _PRETENDO_VIEW_H_
