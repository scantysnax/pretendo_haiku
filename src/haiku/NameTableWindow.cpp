
#include "NameTableWindow.h"


NameTableWindow::NameTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent)
{
	ResizeTo(256, 240);
	//SetTitle((which == 0) ? "Pattern Table #0" : "Pattern Table #1");
	
	fView = new NameTableView(Bounds(), which);
	AddChild(fView);	
}


NameTableWindow::~NameTableWindow()
{
} 


void
NameTableWindow::MessageReceived (BMessage *message)
{	
	BWindow::MessageReceived(message);
}


bool
NameTableWindow::QuitRequested()
{
	return true;
}
