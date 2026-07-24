
#include <File.h>

#include <iostream>

#include "PretendoWindow.h"
#include "ROMInfoWindow.h"
#include "Settings.h"

ROMInfoWindow::ROMInfoWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "ROM Info", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
	B_NOT_RESIZABLE|B_NOT_ZOOMABLE)

{
	fParent = parent;
	
	ResizeTo(720, 320);
	
	BRect r = Bounds();
	r.right -= 16;
	fROMInfoView = new ROMInfoView(r);
	BScrollView *sv = new BScrollView("rom_info_sv", fROMInfoView, B_FOLLOW_ALL, 0, false, true);
	AddChild(sv);
	
	fSettingsMessage = new BMessage;
	LoadSettings();
}


ROMInfoWindow::~ROMInfoWindow()
{
	SaveSettings();
}




void
ROMInfoWindow::MessageReceived (BMessage *message)
{
	BWindow::MessageReceived (message);
}


// -----------------------------------------------------------------------------
// ROMInfoWindow::QuitRequested
//
// Notifies the parent PretendoWindow that the ROM info window is closing so the
// parent can release tool-input ownership and clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
ROMInfoWindow::QuitRequested()
{
	if (fParent) {
		fParent->ROMInfoWindowClosed();
	}

	return true;
}


void
ROMInfoWindow::LoadSettings()
{
	BString path = Settings::configDirectory().c_str();
	path += "/rom_info_window";
		
	// open settings file.  create a new one if it doesn't exist
	BFile file;
	status_t status;
	off_t size;
	
	status = file.SetTo(path, B_READ_WRITE|B_CREATE_FILE);
	
	if (status == B_OK) {
		file.GetSize(&size);
		
		// if file is empty, load some defaults
		if (size == 0) {
			CenterOnScreen();
			
			int32 x = Frame().left;
			int32 y = Frame().top;
			
			// stash default settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);

			fSettingsMessage->Flatten(&file);
	
			// apply settings (update user interface)
			MoveTo(x, y);		
		} else {
			// load from file
			status = fSettingsMessage->Unflatten(&file);
			
			if (status == B_OK) {
				// read settings
				int32 x;
				int32 y;
				
				fSettingsMessage->FindInt32("window_x", &x);
				fSettingsMessage->FindInt32("window_y", &y);
				
				// apply settings
				MoveTo(x, y);
			} else {
				// eli: handle error if unflatten fails?
			}
		}
	}
}


void
ROMInfoWindow::SaveSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/rom_info_window";
	
	BFile file;
	status_t status;
	off_t size;

	// load settings file
	status = file.SetTo(path, B_READ_WRITE|B_CREATE_FILE);
	
	if (status == B_OK) {
		file.GetSize(&size);
		
		if (size == 0) {
			// file is empty, stash default settings
			fSettingsMessage->AddInt32("window_x", Frame().left);
			fSettingsMessage->AddInt32("window_y", Frame().top);
		} else {
			// replace old settings
			fSettingsMessage->ReplaceInt32("window_x", Frame().left);
			fSettingsMessage->ReplaceInt32("window_y", Frame().top);
		}		
		
		// write to file
		fSettingsMessage->Flatten(&file);
	} else {
		// eli: handle error if we can't open the file?
	}	
}

