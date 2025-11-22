
#include "PaletteView.h"
#include "PaletteWindow.h"
#include "Palette.h"

#include <File.h>


PaletteWindow::PaletteWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "Adjust Palette", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fSettingsMessage = new BMessage;
	
	ResizeTo(480, 648);
	
	fPaletteView = new PaletteView(parent, Bounds(), 24);
	AddChild(fPaletteView);
	
	LoadSettings();
}


PaletteWindow::~PaletteWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


bool
PaletteWindow::QuitRequested()
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
			
			// stash settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			fSettingsMessage->AddFloat("palette_hue", hue);
			fSettingsMessage->AddFloat("palette_saturation", saturation);
			fSettingsMessage->AddFloat("palette_contrast", contrast);
			fSettingsMessage->AddFloat("palette_brightness", brightness);
			fSettingsMessage->AddFloat("palette_gamma", gamma);
			fSettingsMessage->Flatten(&file);
	
			// apply settings	
			MoveTo(x, y);
			fPaletteView->SetHue(hue);
			fPaletteView->SetSaturation(saturation);
			fPaletteView->SetContrast(contrast);
			fPaletteView->SetBrightness(brightness);
			fPaletteView->SetGamma(gamma);
			fPaletteView->UpdatePalette();
			fPaletteView->UpdateSliders();
			fPaletteView->Invalidate();
		} else {
			// load from file
			st = fSettingsMessage->Unflatten(&file);
			
			if (st == B_OK) {
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
				fPaletteView->Invalidate();
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
		
		// write to file
		fSettingsMessage->Flatten(&file);
	}
}

