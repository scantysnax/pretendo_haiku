
#include "PaletteDebugWindow.h"


PaletteDebugWindow::PaletteDebugWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{			
	fParent = parent;
	
	// just for now.
	ResizeTo(600, 600);
	CenterOnScreen();
	SetTitle("Palettes");

	fPaletteDebugView = new PaletteDebugView(Bounds());
	AddChild(fPaletteDebugView);
	
	SetPulseRate(16667);
	
	fSettingsMessage = new BMessage;
	LoadSettings();
}
 

PaletteDebugWindow::~PaletteDebugWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


void
PaletteDebugWindow::MessageReceived (BMessage *message)		
{
	BWindow::MessageReceived (message);
}


bool
PaletteDebugWindow::QuitRequested()
{
	return true;
}


void
PaletteDebugWindow::LoadSettings()
{	
}


void
PaletteDebugWindow::SaveSettings()
{	
}	
