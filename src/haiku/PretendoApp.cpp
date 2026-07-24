
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


// -----------------------------------------------------------------------------
// PretendoApp::RefsReceived
//
// Handles files opened through Haiku, including Recent Documents and file refs
// sent to the application.  The resolved filesystem path is forwarded to the
// main window using the same ROM_LOADED message used by the manual file loader,
// so all ROM-load cleanup/reset/debug-refresh behavior stays centralized.
//
// Parameters:
//   message - Message containing one or more "refs" entries.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoApp::RefsReceived (BMessage *message)
{	
	entry_ref ref;
		
	if (message->FindRef("refs", 0, &ref) != B_OK) {
		return;
	}

	BEntry entry(&ref, true);
	BPath path;
	
	if (entry.GetPath(&path) != B_OK) {
		return;
	}

	BMessage msg(PretendoWindow::messages::ROM_LOADED);
	msg.AddString("rom_path", path.Path());

	if (fWindow) {
		fWindow->PostMessage(&msg);
	}
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

