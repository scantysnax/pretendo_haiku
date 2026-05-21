
#include "PaletteInfoWindow.h"


PaletteInfoWindow::PaletteInfoWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{			
	fParent = parent;
	
	ResizeTo(400, 400);
	CenterOnScreen();
	SetTitle("Palettes");
	
	fPaletteInfoView = new PaletteInfoView(Bounds());
	AddChild(fPaletteInfoView);
	
	//fSettingsMessage = new BMessage;
	//LoadSettings();
}
 

PaletteInfoWindow::~PaletteInfoWindow()
{
	//SaveSettings();
	//delete fSettingsMessage;
}


void
PaletteInfoWindow::MessageReceived (BMessage *message)		
{
	BWindow::MessageReceived (message);
}


bool
PaletteInfoWindow::QuitRequested()
{
	return true;
}


void
PaletteInfoWindow::LoadSettings()
{	
}


void
PaletteInfoWindow::SaveSettings()
{	
}	
