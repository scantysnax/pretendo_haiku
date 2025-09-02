
#include <Path.h>
#include <Alert.h>

#include "PretendoApp.h"


PretendoApp::PretendoApp()
	: BApplication("application/x-vnd.scantysnax-Pretendo") 
{
	 	
}


PretendoApp::~PretendoApp()
{
	
}


void
PretendoApp::ReadyToRun()
{
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
	//for (int32 i = 0; i < argc; i++) {
	//	printf("%s\n", argv[i]);
	//
	//}	
	
	BApplication::ArgvReceived(argc, argv);
	
}

