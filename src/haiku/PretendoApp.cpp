
#include <Path.h>
#include <Alert.h>

#include "PretendoApp.h"


PretendoApp::PretendoApp()
	: BApplication("application/x-vnd.scantysnax-Pretendo") 
{
	fWindow = new PretendoWindow;
}


PretendoApp::~PretendoApp()
{

}


void
PretendoApp::ReadyToRun()
{
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

				BMessage msg(MSG_ROM_LOADED);
				msg.AddString("rom_path", path.Path());
				fWindow->PostMessage(&msg);
			}
		} break;
	}
	
	BApplication::RefsReceived(message);
}


void
PretendoApp::ArgvReceived (int32 argc, char **argv)
{	
	if (argv[1] != nullptr) {
		BMessage msg(MSG_ROM_LOADED);
		msg.AddString("rom_path", argv[1]);
		fWindow->PostMessage(&msg);
	}
	
	BApplication::ArgvReceived(argc, argv);
}

