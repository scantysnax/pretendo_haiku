
#include <File.h>

#include "Palette.h"
#include "PaletteView.h"


// -----------------------------------------------------------------------------
// PaletteView::PaletteView
//
// Creates the palette editor view, stores the owning PretendoWindow, allocates
// the 64-color working palette, and records the requested swatch size.
//
// Parameters:
//   mainWindow - Owning PretendoWindow used to apply palette changes.
//   frame      - Initial view frame.
//   swatchSize - Size, in pixels, of each displayed palette swatch.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PaletteView::PaletteView (PretendoWindow *mainWindow, BRect frame, size_t swatchSize)
	: BView(frame, "palette_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fMainWindow = mainWindow;
	fSwatchSize = swatchSize;
	fPalette = new rgb_color[64];	
}


// -----------------------------------------------------------------------------
// PaletteView::~PaletteView
//
// Releases the dynamically allocated 64-color palette buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PaletteView::~PaletteView()
{
	if (fPalette != nullptr) {
		delete[] fPalette;
	}
}


// -----------------------------------------------------------------------------
// PaletteView::AttachedToWindow
//
// Performs PaletteView setup that requires an attached window.
//
// The function creates the palette-adjustment sliders, splitters, and command
// buttons, assigns their message targets, initializes default control values,
// and positions the controls within the view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	int32 x;
	int32 y;
	int32 width;
	int32 height;
	
	x = fSwatchSize;
	y = (fSwatchSize*5)+32;
	width = (fSwatchSize*16)+64;
	fHorizSplitter = new HorizontalSplitter(x, y, width);
	AddChild(fHorizSplitter);
	
	float const left = 16.0f;
	float const right = Frame().Width() * 0.67f;
	BRect r;
	                
	r.Set(left, fHorizSplitter->Frame().top+18, right, 0);
	fHueSlider = new BSlider(r, "hue_slider", "Hue", new BMessage(messages::CHANGE_HUE),
		-10000, +10000);
	fHueSlider->SetLimitLabels("-1.0 (-30°)", "1.0 (30°)");
	fHueSlider->SetValue(0);
	fHueSlider->SetTarget(this);
	AddChild(fHueSlider);
	
	r.Set(left, fHueSlider->Frame().bottom+18, right, 0);
	fSaturationSlider = new BSlider(r, "sat_slider", "Saturation", 
		new BMessage(messages::CHANGE_SATURATION), 0, 50000);
	fSaturationSlider->SetLimitLabels("0.0 (grayscale)", "5.0");
	fSaturationSlider->SetTarget(this);
	fSaturationSlider->SetValue(10000);
	AddChild(fSaturationSlider);

	r.Set(left, fSaturationSlider->Frame().bottom+18, right, 0);
	fContrastSlider = new BSlider (r, "contrast_slider", "Contrast", 
		new BMessage(messages::CHANGE_CONTRAST), 5000, 20000);
	fContrastSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fContrastSlider->SetTarget(this);
	fContrastSlider->SetValue(10000);
	AddChild(fContrastSlider);

	r.Set(left, fContrastSlider->Frame().bottom+18, right, 0);
	fBrightnessSlider = new BSlider (r, "brightness_slider", "Brightness", 
		new BMessage(messages::CHANGE_BRIGHTNESS), 5000, 20000);
	fBrightnessSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fBrightnessSlider->SetTarget(this);
	fBrightnessSlider->SetValue(10000);
	AddChild(fBrightnessSlider);

	r.Set(left, fBrightnessSlider->Frame().bottom+18, right, 0);
	fGammaSlider = new BSlider (r, "gamma_slider", "Gamma", 
		new BMessage(messages::CHANGE_GAMMA), 10000, 25000);
	fGammaSlider->SetLimitLabels("1.0", "2.5");
	fGammaSlider->SetTarget(this);
	fGammaSlider->SetValue(14000);
	AddChild(fGammaSlider);		

	x = fHueSlider->Frame().right + 16;
	y = fHueSlider->Frame().top - 4;
	height = fGammaSlider->Frame().bottom + 16;
	fVertSplitter = new VerticalSplitter(x, y, height);
	AddChild(fVertSplitter);
	
	r.Set(fVertSplitter->Frame().right + 0, fVertSplitter->Frame().top, fVertSplitter->Frame().right, 0);
	fDefaultButton = new BButton(r,"default_button", "Default", new BMessage(messages::SET_DEFAULT));
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetTarget(this);
	AddChild(fDefaultButton);
	
	r.Set(fVertSplitter->Frame().right, fDefaultButton->Frame().bottom + 16, 0, 0);
	fRevertButton = new BButton(r, "revert_button", "Revert", new BMessage(messages::REVERT));
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetTarget(this);
	AddChild(fRevertButton);
	
	r.Set(fVertSplitter->Frame().right, fRevertButton->Frame().bottom + 16, 0, 0);
	fLoadButton = new BButton(r, "load_button", "Load", new BMessage(messages::LOAD_PALETTE));
	fLoadButton->ResizeToPreferred();
	fLoadButton->SetTarget(this);
	fLoadButton->SetEnabled(false);
	AddChild(fLoadButton);
	
	r.Set(fVertSplitter->Frame().right, fLoadButton->Frame().bottom + 16, 0, 0);
	fSaveButton = new BButton(r, "save_button", "Save", new BMessage(messages::SAVE_PALETTE));
	fSaveButton->ResizeToPreferred();
	fSaveButton->SetTarget(this);
	fSaveButton->SetEnabled(false);
	AddChild(fSaveButton);

	float const windowWidth = Window()->Frame().Width() - fVertSplitter->Frame().right;
	float const diff = windowWidth - fDefaultButton->Frame().Width() + fVertSplitter->Frame().Width();
	float const x2 = diff / 2;
	
	float const buttonHeight = fDefaultButton->Frame().Height();
	float const buttonSpace = fRevertButton->Frame().top - fDefaultButton->Frame().bottom; 
	float const totalSpace = (buttonHeight*4) + (buttonSpace*3); // four buttons, three spaces
	float const totalHeight = fVertSplitter->Frame().Height();
	float const y2 = (totalHeight - totalSpace) / 2;
	
	fDefaultButton->MoveBy(x2, y2);
	fRevertButton->MoveBy(x2, y2);
	fLoadButton->MoveBy(x2, y2);
	fSaveButton->MoveBy(x2, y2);
}


