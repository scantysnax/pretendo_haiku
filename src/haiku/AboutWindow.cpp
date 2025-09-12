
#include "AboutWindow.h"


AboutWindow::AboutWindow()
	: BWindow (BRect (0,0,0,0), "About Window", B_MODAL_WINDOW, 
		B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{

	ResizeTo(340, 440);
	CenterOnScreen();
	
	fAboutView = new AboutView(Bounds());
	AddChild(fAboutView);
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
	BTextView *textView = new BTextView(r, "text_view", 
		BRect(3, 3, r.Width() - 3, r.Height() - 3), 
		B_FOLLOW_ALL, B_WILL_DRAW);
		
	BFont f = be_plain_font;
	f.SetSize(12.0f);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	textView->SetFontAndColor(&f);	
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	
	BString aboutText;
	aboutText << "A freeware multiplatform Nintendo NES emulator\n\n"
			  << "Version: " << __PRETENDO_VERSION__ << "\n"
			  << "Written by: Evan Teran and Eli Dayan\n"
			  << "Built on: " << __DATE__ << " " << __TIME__ << "\n"
			  << "Built with: gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "."
			  << __GNUC_PATCHLEVEL__ << "\n" 
			  << "\n\"Nintendo\" and \"Nintendo Entertainment System\" are registered "
			   		"trademarks of " "Nintendo Co., Ltd\n\n";

	textView->SetText(aboutText.String());
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(textView);
	
	int32 top = textView->Bounds().bottom + 70;
	
	r.Set(53, top, 53+148, top+18);
	AddChild(new LinkView(r,
		"Pretendo Haiku on GitHub", 
		"https://github.com/scantysnax/pretendo_haiku/tree/haiku-port/"));	
	
	r.Set(53, top+24, 53+109, top+40);
	AddChild(new LinkView(r,
		"Pretendo on GitHub", 
		"https://github.com/eteran/pretendo"));
	
	r.Set(53, 312, 53+68, 330);
	AddChild(new LinkView(r,
		"Eli's website", 
		"http://www.pathtoground.org/"));

	r.Set(53, 326, 53+82, 350);	
	AddChild(new LinkView(r,
		"Evan's website", 
		"http://www.codef00.com"));
	
	BButton *button = new BButton(r, "okay_button", "Okay ", new BMessage ('OKAY'));
	button->ResizeToPreferred();
	button->MakeDefault(true);
	button->MoveTo((Frame().Width() - button->Frame().Width()) / 2, 380);
	AddChild(button);

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
