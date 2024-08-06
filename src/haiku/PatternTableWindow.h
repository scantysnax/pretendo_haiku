
#ifndef _PATTERNTABLE_WINDOW_H_
#define _PATTERNTABLE_WINDOW_H_

#include <Window.h>

class PretendoWindow;


class PatternTableWindow : public BWindow
{
	public:
			PatternTableWindow(PretendoWindow *parent);
	virtual ~PatternTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	private:
	PretendoWindow *fParent;

};

#endif // _PATTERNTABLE_WINDOW_H_
