
#include "InputView.h"
#include "Settings.h"

#include <iostream>
#include <Alert.h>
#include <cstdio>


InputView::InputView (BRect frame)
	: BView(frame, "input_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fControllerBitmap = BTranslationUtils::GetBitmap('bits', "Controller");	
	
	//int32 i;
	//for (i = 0; i < 8; i++) {
//		memset(&fKeys[i], 0, sizeof(fKeys[i]));
//	}
		
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
	fUpView = new KeyTextView(r);
	fUpView->SetViewColor(82, 76, 63);
	fUpView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fUpView);
	
	r.Set(107, 204, 131, 224);
	fDownView = new KeyTextView(r);
	fDownView->SetViewColor(82, 76, 63);
	fDownView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fDownView);
	
	r.Set(74, 164, 98, 186);
	fLeftView = new KeyTextView(r);
	fLeftView->SetViewColor(82, 76, 63);
	fLeftView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fLeftView);
	
	r.Set(140, 166, 164, 186);
	fRightView = new KeyTextView(r);
	fRightView->SetViewColor(82, 76, 63);
	fRightView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fRightView);
	
	
	r.Set(228, 204, 248, 228);
	fSelectView = new KeyTextView(r);
	fSelectView->SetViewColor(82, 76, 63);
	fSelectView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fSelectView);
	
	r.Set(300, 204, 320, 228);
	fStartView = new KeyTextView(r);
	fStartView->SetViewColor(82, 76, 63);
	fStartView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fStartView);

	r.Set(390, 200, 410, 220);
	fBView = new KeyTextView(r);
	fBView->SetViewColor(175, 58, 24);
	fBView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fBView);
	
	r.Set(462, 200, 482, 220);
	fAView = new KeyTextView(r);
	fAView->SetViewColor(175, 58, 24);
	fAView->SetFontAndColor(&f, B_FONT_ALL, &c);
	AddChild(fAView);
	
	int32 y = controller_size::HEIGHT+(controller_size::BORDER*2)+8;
	HorizontalSplitter *hs = new HorizontalSplitter(controller_size::BORDER, y, controller_size::WIDTH);
	AddChild(hs);
	
	int32 top = hs->Frame().bottom+32;
	r.Set(0, top, 0, 0);
	fCancelButton = new BButton(r, "cancel_button", "Cancel", new BMessage(messages::CANCEL));
	fCancelButton->ResizeToPreferred();
	fCancelButton->SetTarget(this);
	AddChild(fCancelButton);
	
	r.Set(fCancelButton->Frame().right+32, top, 0, 0);
	fDefaultButton = new BButton(r, "default_button", "Default", new BMessage(messages::DEFAULT));
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetTarget(this);
	AddChild(fDefaultButton);
	
	r.Set(fDefaultButton->Frame().right+32, top, 0, 0);
	fSaveButton = new BButton(r, "save_button", "Save", new BMessage(messages::SAVE));
	fSaveButton->ResizeToPreferred();
	fSaveButton->SetTarget(this);
	AddChild(fSaveButton);
	
	int32 buttonWidth = fCancelButton->Frame().Width() * 3 + (32*2);
	int32 windowWidth = Bounds().Width();
	int32 x = (windowWidth - buttonWidth) / 2;
	fCancelButton->MoveBy(x, 0);
	fSaveButton->MoveBy(x, 0);
	fDefaultButton->MoveBy(x, 0);
	
	// load inputs here
	
	BView::AttachedToWindow();
}


void 
InputView::Draw (BRect updateRect)
{
	BRect r(0, 0, controller_size::WIDTH-1, controller_size::HEIGHT-1);
	r.OffsetTo(controller_size::BORDER, controller_size::BORDER);
	DrawBitmap(fControllerBitmap, r);
	
	BView::Draw (updateRect);
}


void
InputView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case messages::CANCEL:
		OnCancel();
		break;
		
		case messages::DEFAULT:
		OnDefault();
		break;
		
		case messages::SAVE:
		OnSave();
		break;
	}	
	
	BView::MessageReceived (message);
}


void
InputView::OnCancel()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	/*
	uint8 keys[] = {
		13, 2, 4, 2, 13, 5, 4
	};
	
	size_t size = sizeof(keys) / sizeof(*keys);
	std::pmr::unordered_set<uint8> duplicates = FindDuplicates(keys, size);
   	
   	for (int32 i: duplicates) {
   		std::cout << i << std::endl;
   	}
  	*/
  	
  	/*
  	uint8 keys[] = { 
  		fUpKey,			// 1e
  		fDownKey,		// 1f
  		fLeftKey,		// 1c
  		fRightKey,		// 1d
  		fSelectKey,		// 41
  		fStartKey,		// 53
  		fBKey,			// 5a
  		fAKey			// 58
  	};
  	
  	int32 size = sizeof(keys) / sizeof(*keys);
  	
  	printf("size: %d\n", size);
  	
  	for (int32 i = 0; i < size; i++) {
  		printf("%02x\n", keys[i]);
  	}
  	
  	*/
}

	
void
InputView::OnDefault()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	SetDefaultKeys();
}


