
#include "InputWindow.h"

#include <iostream>

InputWindow::InputWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fParent = parent;
	
	ResizeTo(InputView::controller_size::WIDTH + 
			 InputView::controller_size::BORDER*2, 
			 430);
	CenterOnScreen();
	SetTitle("Configure Input");
	
	fInputView = new InputView(Bounds());
	AddChild(fInputView);
	
	//SetDefaultKeys();
	
	fSettingsMessage = new BMessage;
	LoadSettings();
}


InputWindow::~InputWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


void
InputWindow::MessageReceived (BMessage *message)		
{
	BWindow::MessageReceived (message);
}


bool
InputWindow::QuitRequested()
{
	return true;
}


void
InputWindow::LoadSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	BString path = Settings::configDirectory().c_str();
	path += "/input_window";
		
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
			
			int32 const x = Frame().left;
			int32 const y = Frame().top;
			
			// stash default settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			
			// keys
			/*
			fSettingsMessage->AddInt8("input_up_key", B_UP_ARROW);
			fSettingsMessage->AddInt8("input_down_key", B_DOWN_ARROW);
			fSettingsMessage->AddInt8("input_left_key", B_LEFT_ARROW);
			fSettingsMessage->AddInt8("input_right_key", B_RIGHT_ARROW);
			fSettingsMessage->AddInt8("input_select_key", 'A');
			fSettingsMessage->AddInt8("input_start_key", 'S');
			fSettingsMessage->AddInt8("input_b_key", 'Z');
			fSettingsMessage->AddInt8("input_a_key", 'X');
			*/
			
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
InputWindow::SaveSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/input_window";
	
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


void
InputWindow::SetDefaultKeys()
{
	fInputView->SetDefaultKeys();
}
