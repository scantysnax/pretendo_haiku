
#include "PretendoApp.h"


PretendoApp::PretendoApp()
	: BApplication("application/x-vnd.scantysnax-Pretendo") 
{
	fWindow = new PretendoWindow;
	
	(new BAlert(0, __PRETTY_FUNCTION__, "Okay"))->Go();
}


PretendoApp::~PretendoApp()
{

}


void
PretendoApp::ReadyToRun()
{	
	fWindow->Show();
	
	(new BAlert(0, __PRETTY_FUNCTION__, "Okay"))->Go();
	
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
	(new BAlert(0, __PRETTY_FUNCTION__, "Okay"))->Go();
	
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
	
	BApplication::RefsReceived (message);
}


void
PretendoApp::ArgvReceived (int32 argc, char **argv)
{	
	if (argv[1] != nullptr) {
		(new BAlert(0, __PRETTY_FUNCTION__, "Okay"))->Go();
		BMessage msg(MSG_ROM_LOADED);
		
		msg.AddString("rom_path", argv[1]);
		fWindow->PostMessage(&msg);
	}
	
	BApplication::ArgvReceived (argc, argv);
}