void 
InputView::OnSave()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
}


void
InputView::SetDefaultKeys()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	/*
	fUpKey = B_UP_ARROW;	
	fDownKey = B_DOWN_ARROW;
	fLeftKey = B_LEFT_ARROW;
	fRightKey = B_RIGHT_ARROW;
	fSelectKey = 'A';
	fStartKey = 'S';
	fBKey = 'Z';
	fAKey = 'X';
	*/
	
	printf("up key: %02x\n", fUpKey);
	printf("down key: %02x\n", fDownKey);
	printf("left key: %02x\n", fLeftKey);
	printf("right key: %02x\n", fRightKey);
	printf("select key: %02x\n", fSelectKey);
	printf("start key: %02x\n", fStartKey);
	printf("b key: %02x\n", fBKey);
	printf("a key: %02x\n", fAKey);
	
	fUpView->SetText("↑");
	fDownView->SetText("↓");
	fLeftView->SetText("←");
	fRightView->SetText("→");
	fSelectView->SetText("A");
	fStartView->SetText("S");
	fBView->SetText("Z");
	fAView->SetText("X");
	
	//BString s = fAView->Text();
	//printf("length: %d\n", s.Length());
	
	//printf("key: %02x\n", (uint8)s[0]);
	
	
	
	//putchar(s[0]);putchar(s[1]);putchar(s[2]);
	//putchar('\n');
}


void
InputView::ValidateKeys()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	//BString strUp = fUpView->Text();
	
	//if (strUp.Length() == 3) {
	//	}else {
	//}
					
/*
	BString s;
	s += fUpView->Text();
	s += fDownView->Text();
	s += fLeftView->Text();
	s += fRightView->Text();
	
	int32 up = s.FindFirst("↑");
	int32 down = s.FindFirst("↓");
	int32 left = s.FindFirst("←");
	int32 right = s.FindFirst("→");
	
	if (up != -1) {
		printf ("found: %02x %02x %02x at offset %d\n", (uint8)s[0], (uint8)s[1], (uint8)s[2], up);
	}
	
	if (down != -1) {
		printf ("found: %02x %02x %02x at offset %d\n", (uint8)s[3], (uint8)s[4], (uint8)s[5], down);
	}
	
	if (left != -1) {
		printf ("found: %02x %02x %02x at offset %d\n", (uint8)s[6], (uint8)s[7], (uint8)s[8], left);
	}
	
	if (right != -1) {
		printf ("found: %02x %02x %02x at offset %d\n", (uint8)s[9], (uint8)s[10], (uint8)s[11], right);
	}
*/
	//printf("key string: %s\n",strUp.String());
}


KeyTextView::KeyTextView (BRect frame)
	: BTextView(frame, "key_text_view", BRect(0, 0, 0, 0), B_FOLLOW_ALL, B_WILL_DRAW)
{
	
}


KeyTextView::~KeyTextView()
{

}


void
KeyTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	
	BRect r(Bounds());
	SetTextRect(BRect(2, 2, r.Width() - 2, r.Height() - 2));
	SetMaxBytes(1);
	SetAlignment(B_ALIGN_CENTER);
	
	BTextView::AttachedToWindow();
}


void
KeyTextView::Draw (BRect updateRect)
{
	BTextView::Draw(updateRect);
}


void
KeyTextView::KeyDown (const char *bytes, int32 numBytes)
{	
	uint8 const key = bytes[0];
	bool allowed = true;
	BString keys;

	switch (key) {
		case B_HOME:
		case B_END:
		case B_INSERT:
		case B_BACKSPACE:
		case B_TAB:
		case B_ENTER:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
		case B_FUNCTION_KEY:
		case B_SPACE:
		case B_DELETE:
		case B_ESCAPE:
		case B_PRINT_KEY:
		case B_SCROLL_KEY:
		case B_NUM_LOCK_KEY:
		case B_CAPS_LOCK_KEY:
		case B_SPACE_BAR_KEY:
		allowed = false;
		break;
	
		case B_UP_ARROW:                                                                                                                             
		(new BAlert(0, "Up", "Okay"))->Go();
		keys = "↑";
		break;
		
		case B_DOWN_ARROW:
		keys = "↓";
		break;
		
		case B_LEFT_ARROW:
		keys = "←";
		break;
		
		case B_RIGHT_ARROW:
		keys = "→";
		break;
		
		default:
		keys = key;
		keys.ToUpper();
	}
	
	if (allowed) {		
		SetText(keys.String());
		(new BAlert(0, keys.String(), "Okay"))->Go();
	}

 	BTextView::KeyDown (bytes, numBytes);
}


std::pmr::unordered_set<uint8>
InputView::FindDuplicates (uint8 *list, size_t size) 
{
	size_t i = 0;
	std::pmr::unordered_map<int32, int32> count;
	
	while (i < size) {
		count[list[i]]++;
		i++;
	}
	
	std::pmr::unordered_set<uint8> duplicates;
	
	for (auto const &pair: count) {
		if (pair.second > 1) {
			duplicates.insert(pair.first);
		}
	}
	
	return duplicates;
}
