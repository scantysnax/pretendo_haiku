
#include "PatternTableWindow.h"

#include "Nes.h"
#include "Cart.h"


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, uint32 address)
	: BWindow(BRect(200, 200, 0, 0), "0", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent)
{
	if (address == 0x0) {
		SetTitle ("Pattern Table 0");
	} else if (address == 0x1000) {
		SetTitle("Pattern Table 1");
	}
	
	
	ResizeTo(255, 255);
	
	//CenterOnScreen();
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

