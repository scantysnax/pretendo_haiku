
#include <File.h>
#include <String.h>

#include "PatternTableWindow.h"
#include "PatternTableView.h"
#include "CHRExplorerView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// PatternTableWindow::PatternTableWindow
//
// Creates a Pattern Table debugger window together with its PatternTableView
// and CHRExplorerView.
//
// The selected pattern-table index determines the initial window title and
// pattern-table source. The window is sized to contain both the pattern-table
// display and CHR explorer, and persistent window settings are restored after
// construction.
//
// Parameters:
//   parent - Owning PretendoWindow.
//   which  - Pattern-table index:
//              0 = Pattern Table 0 ($0000)
//              1 = Pattern Table 1 ($1000)
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PatternTableWindow::PatternTableWindow(PretendoWindow *parent, int32 which)
    : BWindow(BRect(100, 100, 100, 100), nullptr, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE)
{
    fParent = parent;
    fWhich = which;
    fSettingsMessage = new BMessage;

    const float kPatternW = 280.0f;
    const float kExplorerW = CHRExplorerView::PreferredWidth();
    const float kWindowH = CHRExplorerView::PreferredHeightForPatternTable();

    ResizeTo(kPatternW + kExplorerW, kWindowH);
    SetTitle((fWhich == 0) ? "Pattern Table 1 (8x8)" : "Pattern Table 2 (8x8)");

    BRect patternFrame(0.0f, 0.0f, kPatternW - 1.0f, kWindowH - 1.0f);
    BRect explorerFrame(kPatternW, 0.0f, kPatternW + kExplorerW - 1.0f, kWindowH - 1.0f);

    fView = new PatternTableView(patternFrame, fParent, fWhich, nullptr);
    AddChild(fView);

    fExplorer = new CHRExplorerView(explorerFrame);
    fExplorer->SetHostPalette(fParent->Palette());
    AddChild(fExplorer);

    fView->SetExplorer(fExplorer);

    SetPulseRate(16667);

    LoadSettings();
}


// -----------------------------------------------------------------------------
// PatternTableWindow::~PatternTableWindow
//
// Saves persistent Pattern Table window settings and releases resources owned
// directly by the window.
//
// Child views are owned and destroyed by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PatternTableWindow::~PatternTableWindow()
{
	// save settings and clean up
	SaveSettings();
	delete fSettingsMessage;
} 


// -----------------------------------------------------------------------------
// PatternTableWindow::QuitRequested
//
// Handles a request to close one of the Pattern Table debugger windows.
//
// The owning PretendoWindow is notified so it can release tool-input ownership,
// clear the corresponding stored window pointer, and reconnect the remaining
// debugger views.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PatternTableWindow::QuitRequested()
{
	if (fParent) {
		switch (fWhich) {
			case 0:
				fParent->PatternTable1WindowClosed();
				break;

			case 1:
				fParent->PatternTable2WindowClosed();
				break;

			default:
				break;
		}
	}

	return true;
}


// -----------------------------------------------------------------------------
// PatternTableWindow::Zoom
//
// Toggles the Pattern Table debugger between 8x8 and 8x16 display modes.
//
// The PatternTableView mode and window title are updated to reflect the new
// display mode, after which the view is redrawn immediately.
//
// Parameters:
//   origin - Zoom origin supplied by BWindow.
//   width  - Requested zoom width.
//   height - Requested zoom height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PatternTableWindow::Zoom (BPoint origin, float width, float height)
{
    (void)origin;
    (void)width;
    (void)height;

    if (!fView) {
    	return;
    }
    
    PatternTableView::view_mode mode;
    mode = fView->Show8x16() ? PatternTableView::view_mode::MODE_8x8
    						 : PatternTableView::view_mode::MODE_8x16;

    
    // change view mode
	fView->SetViewMode(mode);
    
    // update window title
    BString title;
    if (mode == PatternTableView::view_mode::MODE_8x16)
        title = (fWhich == 0)
            ? "Pattern Table 1 (8x16)"
            : "Pattern Table 2 (8x16)";
    else
        title = (fWhich == 0)
            ? "Pattern Table 1 (8x8) "
            : "Pattern Table 2 (8x8) ";

    SetTitle(title.String());

    // force redraw + explorer refresh (essential)
    fView->Invalidate();
    fView->Flush();
}


// -----------------------------------------------------------------------------
// PatternTableWindow::LoadSettings
//
// Restores persistent settings for this Pattern Table debugger window.
//
// Settings are maintained independently for Pattern Table 0 and Pattern Table 1.
// Saved state includes the window position and PatternTableView display mode.
// The window title is updated to match the restored mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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
			CenterOnScreen();
			int32 x = Frame().left;
			int32 y = Frame().top;
			int32 mode = PatternTableView::view_mode::MODE_8x8;
		
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


// -----------------------------------------------------------------------------
// PatternTableWindow::SaveSettings
//
// Saves persistent settings for this Pattern Table debugger window.
//
// Settings are maintained independently for Pattern Table 0 and Pattern Table 1.
// The current window position and PatternTableView display mode are stored for
// restoration the next time the window is opened.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

	
