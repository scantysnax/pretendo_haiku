
#include "PatternTableWindow.h"

PatternTableWindow::PatternTableWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "Adjust Palette", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent)
{
	ResizeTo(148, 148);
	CenterOnScreen();
	
	BView *backView = new BView(Bounds(), "_back_view", B_FOLLOW_ALL, 0);
	backView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild (backView);
	
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


