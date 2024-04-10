// PretendoView.cc

#include <Alert.h>
#include <Entry.h>
#include <Path.h>

#include "PretendoView.h"

class PretendoWindow;

PretendoView::PretendoView (BRect frame, PretendoWindow *parent)
	: BView (frame, "_pretendo_view_", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fParent(parent)
{
}


PretendoView::~PretendoView()
{
	
}


void
PretendoView::DrawBitmap (BBitmap *bitmap)
{
	BView::DrawBitmap (bitmap);
}

void 
PretendoView::MessageReceived (BMessage *message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		
		if (message->FindRef ("refs", 0, &ref) == B_OK) {
			BEntry entry;
			BPath path;
			BMessage *msg;
			
			entry.SetTo(&ref, true);
			entry.GetPath(&path);
			
			msg = new BMessage(MSG_ROM_LOADED);
			msg->AddString("path", path.Path());
			fParent->PostMessage(msg);
			
			delete msg;
		}
	}
	
	BView::MessageReceived (message);
}

void 
PretendoView::MouseMoved (BPoint point, uint32 transit, BMessage *message) 
{
	BView::MouseMoved (point, transit, message);
}
	

