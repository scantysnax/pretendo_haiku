
#include "AboutWindow.h"


AboutWindow::AboutWindow()
	: BWindow (BRect (0,0,0,0), "About Window", B_MODAL_WINDOW, 
		B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{

	ResizeTo(340, 420);
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
			  
	#if defined(__clang__)
		<< "Built with: clang " << __clang_version__ << "\n" 
	#else
		<< "Built with: gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "."
		<< __GNUC_PATCHLEVEL__ << "\n" 
	#endif
	
	<< "\n\"Nintendo\" and \"Nintendo Entertainment System\" are registered "
	<< "trademarks of " "Nintendo Co., Ltd\n\n";

	textView->SetText(aboutText.String());
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(textView);
	
	int32 top = textView->Bounds().bottom + 70;
	int32 left = 53;
	int32 bottom = top+16;
	
	r.Set(left, top, left+112, bottom);
	AddChild(new LinkView(r,
		"Pretendo on GitHub",
		"github.com/eteran/pretendo"));
	
	r.Set(left, top+20, left+84, top+34);
	AddChild(new LinkView(r,
		"Evan's Website", 
		"https://www.codef00.com/"));
	
	top = r.bottom+2;
	r.Set(left, r.bottom-2, 120, top+18);
	AddChild(new LinkView(r,
		"Eli's website", 
		"http://www.pathtoground.org/"));
			
	BButton *button = new BButton(BRect(0,0,0,0), "okay_button", "Okay", new BMessage ('OKAY'));
	button->ResizeToPreferred();
	button->MakeDefault(true);
	button->MoveTo((Frame().Width() - button->Frame().Width()) / 2, 360);
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
