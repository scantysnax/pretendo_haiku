
#include "NameTableWindow.h"
#include "NameTableView.h"

#include <iostream>

NameTableWindow::NameTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fParent = parent;
	fWhich = which;
	fSettingsMessage = new BMessage;
	
	ResizeTo(32*8, 30*8);
	
	switch (which) {
		case 0:
		SetTitle("Name Table 1 (0x2000-0x23ff)");
		break;
		
		case 1:
		SetTitle("Name Table 2 (0x2400-0x27ff)");
		break;
		
		case 2:
		SetTitle("Name Table 3 (0x2800-0x2bff)");
		break;
		
		case 3:
		SetTitle("Name Table 4 (0x2c00-0x2fff)");
		break;
	}
		
	
	fView = new NameTableView(Bounds(), this, which);
	AddChild(fView);
	SetPulseRate(1000000ULL); // one second
	
	LoadSettings();
}


NameTableWindow::~NameTableWindow()
{
	SaveSettings();
	delete fSettingsMessage;
} 


void
NameTableWindow::MessageReceived (BMessage *message)
{	
	BWindow::MessageReceived (message);
}


bool
NameTableWindow::QuitRequested()
{
	return true;
}


void
NameTableWindow::LoadSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
}


void
NameTableWindow::SaveSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
}


