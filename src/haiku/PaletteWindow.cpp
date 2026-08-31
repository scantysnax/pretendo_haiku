
#include "PaletteView.h"
#include "PaletteWindow.h"
#include "Palette.h"

#include <File.h>


// -----------------------------------------------------------------------------
// PaletteWindow::PaletteWindow
//
// Creates the palette adjustment window.  notifyParentOnClose controls whether
// this window reports its closure back to PretendoWindow.  The startup/internal
// palette window uses false because it is created only to initialize palette
// state and should not affect tool-window input tracking.
//
// Parameters:
//   parent              - Owning PretendoWindow.
//   notifyParentOnClose - true for interactive windows, false for internal use.
//
// Returns:
//   Nothing.
// ----------------------------------------------------------------------------- 

PaletteWindow::PaletteWindow (PretendoWindow *parent, bool notifyParentOnClose)
	: BWindow(BRect(0, 0, 0, 0), "Adjust Palette", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fParent = parent;
	fNotifyParentOnClose = notifyParentOnClose;
	
	ResizeTo(480, 580);
	
	fPaletteView = new PaletteView(parent, Bounds(), 24);
	AddChild(fPaletteView);
	
	fSettingsMessage = new BMessage;
	LoadSettings();
}



PaletteWindow::~PaletteWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


// -----------------------------------------------------------------------------
// PaletteWindow::QuitRequested
//
// Notifies the parent window when an interactive palette window closes.  Startup
// palette-helper windows do not notify the parent because they are not counted
// as tool-input windows.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PaletteWindow::QuitRequested()
{
	if (fNotifyParentOnClose && fParent) {
		fParent->PaletteWindowClosed();
	}

	return true;
}


// -----------------------------------------------------------------------------
// PaletteWindow::LoadSettings
//
// Loads the Palette window position and palette-adjustment settings from the
// application's configuration directory.
//
// If the settings file is empty, default palette values are created, stored,
// applied to the PaletteView, and saved as the initial previous-state values.
// If an existing settings file is present, its values are restored and applied
// to both the window and palette controls.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteWindow::LoadSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/palette_window";
		
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
			
			float hue = Palette::default_hue;			
			float saturation = Palette::default_saturation;
			float contrast = Palette::default_contrast;
			float brightness = Palette::default_brightness;
			float gamma = Palette::default_gamma;
			
			// stash default settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			fSettingsMessage->AddFloat("palette_hue", hue);
			fSettingsMessage->AddFloat("palette_saturation", saturation);
			fSettingsMessage->AddFloat("palette_contrast", contrast);
			fSettingsMessage->AddFloat("palette_brightness", brightness);
			fSettingsMessage->AddFloat("palette_gamma", gamma);
			
			fSettingsMessage->Flatten(&file);
	
			// apply settings (update user interface)
			MoveTo(x, y);
			fPaletteView->SetHue(hue);
			fPaletteView->SetSaturation(saturation);
			fPaletteView->SetContrast(contrast);
			fPaletteView->SetBrightness(brightness);
			fPaletteView->SetGamma(gamma);
			fPaletteView->UpdatePalette();
			fPaletteView->UpdateSliders();
			
			fPaletteView->SetPrevHue(hue);
			fPaletteView->SetPrevSaturation(saturation);
			fPaletteView->SetPrevContrast(contrast);
			fPaletteView->SetPrevBrightness(brightness);
			fPaletteView->SetPrevGamma(gamma);
		} else {
			// load from file
			status = fSettingsMessage->Unflatten(&file);
			
			if (status == B_OK) {
				// read settings
				int32 x;
				int32 y;
				float hue;
				float saturation;
				float contrast;
				float brightness;
				float gamma;
				
				fSettingsMessage->FindInt32("window_x", &x);
				fSettingsMessage->FindInt32("window_y", &y);
				fSettingsMessage->FindFloat("palette_hue", &hue);
				fSettingsMessage->FindFloat("palette_saturation", &saturation);
				fSettingsMessage->FindFloat("palette_contrast", &contrast);
				fSettingsMessage->FindFloat("palette_brightness", &brightness);
				fSettingsMessage->FindFloat("palette_gamma", &gamma);
				
				// apply settings
				MoveTo(x, y);
				fPaletteView->SetHue(hue);
				fPaletteView->SetSaturation(saturation);
				fPaletteView->SetContrast(contrast);
				fPaletteView->SetBrightness(brightness);
				fPaletteView->SetGamma(gamma);
				fPaletteView->UpdatePalette();
				fPaletteView->UpdateSliders();
				
				fPaletteView->SetPrevHue(hue);
				fPaletteView->SetPrevSaturation(saturation);
				fPaletteView->SetPrevBrightness(brightness);
				fPaletteView->SetPrevContrast(contrast);
				fPaletteView->SetPrevGamma(gamma);
			} else {
				// eli: handle error if unflatten fails?
			}	
		}
	}
}


// -----------------------------------------------------------------------------
// PaletteWindow::SaveSettings
//
// Saves the current Palette window position and palette-adjustment values to
// the application's configuration directory.
//
// New settings are added when the settings file is empty; otherwise the existing
// values are replaced with the current window position and PaletteView state.
// The resulting settings message is then flattened to disk.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteWindow::SaveSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/palette_window";
	
	BFile file;
	status_t status;
	off_t size;

	// load settings file
	status = file.SetTo(path, B_READ_WRITE|B_CREATE_FILE);
	
	if (status == B_OK) {
		file.GetSize(&size);
		
		if (size == 0) {
			// file is empty, stash settings
			fSettingsMessage->AddInt32("window_x", Frame().left);
			fSettingsMessage->AddInt32("window_y", Frame().top);
			fSettingsMessage->AddFloat("palette_hue", fPaletteView->Hue());
			fSettingsMessage->AddFloat("palette_saturation", fPaletteView->Saturation());
			fSettingsMessage->AddFloat("palette_contrast", fPaletteView->Contrast());
			fSettingsMessage->AddFloat("palette_brightness", fPaletteView->Brightness());
			fSettingsMessage->AddFloat("palette_gamma", fPaletteView->Gamma());
		} else {
			// replace old settings
			fSettingsMessage->ReplaceInt32("window_x", Frame().left);
			fSettingsMessage->ReplaceInt32("window_y", Frame().top);
			fSettingsMessage->ReplaceFloat("palette_hue", fPaletteView->Hue());
			fSettingsMessage->ReplaceFloat("palette_saturation", fPaletteView->Saturation());
			fSettingsMessage->ReplaceFloat("palette_contrast", fPaletteView->Contrast());
			fSettingsMessage->ReplaceFloat("palette_brightness", fPaletteView->Brightness());
			fSettingsMessage->ReplaceFloat("palette_gamma", fPaletteView->Gamma());
		}
	} else {		
		// eli: handle error if we couldn't load the file?
	}
	
	// write to file
	fSettingsMessage->Flatten(&file);
}

