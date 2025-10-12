
#include "PatternTableWindow.h"


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fParent = parent;
	
	ResizeTo(PatternTableView::screen_size::WIDTH*2, 													 PatternTableView::screen_size::HEIGHT*2);
	SetTitle((which == 0) ? 
							"Pattern Table 1 (0x0-0xfff)" 
						: 	"Pattern Table 2 (0x1000-0x1fff)"
	);
	
	fView = new PatternTableView(Bounds(), which);
	AddChild(fView);	
}


PatternTableWindow::~PatternTableWindow()
{
	
} 


void
PatternTableWindow::MessageReceived (BMessage *message)
{	
	BWindow::MessageReceived (message);
}


bool
PatternTableWindow::QuitRequested()
{
	return true;
}

