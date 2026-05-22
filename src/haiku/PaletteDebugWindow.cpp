
#include "PaletteDebugWindow.h"


PaletteDebugWindow::PaletteDebugWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "Palette Viewer", B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{			
	fParent = parent;
	fSettingsMessage = new BMessage;
	
	const float kWindowW = 430.0f;
	const float kWindowH = 540.0f;

	ResizeTo(kWindowW, kWindowH);
	MoveTo(200.0f, 200.0f);

	
	BRect viewFrame(
		0.0f,
		0.0f,
		kWindowW - 1.0f,
		kWindowH - 1.0f
	);

	fView = new PaletteDebugView(viewFrame, fParent);
	AddChild(fView);

	SetPulseRate(16667);
	
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
