
#include <File.h>

#include "PaletteView.h"
#include "PaletteWindow.h"


PaletteWindow::PaletteWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "Adjust Palette", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fParent = parent;
	fSettingsMessage = new BMessage;
	
	ResizeTo(480, 648);
	
	//BView *backView = new BView(Bounds(), "back_view", B_FOLLOW_ALL, 0);
	//backView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	//AddChild (backView);
	 
	fPaletteView = new PaletteView(fParent, Bounds(), 24);
	AddChild(fPaletteView);
	
	LoadSettings();
}


PaletteWindow::~PaletteWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


bool
PaletteWindow::QuitRequested (void)
{
	return true;
}


void
PaletteWindow::LoadSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/palette_window";
		
	// open settings file
	BFile file;
	status_t st;
	off_t size;
	
	st = file.SetTo(path, B_READ_WRITE | B_CREATE_FILE);
	
	if (st == B_OK) {
		std::cout << "successfuly opened file." << std::endl;
		file.GetSize(&size);
		
		// if file is empty, load some defaults
		if (size == 0) {
			CenterOnScreen();
			int32 x = Frame().left;
			int32 y = Frame().top;
			
			/*
			float hue;
			float saturation;
			float contrast;
			float brightness;
			float gamma;
			*/
			
			// stash settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			fSettingsMessage->Flatten(&file);
	
			// apply settings	
			MoveTo(x, y);
			 //fView->SetViewMode(static_cast<PatternTableView::view_mode>(mode));	
			 // etc..
		} else {
			// load from file
			st = fSettingsMessage->Unflatten(&file);
			
			if (st == B_OK) {
				// read settings
				int32 x;
				int32 y;
				
				//float hue;
				//float saturation;
				//float contrast;
				//float brightness;
				//float gamma;
				
				fSettingsMessage->FindInt32("window_x", &x);
				fSettingsMessage->FindInt32("window_y", &y);

				// apply settings
				MoveTo(x, y);
				// fView->SetViewMode(static_cast<PatternTableView::view_mode>(mode));
				// etc...
			} else {
				// eli: handle error if unflatten fails?
			}
		}
	}
}


void
PaletteWindow::SaveSettings()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/palette_window";
	
	BFile file;
	status_t st;
	off_t size;

	// load settings file
	st = file.SetTo(path, B_READ_WRITE | B_CREATE_FILE);
	
	if (st == B_OK) {
		file.GetSize(&size);
		if (size == 0) {
			// file is empty, stash settings
			fSettingsMessage->AddInt32("window_x", Frame().left);
			fSettingsMessage->AddInt32("window_y", Frame().top);
		} else {
			// replace old settings
			fSettingsMessage->ReplaceInt32("window_x", Frame().left);
			fSettingsMessage->ReplaceInt32("window_y", Frame().top);
		}
		
		// write to file
		fSettingsMessage->Flatten(&file);
	}
}

