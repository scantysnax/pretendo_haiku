// PretendoView.cpp

#include <Entry.h>
#include <Path.h>

#include "PretendoView.h"


class PretendoWindow;


PretendoView::PretendoView (BRect frame, PretendoWindow *parent)
	: BView (frame, "_pretendo_view_", B_FOLLOW_ALL_SIDES, 0)
{
	fParent = parent;
}


PretendoView::~PretendoView()
{
	
}


void 
PretendoView::MessageReceived (BMessage *message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		
		if (message->FindRef("refs", 0, &ref) == B_OK) {
			BEntry entry;
			BPath path;
			BMessage *msg;
			
			entry.SetTo(&ref, true);
			entry.GetPath(&path);
			
			msg = new BMessage(PretendoWindow::messages::ROM_LOADED);
			msg->AddString("rom_path", path.Path());
			fParent->PostMessage(msg);
			
			delete msg;
		}
	}
	
	BView::MessageReceived (message);
}
