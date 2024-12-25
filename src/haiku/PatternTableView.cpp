
#include "PatternTableView.h"

#include <cstdio>


PatternTableView::PatternTableView (BRect frame)
	: BView (frame, "pattern_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}


PatternTableView::~PatternTableView()
{
	//delete fMsgChangePalette;

}


void
PatternTableView::AttachedToWindow()
{
	SetViewColor (ui_color(B_PANEL_BACKGROUND_COLOR));
}

void 
PatternTableView::Draw (BRect updateRect)
{
	BView::Draw(updateRect);
}


void
PatternTableView::MessageReceived (BMessage *message)
{
	//switch (message->what) {
			
				
	//	default:
	//		break;
	//}
	
	BView::MessageReceived (message);
}





