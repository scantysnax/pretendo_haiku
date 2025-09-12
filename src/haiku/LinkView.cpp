
#include "LinkView.h"


uint8 link_cursor[] = { 
	16, 1, 1, 2,
 	// This is the cursor data.
 	0x00, 0x00, 0x38, 0x00, 0x24, 0x00, 0x24, 0x00,
 	0x13, 0xe0, 0x12, 0x5c, 0x09, 0x2a, 0x08, 0x01,
 	0x3c, 0x21, 0x4c, 0x71, 0x42, 0x71, 0x30, 0xf9,
 	0x0c, 0xf9, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
 	// This is the cursor mask.
 	0x00, 0x00, 0x38, 0x00, 0x3c, 0x00, 0x3c, 0x00,
 	0x1f, 0xe0, 0x1f, 0xfc, 0x0f, 0xfe, 0x0f, 0xff,
 	0x3f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x3f, 0xff,
 	0x0f, 0xff, 0x03, 0xfe, 0x01, 0xf8, 0x00, 0x00,
};


LinkView::LinkView (BRect frame, char const *text, char const *link)
	: BStringView(frame, "_linkview", text, B_FOLLOW_ALL, B_WILL_DRAW)
{
	fText = text;
	fLink = link;
	fLinkCursor = new BCursor(link_cursor);
}


LinkView::~LinkView()
{
}


void
LinkView::AttachedToWindow()
{	
	BFont f;
	GetFont(&f);
	f.SetSize(12.0f);
	f.SetFace(B_UNDERSCORE_FACE);
	SetFont(&f);
	SetHighColor(0, 0, 255);
	
	SetViewColor(255,255,255);
	
	BStringView::AttachedToWindow();
}

void
LinkView::MouseUp (BPoint point)
{
	SetHighColor(0x55, 0x1a, 0x8b);
	BRect frame(Frame());
	frame.OffsetTo(B_ORIGIN);
	Invalidate();
	
	be_roster->Launch("text/html", 1, const_cast<char **>(&fLink));
	
	BStringView::MouseUp (point);
}


void
LinkView::MouseDown (BPoint point)
{
	SetHighColor(255, 0, 0);
	
	BRect frame(Frame());
	frame.OffsetTo(B_ORIGIN);
	Invalidate();
	
	BStringView::MouseDown (point);
}


void
LinkView::MouseMoved (BPoint point, uint32 transit, const BMessage *message)
{
	if (transit == B_ENTERED_VIEW) {
		be_app->SetCursor(fLinkCursor);
	} else if (transit == B_EXITED_VIEW) {
		be_app->SetCursor(B_HAND_CURSOR);
	}
	
	BStringView::MouseMoved(point, transit, message);
}

void 
LinkView::Draw (BRect updateRect)
{
	BStringView::Draw(updateRect);
}

