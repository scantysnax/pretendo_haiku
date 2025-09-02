
#include <libxml2/libxml/parser.h>

#include "ROMInfoWindow.h"


ROMInfoWindow::ROMInfoWindow()
	: BWindow(BRect(0, 0, 0, 0), "ROM Info", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
	B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fROMInfoView(NULL)
{
	ResizeTo(720, 320);
	CenterOnScreen();
	
	BRect r = Bounds();
	r.right -= 16;
	fROMInfoView = new ROMInfoView(r);
	BScrollView *sv = new BScrollView("rom_info_sv", fROMInfoView, B_FOLLOW_ALL, 0, false, true);
	AddChild(sv);
}


ROMInfoWindow::~ROMInfoWindow()
{
}




void
ROMInfoWindow::MessageReceived (BMessage *message)
{
	BWindow::MessageReceived (message);
}


bool
ROMInfoWindow::QuitRequested (void)
{
	//Hide();
	//return false;
	
	return true;
}
