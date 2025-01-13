
#ifndef _PATTERNTABLE_WINDOW_H_
#define _PATTERNTABLE_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>

#include "PatternTableView.h"

class PretendoWindow;


class PatternTableWindow : public BWindow
{
	public:
			PatternTableWindow(PretendoWindow *parent, uint32 address);
	virtual ~PatternTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
		
	private:
	PatternTableView *fView = nullptr;
	PretendoWindow *fParent = nullptr;
	

};

#endif // _PATTERNTABLE_WINDOW_H_