// -----------------------------------------------------------------------------
// PaletteView::MessageReceived
//
// Handles palette-editor control messages.
//
// Slider changes update the corresponding palette-generation parameter and
// regenerate the active palette.  Default and Revert restore predefined or
// previously saved settings.  Load and Save messages are currently placeholders.
//
// Parameters:
//   message - Message delivered to the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::MessageReceived (BMessage *message)
{
	float const scale = 10000.0f;
	
	switch (message->what) {
		case messages::CHANGE_HUE:
			fCurrentHue = fHueSlider->Value() / scale;
			UpdatePalette();
			break;
		
		case messages::CHANGE_SATURATION:
			fCurrentSaturation = fSaturationSlider->Value() / scale;
			UpdatePalette();
			break;

		case messages::CHANGE_CONTRAST:
			fCurrentContrast = fContrastSlider->Value() / scale;
			UpdatePalette();	
			break;
			
		case messages::CHANGE_BRIGHTNESS:
			fCurrentBrightness = fBrightnessSlider->Value() / scale;
			UpdatePalette();
			break;
		
		case messages::CHANGE_GAMMA:
			fCurrentGamma = fGammaSlider->Value() / scale;
			UpdatePalette();
			break;

		case messages::SET_DEFAULT:
			SetDefaultPalette();
			UpdateSliders();
			break;
			
		case messages::REVERT:
			fCurrentHue = fPrevHue;
			fCurrentSaturation = fPrevSaturation;
			fCurrentBrightness = fPrevBrightness;
			fCurrentContrast = fPrevContrast;
			fCurrentGamma = fPrevGamma;
			
			UpdatePalette();
			UpdateSliders();
			break;
			
		case messages::LOAD_PALETTE:
			break;
			
		case messages::SAVE_PALETTE:
			break;	
				
		default:
			break;
	}
	
	BView::MessageReceived (message);
}


// -----------------------------------------------------------------------------
// PaletteView::Draw
//
// Regenerates the current 64-color NES palette from the active adjustment
// parameters and draws the palette swatch matrix and hexadecimal indexes.
//
// Parameters:
//   frame - Area of the view requested for redraw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::Draw (BRect frame)
{		
	rgb_color_t const *palette = Palette::Generate(fCurrentSaturation,
													fCurrentHue,
													fCurrentContrast,
													fCurrentBrightness,
													fCurrentGamma
													);
	for (int32 i = 0; i < 64; i++) {
		fPalette[i].red = palette[i].r;
		fPalette[i].green =  palette[i].g;
		fPalette[i].blue = palette[i].b;
	}

	fWorkPalette = fPalette;	
	DrawSwatchMatrix(BPoint(16, 16), fSwatchSize, 16, 4);
	DrawIndexes();
	
	BView::Draw(frame);
}


// -----------------------------------------------------------------------------
// PaletteView::DrawSwatch
//
// Draws a single palette color swatch with a recessed BeOS-style border.
//
// Parameters:
//   where - Top-left position of the swatch.
//   fill  - Color used to fill the interior of the swatch.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::DrawSwatch (BPoint where, rgb_color fill)
{
	rgb_color const no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color const lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT);
	rgb_color const darken1 = tint_color(no_tint, B_DARKEN_1_TINT); 
	rgb_color const darken4 = tint_color(no_tint, B_DARKEN_4_TINT);
	
	BRect rect(where.x, where.y, where.x+fSwatchSize, where.y+fSwatchSize);
	
	SetHighColor(darken1); 
	StrokeLine(rect.LeftBottom(), rect.LeftTop()); 
	StrokeLine(rect.LeftTop(), rect.RightTop()); 
	SetHighColor(lightenmax); 
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom()); 
	StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f)); 
	rect.InsetBy (1, 1);
	
	SetHighColor(darken4); 
	StrokeLine(rect.LeftBottom(), rect.LeftTop()); 
	StrokeLine(rect.LeftTop(), rect.RightTop()); 
	SetHighColor(no_tint); 
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom()); 
	StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f)); 
	
	rect.InsetBy(1,1);
	SetHighColor(fill);
	FillRect(rect);	
}


