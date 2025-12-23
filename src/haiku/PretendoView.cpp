// PretendoView.cpp

#include <Entry.h>
#include <Path.h>

#include "PretendoView.h"


class PretendoWindow;


PretendoView::PretendoView (BRect frame, PretendoWindow *parent)
	: BView (frame, "pretendo_view", B_FOLLOW_ALL_SIDES, B_NAVIGABLE)
{
	fParent = parent;
	frame.PrintToStream();
	SetViewColor(0, 0, 0);
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
			BMessage msg(PretendoWindow::messages::ROM_LOADED);
			
			entry.SetTo(&ref, true);
			entry.GetPath(&path);
			
			msg.AddString("rom_path", path.Path());
			fParent->PostMessage(&msg);
		}
	}
	
	BView::MessageReceived (message);
}

