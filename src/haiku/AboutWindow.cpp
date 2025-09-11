
#include "AboutWindow.h"


AboutWindow::AboutWindow()
	: BWindow (BRect (0,0,0,0), "About Window", B_MODAL_WINDOW, 
		B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{

	ResizeTo(340, 460);
	CenterOnScreen();
	
	fAboutView = new AboutView(Bounds());
	AddChild(fAboutView);
	
	/*
	BRect r;	
	r.Set(53, 225, 53+105, 238);
	fAboutView->AddChild(new LinkView(r, 
		"Pretendo on GitHub", 
		"https://github.com/eteran/pretendo"));
	
	
	r.Set(53, 243, 53+75, 256);
	fAboutView->AddChild(new LinkView(r,
		"Evan's website", 
		"http://www.codef00.com"));
	

	r.Set(53, 261, 53+63, 274);	
	fAboutView->AddChild(new LinkView(r,
		"Eli's website", 
		"http://www.pathtoground.org/"));	
	*/
}


AboutWindow::~AboutWindow()
{
	
}


bool
AboutWindow::QuitRequested()
{
	return true;
}


void
AboutWindow::MessageReceived (BMessage *message)
{
	if (message->what == 'OKAY') {
		Quit();
	}
	
	BWindow::MessageReceived (message);
}



AboutView::AboutView (BRect frame)
	: BView (frame, "_about_view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fIcon = BTranslationUtils::GetBitmap('bits', "Icon");
	fLogo = BTranslationUtils::GetBitmap('bits', "Logo");
}


AboutView::~AboutView()
{
	
}


void
AboutView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	r.Set(53, 60, Frame().Width()-8, 260);
	BTextView *textview = new BTextView(r, "text_view", 
		BRect(3, 3, r.Width() - 3, r.Height() - 3), 
		B_FOLLOW_ALL, B_WILL_DRAW);
		
	BFont f = be_plain_font;
	f.SetSize(12.0f);
	textview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	textview->SetFontAndColor(&f);	
	textview->MakeEditable(false);
	textview->MakeSelectable(false);
	
	BString aboutText;
	aboutText << "A freeware, multiplatform Nintendo NES emulator\n\n"
			  << "Version: " << __PRETENDO_VERSION__ << "\n"
			  << "Written by: Evan Teran and Eli Dayan\n"
			  << "Built on: " << __DATE__ << " " << __TIME__ << "\n"
			  << "Built with: gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "."
			  << __GNUC_PATCHLEVEL__ << "\n" 
			  << "\n\"Nintendo\" and \"Nintendo Entertainment System\" are registered "
			   		"trademarks of " "Nintendo Co., Ltd\n\n";

	textview->SetText(aboutText.String());
	//textview->SetViewColor(0, 255, 255);
	AddChild(textview);
	
	BButton *button = new BButton(r, "okay_button", "Okay ", new BMessage ('OKAY'));
	button->ResizeToPreferred();
	button->MakeDefault(true);
	button->MoveTo((Frame().Width() - button->Frame().Width()) / 2, 300);
	AddChild(button);
	
	//(new BAlert(0, aboutText.String(), "Okay"))->Go();

	BView::AttachedToWindow();
}


void
AboutView::Draw (BRect updateRect)
{
	BRect r = Bounds(); 
    r.right = 30;
    SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT)); 
  	FillRect(r); 
    SetDrawingMode(B_OP_OVER); 
    DrawBitmap (fIcon, BPoint(18, 6));
    DrawBitmap(fLogo, BPoint((Bounds().Width() - 196) / 2, 11));

    BView::Draw (updateRect);
}