// -----------------------------------------------------------------------------
// PaletteView::DrawSwatchRow
//
// Draws one horizontal row of palette swatches using the current working
// palette.
//
// Parameters:
//   start  - Top-left position of the first swatch.
//   size   - Swatch size in pixels.
//   rowlen - Number of swatches to draw in the row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
PaletteView::DrawSwatchRow (BPoint start, int32 size, int32 rowlen)
{
	if (fPalette == nullptr || fWorkPalette == nullptr || size <= 0 || rowlen <= 0) {
		return;
	}
	
	for (int32 i = 0; i < rowlen; i++) {
		DrawSwatch(start, fWorkPalette[i]);
		start.x += size+4;
	}
}


// -----------------------------------------------------------------------------
// PaletteView::DrawSwatchMatrix
//
// Draws the complete palette swatch matrix as a series of horizontal rows.
//
// The current working-palette pointer is advanced between rows so successive
// palette colors are displayed throughout the matrix.
//
// Parameters:
//   start - Top-left position of the matrix.
//   size  - Swatch size in pixels.
//   ncols - Number of swatches per row.
//   nrows - Number of rows to draw.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::DrawSwatchMatrix (BPoint start, int32 size, int32 ncols, int32 nrows)
{
	if (nrows <= 0 || size <= 0 || fPalette == nullptr || fWorkPalette == nullptr) {
		return;
	}
	
	for (int32 y = 0; y < nrows; y++) {
		DrawSwatchRow(start, size, ncols);
		start.y += size+4;
		fWorkPalette += nrows * sizeof(rgb_color);
	}
}


// -----------------------------------------------------------------------------
// PaletteView::DrawIndexes
//
// Draws hexadecimal row and column indexes around the palette swatch matrix
// using the fixed-width system font.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::DrawIndexes()
{
	char const nybbles[] = "0123456789ABCDEF";
	
	SetHighColor(0, 0, 0);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetFont(be_fixed_font);
	
	// y 
	BPoint p(4, 28);
	for (int32 i = 0; i < 4; i++) {
		DrawChar(nybbles[i], p);
		p.y += (fSwatchSize+4);
	}

	// x
	p.Set(26, 136);
	for (int32 i = 0; i < 16; i++) {
		DrawChar(nybbles[i], p);
		p.x += fSwatchSize+4;
	}
}


// -----------------------------------------------------------------------------
// PaletteView::SetDefaultPalette
//
// Restores the palette-generation parameters to the application's predefined
// defaults, applies the regenerated palette to the emulator, and schedules the
// view for redraw.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::SetDefaultPalette()
{	
	fCurrentSaturation = Palette::default_saturation;
	fCurrentHue = Palette::default_hue;
	fCurrentContrast = Palette::default_contrast;
	fCurrentBrightness = Palette::default_brightness;
	fCurrentGamma = Palette::default_gamma;	
	
	fMainWindow->set_palette(Palette::intensity,
							 Palette::Generate(fCurrentSaturation,
							 					fCurrentHue,
							 			 		fCurrentContrast,
							 					fCurrentBrightness,
							 					fCurrentGamma
							 					));		
	fWorkPalette = fPalette;
	Invalidate();
}


// -----------------------------------------------------------------------------
// PaletteView::UpdatePalette
//
// Regenerates and applies the NES palette using the current adjustment
// parameters, resets the working-palette pointer, and schedules a redraw.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
PaletteView::UpdatePalette()
{	
	fMainWindow->set_palette(Palette::intensity, 
							 Palette::Generate(fCurrentSaturation,
							 					fCurrentHue,
							 					fCurrentContrast,
							 					fCurrentBrightness,
							 					fCurrentGamma
							 					));
	fWorkPalette = fPalette;
	Invalidate();
}


// -----------------------------------------------------------------------------
// PaletteView::UpdateSliders
//
// Synchronizes the palette-adjustment sliders with the current hue,
// saturation, contrast, brightness, and gamma values.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteView::UpdateSliders()
{
	int32 const scale = 10000;
	
	fHueSlider->SetValue(fCurrentHue*scale);
	fSaturationSlider->SetValue(fCurrentSaturation*scale);
	fContrastSlider->SetValue(fCurrentContrast*scale);
	fBrightnessSlider->SetValue(fCurrentBrightness*scale);
	fGammaSlider->SetValue(fCurrentGamma*scale);
	Invalidate();
}

