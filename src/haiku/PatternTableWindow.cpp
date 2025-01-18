
#include "PatternTableWindow.h"


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent)
{
	ResizeTo(kPatternTableWidth*2, kPatternTableHeight*2);
	SetTitle((which == 0) ? "Pattern Table #0" : "Pattern Table #1");
	
	fView = new PatternTableView(Bounds(), which);
	AddChild(fView);	
}


PatternTableWindow::~PatternTableWindow()
{
} 


void
PatternTableWindow::MessageReceived (BMessage *message)
{	
	BWindow::MessageReceived(message);
}


bool
PatternTableWindow::QuitRequested()
{
	Hide();
	return false;
}

