
#include "InputView.h"

#include <Alert.h>


InputView::InputView (BRect frame)
	: BView(frame, "input_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
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
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	r.Set(32, 400, 48, 420);
	fUpView = new ButtonTextView(r);
	AddChild(fUpView);
	
	BView::AttachedToWindow();
}


void 
InputView::Draw (BRect updateRect)
{
	BRect r(0, 0, kControllerWidth, kControllerHeight);
	r.OffsetTo(kControllerBorder, kControllerBorder);
	DrawBitmap(fControllerBitmap, r);		
	
	BView::Draw (updateRect);
}


void
InputView::MessageReceived (BMessage *message)
{		
	BView::MessageReceived (message);
}


ButtonTextView::ButtonTextView (BRect frame)
	: BTextView(frame, "button_text_view", BRect(0,0,0,0), B_FOLLOW_ALL, B_WILL_DRAW)
{
	
}


ButtonTextView::~ButtonTextView()
{

}


void
ButtonTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	
	BRect r(Bounds());
	SetTextRect(BRect(2, 2, r.Width() - 2, r.Height() - 2));
	SetMaxBytes(1);
}


void
ButtonTextView::Draw (BRect updateRect)
{	
	BTextView::Draw(updateRect);
}


void
ButtonTextView::KeyDown (const char *bytes, int32 numBytes)
{
	BString keyString;
	keyString << bytes[0];
	
	keyString.Capitalize();
	SetText(keyString.String());
	
	BTextView::KeyDown (bytes, numBytes);
}

