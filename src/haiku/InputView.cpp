
#include "InputView.h"


InputView::InputView (BRect frame)
	: BView (frame, "input_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fControllerBitmap = BTranslationUtils::GetBitmap('bits', "Controller");
}


InputView::~InputView()
{
	delete fControllerBitmap;
}


void
InputView::AttachedToWindow()
{
	//fBitmap = new BBitmap(BRect(0, 0,width-1, height-1), B_CMAP8);
	
	DrawBitmap(fControllerBitmap);
	
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



