
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
	BView::AttachedToWindow();
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	r.Set(32, 400, 80, 420);
	fUpTextView = new BTextView (r, "up_view", 
		BRect(3, 3, r.Width() - 3, r.Height() - 3), B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	
	BFont f = be_plain_font;
	f.SetSize(11.0f);
	fUpTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fUpTextView->SetFontAndColor(&f);		
	AddChild(fUpTextView);
}


void 
InputView::Draw (BRect updateRect)
{
	BRect r(0, 0, kControllerWidth, kControllerHeight);
	r.OffsetTo(16, 16);
	DrawBitmap(fControllerBitmap, r);		
	
	BView::Draw(updateRect);
}


void
InputView::MessageReceived (BMessage *message)
{		
	BView::MessageReceived (message);
}



