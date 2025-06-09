
#ifndef _NAMETABLE_WINDOW_H_
#define _NAMETABLE_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>

#include "NameTableView.h"


class PretendoWindow;


class NameTableWindow : public BWindow
{
	public:
			NameTableWindow(PretendoWindow *parent, int32 which);
	virtual ~NameTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
		
	private:
	NameTableView *fView = nullptr;
	PretendoWindow *fParent = nullptr;
};

#endif // _NAMETABLE_WINDOW_H_
