
#include "InputWindow.h"


InputWindow::InputWindow(PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
		fParent(parent)
{
	ResizeTo(kControllerWidth+kControllerBorder*2, 500);
	CenterOnScreen();
	SetTitle("Configure Input");
	
	fInputView = new InputView(Bounds());
	AddChild(fInputView);
}


InputWindow::~InputWindow()
{
}


void
InputWindow::MessageReceived (BMessage *message)		
{
	BWindow::MessageReceived(message);
}


bool
InputWindow::QuitRequested()
{
	return true;
}
