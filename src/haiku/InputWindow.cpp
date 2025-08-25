
#include "InputWindow.h"


InputWindow::InputWindow(PretendoWindow *parent)
	: BWindow(BRect(300, 300, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
		fParent(parent)
{
	ResizeTo(300, 300);
	SetTitle("Configure Input");
	
	fBackgroundView = new BView(Bounds(),"_input_bg_view", B_FOLLOW_ALL, 0);
	AddChild(fBackgroundView);
	fBackgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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
