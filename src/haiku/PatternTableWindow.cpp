
#include "PatternTableWindow.h"

#include "Nes.h"
#include "Cart.h"


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, uint32 address)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent)
{
	ResizeTo(128*2, 128*2);
	SetTitle( (address & 0x1000) ? "Pattern Table #1" : "Pattern Table #0");
	
	fView = new PatternTableView(Bounds(), address);
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

