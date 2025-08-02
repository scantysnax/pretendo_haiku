// PretendoView.h
#ifndef _PRETENDO_VIEW_H_
#define _PRETENDO_VIEW_H_

#include <View.h>

#include "PretendoWindow.h"

class PretendoView : public BView
{
	public:
	PretendoView (BRect frame, PretendoWindow *parent);
	virtual ~PretendoView();
	
	public:
	virtual void MessageReceived (BMessage *message);

	private:
	PretendoWindow *fParent;	
};

#endif // _PRETENDO_VIEW_H_
