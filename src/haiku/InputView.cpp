
#include "InputView.h"


InputView::InputView (BRect frame)
	: BView (frame, "pattern_table", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	
}


InputView::~InputView()
{
//	delete fBitmap;
}


void
InputView::AttachedToWindow()
{
	//fBitmap = new BBitmap(BRect(0, 0,width-1, height-1), B_CMAP8);
	
	BView::AttachedToWindow();
}


void 
InputView::Draw (BRect updateRect)
{		
	BView::Draw(updateRect);
}


void
InputView::MessageReceived (BMessage *message)
{		
	BView::MessageReceived (message);
}



