
#include "PatternTableWindow.h"

#include <File.h>
#include <String.h>


PatternTableWindow::PatternTableWindow (PretendoWindow *parent, int32 which)
	: BWindow(BRect(0, 0, 0, 0), nullptr, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE)
{
	fParent = parent;
	fWhich = which;
	fSettingsMessage = new BMessage;
	
	// set size and default title
	ResizeTo(PatternTableView::screen_size::WIDTH*2, PatternTableView::screen_size::HEIGHT*2);
	SetTitle((fWhich == 0) 	?	"Pattern Table 1 (8x8)" 
							: 	"Pattern Table 2 (8x8)"
	);
	
	// setup some things we need
	fView = new PatternTableView(Bounds(), fWhich);
	AddChild(fView);	
	SetPulseRate(1000000ULL); // one second
	
	// load settings
	LoadSettings();
	
}


PatternTableWindow::~PatternTableWindow()
{
	// save settings and clean up
	SaveSettings();
	delete fSettingsMessage;
} 


bool
PatternTableWindow::QuitRequested()
{
	// nothing to do here
	return true;
}


void
PatternTableWindow::Zoom (BPoint origin, float width, float height)
{
	(void)origin;
	(void)width;
	(void)height;
	
	BString title = Title();
	
	// flip tile arrangement on zoom between 8x8 and 8x16
	if (fView->ViewMode() == PatternTableView::view_mode::MODE_8x8) {
		fView->SetViewMode(PatternTableView::view_mode::MODE_8x16);
		title.ReplaceFirst("(8x8)", "(8x16)");
	} else {
		fView->SetViewMode(PatternTableView::view_mode::MODE_8x8);
		title.ReplaceFirst("(8x16)", "(8x8)");
	}
	
	SetTitle(title.String());
	
	// don't call the default
}


void
PatternTableWindow::LoadSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/pattern_table_window_";
	path += (fWhich == 0) ? "0" : "1";
	
	// open settings file
	BFile file;
	status_t st;
	off_t size;
	
	st = file.SetTo(path, B_READ_WRITE | B_CREATE_FILE);
	
	if (st == B_OK) {
		file.GetSize(&size);
		
		// if file is empty, load some defaults
		if (size == 0) {
			int32 x;
			int32 y;
			int32 mode = PatternTableView::view_mode::MODE_8x8;
			
			if (fWhich == 0) {
				x = 200;
				y = 200;
			} else {
				x = 300;
				y = 300;
			}
			
			// stash settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			fSettingsMessage->AddInt32("view_mode", mode);
			fSettingsMessage->Flatten(&file);
			
			// apply settings	
			MoveTo(x, y);
			fView->SetViewMode(static_cast<PatternTableView::view_mode>(mode));	
		} else {
			// load from file
			st = fSettingsMessage->Unflatten(&file);
			
			if (st == B_OK) {
				// read settings
				int32 x;
				int32 y;
				int32 mode;
				fSettingsMessage->FindInt32("window_x", &x);
				fSettingsMessage->FindInt32("window_y", &y);
				fSettingsMessage->FindInt32("view_mode", &mode);
				
				// apply settings
				MoveTo(x, y);
				fView->SetViewMode(static_cast<PatternTableView::view_mode>(mode));
				
				// set window title accordingly
				BString title = Title();
				
				if (mode == PatternTableView::view_mode::MODE_8x16) {
					title.ReplaceFirst("(8x8)", "(8x16)");
				} else {
					title.ReplaceFirst("(8x16)", "(8x8)");
				}
				
				SetTitle(title.String());	
			} else {
				// eli: handle error if unflatten fails?
			}
		}
	}
}


void
PatternTableWindow::SaveSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/pattern_table_window_";
	path += (fWhich == 0) ? "0" : "1";
	
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
			fSettingsMessage->AddInt32("view_mode", fView->ViewMode());
		} else {
			// replace old settings
			fSettingsMessage->ReplaceInt32("window_x", Frame().left);
			fSettingsMessage->ReplaceInt32("window_y", Frame().top);
			fSettingsMessage->ReplaceInt32("view_mode", fView->ViewMode());
		}
		
		// write to file
		fSettingsMessage->Flatten(&file);
	}
}

	
