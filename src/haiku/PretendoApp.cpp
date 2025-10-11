
#include "PretendoApp.h"

class PretendoWindow;

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
	entry_ref ref;
		
	if (message->FindRef("refs", 0, &ref) == B_OK) {
		BEntry entry;
		BPath path;
		
		entry.SetTo(&ref, true);
		entry.GetPath(&path);
		
		BMessage msg(PretendoWindow::messages::ROM_LOADED);
		msg.AddString("rom_path", path.Path());
		fWindow->PostMessage(&msg);
	}
	
	BApplication::RefsReceived (message);
}


void
PretendoApp::ArgvReceived (int32 argc, char **argv)
{	
	if (argv[1] != nullptr) {
		BMessage msg(PretendoWindow::messages::ROM_LOADED);
		msg.AddString("rom_path", argv[1]);
		fWindow->PostMessage(&msg);
	}
	
	BApplication::ArgvReceived (argc, argv);
}

