
#include "PatternTableWindow.h"

#include <String.h>


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE)
{
	fParent = parent;
	
	ResizeTo(PatternTableView::screen_size::WIDTH*2, 													 PatternTableView::screen_size::HEIGHT*2);
	SetTitle((which == 0) ? 
							"Pattern Table 1 (8x8)" 
						: 	"Pattern Table 2 (8x8)"
	);
	
	fView = new PatternTableView(Bounds(), which);
	AddChild(fView);	
	
	SetPulseRate(1000000ULL); // one second
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


void
PatternTableWindow::Zoom (BPoint origin, float width, float height)
{
	(void)origin;
	(void)width;
	(void)height;
	
	BString title(Title());
	
	if (fView->ViewMode() == PatternTableView::view_mode::MODE_8x8) {
		fView->SetViewMode(PatternTableView::view_mode::MODE_8x16);
		title.ReplaceFirst("(8x8)", "(8x16)");
	} else {
		fView->SetViewMode(PatternTableView::view_mode::MODE_8x8);
		title.ReplaceFirst("(8x16)", "(8x8)");
	}
	
	SetTitle(title.String());	
}
	
