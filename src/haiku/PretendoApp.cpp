
#include <Path.h>
#include <Alert.h>

#include "PretendoApp.h"


PretendoApp::PretendoApp()
	: BApplication("application/x-vnd.scantysnax-Pretendo") 
{
	puts(__PRETTY_FUNCTION__); 	
}


PretendoApp::~PretendoApp()
{
	puts(__PRETTY_FUNCTION__);
}


void
PretendoApp::ReadyToRun()
{
	puts(__PRETTY_FUNCTION__);
	
	fWindow = new PretendoWindow;
	fWindow->Show();
	
	BApplication::ReadyToRun();
}


void
PretendoApp::AboutRequested()
{
	(new AboutWindow)->Show();
	
	BApplication::AboutRequested();
}


void
PretendoApp::RefsReceived (BMessage *message)
{		
	puts(__PRETTY_FUNCTION__);
	
	switch (message->what) {
		case B_REFS_RECEIVED:
		{      	
			entry_ref ref;
			
			if (message->FindRef("refs", 0, &ref) == B_OK) {
				BEntry entry;
				BPath path;

				entry.SetTo(&ref, true);
				entry.GetPath(&path);

				BMessage *msg = new BMessage(MSG_ROM_LOADED);
				msg->AddString("rom_path", path.Path());
				fWindow->PostMessage(msg);
				delete msg;
			}
		} break;
	}
	
	BApplication::RefsReceived(message);
}


void
PretendoApp::ArgvReceived (int32 argc, char **argv)
{
	puts(__PRETTY_FUNCTION__);
	
	
	BApplication::ArgvReceived(argc, argv);
	
}

