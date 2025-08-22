
#ifndef _INPUT_WINDOW_H_
#define _INPUT_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>

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
	InputView *fView = nullptr;
	PretendoWindow *fParent = nullptr;
};


#endif // _INPUT_WINDOW_H_
	
	
