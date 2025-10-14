
#include "InputView.h"

#include <cstdio>

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
	BFont f = be_plain_font;
	f.SetSize(12.0f);
	
	rgb_color c;
	c.red = 255;
	c.green = 255;
	c.blue = 255;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	r.Set(107, 132, 131, 152);
	fUpView = new ButtonTextView(r);
	fUpView->SetViewColor(82,76,63);
	fUpView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fUpView);
	
	r.Set(107, 204, 131, 224);
	fDownView = new ButtonTextView(r);
	fDownView->SetViewColor(82,76,63);
	fDownView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fDownView);
	
	r.Set(74, 164, 98, 186);
	fLeftView = new ButtonTextView(r);
	fLeftView->SetViewColor(82,76,63);
	fLeftView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fLeftView);
	
	r.Set(140, 166, 164, 186);
	fRightView = new ButtonTextView(r);
	fRightView->SetViewColor(82,76,63);
	fRightView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fRightView);
	
	
	r.Set(228, 204, 248, 228);
	fSelectView = new ButtonTextView(r);
	fSelectView->SetViewColor(82,76,63);
	fSelectView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fSelectView);
	
	r.Set(300, 204, 320, 228);
	fStartView = new ButtonTextView(r);
	fStartView->SetViewColor(82,76,63);
	fStartView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fStartView);

	r.Set(390, 200, 410, 220);
	fBView = new ButtonTextView(r);
	fBView->SetViewColor(175, 58, 24);
	fBView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fBView);
	
	r.Set(462, 200, 482, 220);
	fAView = new ButtonTextView(r);
	fAView->SetViewColor(175, 58, 24);
	fAView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fAView);
	
	int32 y = kControllerHeight+(kControllerBorder*2)+8;
	HorizontalSplitter *hs = new HorizontalSplitter(kControllerBorder, y, kControllerWidth);
	AddChild(hs);
	
	int32 top = hs->Frame().bottom+32;
	r.Set(0, top, 0, 0);
	fCancelButton = new BButton(r, "cancel_button", "Cancel", new BMessage(MSG_CANCEL));
	fCancelButton->ResizeToPreferred();
	fCancelButton->SetTarget(this);
	AddChild(fCancelButton);
	
	r.Set(fCancelButton->Frame().right+32, top, 0, 0);
	fDefaultButton = new BButton(r, "default_button", "Default", new BMessage(MSG_DEFAULT));
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetTarget(this);
	AddChild(fDefaultButton);
	
	r.Set(fDefaultButton->Frame().right+32, top, 0, 0);
	fSaveButton = new BButton(r, "save_button", "Save", new BMessage(MSG_SAVE));
	fSaveButton->ResizeToPreferred();
	fSaveButton->SetTarget(this);
	AddChild(fSaveButton);
	
	int32 buttonWidth = fCancelButton->Frame().Width() * 3 + (32*2);
	int32 windowWidth = Bounds().Width();
	int32 x = (windowWidth - buttonWidth) / 2;
	fCancelButton->MoveBy(x, 0);
	fSaveButton->MoveBy(x, 0);
	fDefaultButton->MoveBy(x, 0);

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
	switch (message->what) {
		case MSG_CANCEL:
		OnCancel();
		break;
		
		case MSG_DEFAULT:
		OnDefault();
		break;
		
		case MSG_SAVE:
		OnSave();
		break;
	}	
	
	BView::MessageReceived (message);
}


void
InputView::OnCancel()
{
	puts(__PRETTY_FUNCTION__);
}
	
	
void
InputView::OnDefault()
{
	puts(__PRETTY_FUNCTION__);
}


void 
InputView::OnSave()
{
	puts(__PRETTY_FUNCTION__);
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
	SetAlignment(B_ALIGN_CENTER);
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
		
		default:
		keyString << bytes[0];
		keyString.Capitalize();
		SetText(keyString.String());
	}
	
 	BTextView::KeyDown (bytes, numBytes);
}

