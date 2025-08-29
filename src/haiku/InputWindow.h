
#ifndef _INPUT_WINDOW_H_
#define _INPUT_WINDOW_H_

#include <Window.h>

#include "InputView.h"


class PretendoWindow;


class InputWindow : public BWindow
{
	public:
			InputWindow(PretendoWindow *parent);
	virtual ~InputWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	
	private:
	PretendoWindow *fParent = nullptr;
	InputView *fInputView = nullptr;
};


#endif // _INPUT_WINDOW_H_
	
	
