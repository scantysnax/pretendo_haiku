
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
	BRect r;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	r.Set(107, 128, 131, 148);
	fUpView = new ButtonTextView(r);
	AddChild(fUpView);
	
	r.Set(107, 204, 131, 224);
	fDownView = new ButtonTextView(r);
	AddChild(fDownView);
	
	r.Set(74, 166, 98, 184);
	fLeftView = new ButtonTextView(r);
	AddChild(fLeftView);
	
	r.Set(140, 166, 164, 184);
	fRightView = new ButtonTextView(r);
	AddChild(fRightView);
	
	r.Set(228, 204, 248, 228);
	fSelectView = new ButtonTextView(r);
	AddChild(fSelectView);
	
	r.Set(300, 204, 320, 228);
	fStartView = new ButtonTextView(r);
	AddChild(fStartView);
	
	r.Set(390, 200, 410, 220);
	fBView = new ButtonTextView(r);
	AddChild(fBView);
	
	r.Set(462, 200, 482,220);
	fAView = new ButtonTextView(r);
	AddChild(fAView);
	
	
	
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
	BTextView::Draw (updateRect);
}



void
ButtonTextView::KeyDown (const char *bytes, int32 numBytes)
{	
	BString keyString;
	
	switch (bytes[0]) {
		case B_UP_ARROW:
		keyString << "↑";
		SetText(keyString.String());
		break;
		
		case B_DOWN_ARROW:
		keyString << "↓";
		SetText(keyString.String());
		break;
		
		case B_LEFT_ARROW:
		keyString << "←";
		SetText(keyString.String());
		break;
		
		case B_RIGHT_ARROW:
		keyString << "→";
		SetText(keyString.String());
		break;
		
		case('e'):
			(new BAlert(0, "e", "Okay"))->Go();
		break;
	}
	
 	BTextView::KeyDown (bytes, numBytes);
}

