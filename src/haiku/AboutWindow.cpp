

#include <Application.h>
#include <Roster.h>
#include <String.h>
#include <TranslationUtils.h>
#include <View.h>
#include <TextView.h>

#include "AboutWindow.h"
#include "LinkView.h"

#include "version.h"


AboutView::AboutView(BRect frame)
	: BView (frame, "_about_view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fIcon = BTranslationUtils::GetBitmap('bits', "About Icon");
	fLogo = BTranslationUtils::GetBitmap('bits', "Pretendo Logo");
}


AboutView::~AboutView()
{
}

void
AboutView::AttachedToWindow (void)
{
	SetViewColor (ui_color(B_PANEL_BACKGROUND_COLOR));
	
	
	BRect r (0, 0, 0, 0);
	BButton *button = new BButton (r, "_okay", "Okay ", new BMessage ('OKAY'));
	button->ResizeToPreferred();
	button->MakeDefault(true);
	r = Bounds();
	button->MoveTo ((r.Width() - button->Frame().Width()) / 2 , 270);
	
	r.Set(53, 50, r.right - 10, r.bottom - 100);
	BTextView *textview = new BTextView (r, "_textview", 
		BRect(3, 3, r.Width() - 3, r.Height() - 3), 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		
	BFont f = be_plain_font;
	f.SetSize(11.0f);
	textview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	textview->SetFontAndColor(&f);	
	textview->MakeEditable(false);
	textview->MakeSelectable(false);
	
	BString aboutText;
	BString desc;
	BString pretendoVersion;
	BString builtOn;
	BString builtWith;
	BString writtenBy;
	BString haikuPortBy;
	BString trademarks;
	
	desc << "A freeware, portable Nintendo NES emulator\n\n";
	pretendoVersion << "Version: " << __PRETENDO_VERSION__ << "\n";
	writtenBy << "Written by: Evan Teran and Eli Dayan\n";
	haikuPortBy << "Haiku port written by: Eli Dayan\n";
	builtOn << "Built on: " << __DATE__ << " " << __TIME__ << "\n";
	builtWith << "Built with: gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
	trademarks << "\n\"Nintendo\" and \"Nintendo Entertainment System\" are registered"				" trademarks of " "Nintendo Co., Ltd\n\n";
	
	aboutText += desc;
	aboutText += pretendoVersion;
	aboutText += writtenBy;
	aboutText += haikuPortBy;
	aboutText += builtOn;
	aboutText += builtWith;
	aboutText += trademarks;

	textview->SetText(aboutText.String());
	textview->ResizeToPreferred();
	
	AddChild(textview);
	textview->ResizeBy(0,-20);
	button->MoveBy(0, -10);
	AddChild(button);

	BView::AttachedToWindow();
}


void
AboutView::Draw (BRect updateRect)
{
	BRect r = Bounds(); 
    r.right = 30;
    SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT)); 
  	FillRect (r); 
    SetDrawingMode(B_OP_OVER); 
    DrawBitmap (fIcon, BPoint(18, 6));
    DrawBitmap(fLogo, BPoint((Bounds().Width() - 196) / 2, 11));

    BView::Draw(updateRect);
}


AboutWindow::AboutWindow()
	: BWindow (BRect (0,0,0,0), "About Window", B_MODAL_WINDOW, 
		B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{

	ResizeTo (340, 300);
	CenterOnScreen();
	
	fAboutView = new AboutView(Bounds());
	AddChild(fAboutView);
	
	BRect r;	
	r.Set(53, 165, 200, 200);
	fAboutView->AddChild(new LinkView(r, "Pretendo Haiku on GitHub", 
		"https://github.com/scantysnax/pretendo_haiku/tree/haiku-port/src"));
	
	r.Set(53, 185, 140, 220);
	fAboutView->AddChild(new LinkView(r, "Evan's website", "http://www.codef00.com"));
	
	r.Set(53, 205, 200, 240);	
	fAboutView->AddChild(new LinkView(r, "Eli's website", 
		"http://www.pathtoground.org/"));
	
}


AboutWindow::~AboutWindow()
{
}


bool
AboutWindow::QuitRequested (void)
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

