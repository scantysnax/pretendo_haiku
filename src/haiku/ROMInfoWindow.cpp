
#include "PretendoWindow.h"
#include "ROMInfoWindow.h"


// -----------------------------------------------------------------------------
// ROMInfoWindow::ROMInfoWindow
//
// Creates the ROM Info window, constructs the scrollable ROM information view,
// and restores the window's saved position.
//
// Parameters:
//   parent - Owning PretendoWindow notified when the ROM Info window closes.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// ROMInfoWindow::~ROMInfoWindow
//
// Destroys the ROM Info window and saves its current window settings.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ROMInfoWindow::~ROMInfoWindow()
{
	SaveSettings();
}


// -----------------------------------------------------------------------------
// ROMInfoWindow::MessageReceived
//
// Handles messages sent to the ROM Info window.  Messages not handled specially
// by this class are passed to BWindow for normal processing.
//
// Parameters:
//   message - Message received by the window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// ROMInfoWindow::Refresh
//
// Refreshes the ROM Info view so it reflects the currently loaded cartridge.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ROMInfoWindow::Refresh()
{
	if (fROMInfoView) {
		fROMInfoView->Refresh();
	}
}


// -----------------------------------------------------------------------------
// ROMInfoWindow::LoadSettings
//
// Loads the saved ROM Info window position from the application settings file.
// If no saved settings exist yet, the window is centered on screen and the
// resulting position is stored as the initial default.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// ROMInfoWindow::SaveSettings
//
// Saves the current ROM Info window position to the application settings file.
// Existing position values are replaced when settings already exist; otherwise
// initial position values are added before the settings message is written.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

