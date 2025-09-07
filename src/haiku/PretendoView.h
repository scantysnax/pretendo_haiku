// PretendoView.h
#ifndef _PRETENDO_VIEW_H_
#define _PRETENDO_VIEW_H_

#include "PretendoWindow.h"


class PretendoView : public BView
{
	public:
	PretendoView (BRect frame, PretendoWindow *parent);
	virtual ~PretendoView();
	
	public:
	virtual void MessageReceived (BMessage *message);

	private:
	PretendoWindow *fParent = nullptr;	
};

#endif // _PRETENDO_VIEW_H_
