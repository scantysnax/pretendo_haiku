
#include "PaletteView.h"
#include "Palette.h"

#include <cstdio>


PaletteView::PaletteView (PretendoWindow *parent, BRect frame, int32 swatchSize)
	: BView (frame, "palette", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fSwatchSize(swatchSize),
	fPalette(new rgb_color[64]),
	fParent(parent)
{
	//SetDefaultPalette();
}


PaletteView::~PaletteView()
{
	delete[] fPalette;
	//delete fMsgChangePalette;

}


void
PaletteView::AttachedToWindow()
{
	SetViewColor (ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	r.Set(fSwatchSize,
		(fSwatchSize*5)+32,
		(fSwatchSize*16)+64,
		(fSwatchSize*5)+36);
	
	fHorizSeparator = new BBox(r);
	AddChild(fHorizSeparator);
	
	float const left = 16.0f;
	float const right = Frame().Width() * 0.65f;
	                
	r.Set(left, fHorizSeparator->Frame().top+32, right, 0);
	fHueSlider = new BSlider (r, "_hue_slider", "Hue", new BMessage('HUE_'),
		-10000, +10000);
	fHueSlider->SetLimitLabels("-1.0 (-30°)", "1.0 (30°)");
	fHueSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);	
	fHueSlider->SetHashMarkCount(25);
	fHueSlider->SetValue(0);
	fHueSlider->SetTarget(this);
	AddChild(fHueSlider);
		
	r.Set(left, fHueSlider->Frame().bottom+32, right, 0);
	fSaturationSlider = new BSlider (r, "_sat_slider", "Saturation", new BMessage('SAT_'),
		0, 50000);
	fSaturationSlider->SetLimitLabels("0.0 (grayscale)", "5.0");
	fSaturationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSaturationSlider->SetHashMarkCount(25);
	fSaturationSlider->SetTarget(this);
	fSaturationSlider->SetValue(10000);
	AddChild(fSaturationSlider);


	r.Set(left, fSaturationSlider->Frame().bottom+32, right, 0);
	fContrastSlider = new BSlider (r, "_contrast_slider", "Contrast", new BMessage('CONT'),
		5000, 20000);
	fContrastSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fContrastSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fContrastSlider->SetHashMarkCount(25);
	fContrastSlider->SetTarget(this);
	fContrastSlider->SetValue(10000);
	AddChild(fContrastSlider);

	
	r.Set(left, fContrastSlider->Frame().bottom+32, right, 0);
	fBrightnessSlider = new BSlider (r, "_brightness_slider", "Brightness", new BMessage('BRIT'),
		5000, 20000);
	fBrightnessSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fBrightnessSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBrightnessSlider->SetHashMarkCount(25);
	fBrightnessSlider->SetTarget(this);
	fBrightnessSlider->SetValue(10000);
	AddChild(fBrightnessSlider);

	r.Set(left, fBrightnessSlider->Frame().bottom+32, right, 0);
	fGammaSlider = new BSlider (r, "_gamma_slider", "Gamma", new BMessage('GAMA'),
		10000, 25000);
	fGammaSlider->SetLimitLabels("1.0", "2.5");
	fGammaSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGammaSlider->SetHashMarkCount(25);
	fGammaSlider->SetTarget(this);
	fGammaSlider->SetValue(20000);
	AddChild(fGammaSlider);
	
	r.Set (fHueSlider->Frame().right + 16,
			fHueSlider->Frame().top - 4,
			fHueSlider->Frame().right + 20,
			fGammaSlider->Frame().bottom + 16);
	
	fVertSeparator = new BBox(r);
	AddChild(fVertSeparator);
	
	r.Set(fVertSeparator->Frame().right, 
			fVertSeparator->Frame().top,
			fVertSeparator->Frame().right, 0);
	fSaveButton = new BButton(r,"_save_button","Save", new BMessage('SAVE'));
	fSaveButton->ResizeToPreferred();
	fSaveButton->MakeDefault(true);
	fSaveButton->SetTarget(this);
	AddChild(fSaveButton);
	
	r.Set(fVertSeparator->Frame().right,
			fSaveButton->Frame().bottom + 16,
			0, 0);
	fDefaultButton = new BButton(r, "_default_button", "Default", new BMessage('DFLT'));
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetTarget(this);
	AddChild(fDefaultButton);
	
	r.Set(fVertSeparator->Frame().right,
			fDefaultButton->Frame().bottom + 16,
			0, 0);
	fRevertButton = new BButton(r, "_revert_button", "Revert", new BMessage('RVRT'));
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetTarget(this);
	AddChild(fRevertButton);
	
	float width = Window()->Frame().Width() - fVertSeparator->Frame().right;
	float diff = width - fDefaultButton->Frame().Width() + fVertSeparator->Frame().Width();
	float const x = diff / 2;
	
	float buttonHeight = fDefaultButton->Frame().Height();
	float buttonSpace = fRevertButton->Frame().top - fDefaultButton->Frame().bottom; 
	float totalSpace = (buttonHeight * 3) + (buttonSpace * 2); // three buttons, two spaces
	float totalHeight = fVertSeparator->Frame().Height();
	float const y = (totalHeight - totalSpace) / 2;
	
	fSaveButton->MoveBy(x, y);
	fDefaultButton->MoveBy(x, y);
	fRevertButton->MoveBy(x, y);
	
	// first try to read the palette from the config file.  if we can't
	// then we'll use the defaults
	
	// if (......) {
	//		read from config
	// } else {
		SetDefaultPalette();
	//}
}


void
PaletteView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case 'HUE_':	
			fCurrentHue = static_cast<float>(fHueSlider->Value() / 10000.0f);
			SetPalette();
			break;
		
		case 'SAT_':
			fCurrentSaturation = static_cast<float>(fSaturationSlider->Value() / 10000.0f);
			SetPalette();
			break;

		case 'CONT':
			fCurrentContrast = static_cast<float>(fContrastSlider->Value() / 10000.0f);
			SetPalette();	
			break;
			
		case 'BRIT':
			fCurrentBrightness = static_cast<float>(fBrightnessSlider->Value() / 10000.0f);
			SetPalette();
			break;
		
		case 'GAMA':
			fCurrentGamma = static_cast<float>(fGammaSlider->Value() / 10000.0f);
			SetPalette();
			break;

		case 'SAVE':
			std::cout << "SAVE" << std::endl;
			break;
		
		case 'DFLT':
			SetDefaultPalette();
			fHueSlider->SetValue(0);
			fSaturationSlider->SetValue(10000);
			fContrastSlider->SetValue(10000);
			fBrightnessSlider->SetValue(10000);
			fGammaSlider->SetValue(20000);
			break;
		
		case 'RVRT':
			std::cout << "RVRT" << std::endl;
			break;	
				
		default:
			break;
	}
	
	BView::MessageReceived (message);
}

void
PaletteView::Draw (BRect frame)
{		
	(void)frame;

	const rgb_color_t *ntscPalette = Palette::NTSC(
					fCurrentSaturation,
					fCurrentHue,
					fCurrentContrast,
					fCurrentBrightness,
					fCurrentGamma);
	
	for (int32 i = 0; i < 64; i++) {
		fPalette[i].red = ntscPalette[i].r;
		fPalette[i].green = ntscPalette[i].g;
		fPalette[i].blue = ntscPalette[i].b;
		
	
	}


	fWorkPalette = fPalette;	
	DrawSwatchMatrix (BPoint(16, 16), fSwatchSize, 16, 4);
	DrawIndexes();
	
	BView::Draw(frame);
}


void
PaletteView::SetDefaultPalette()
{
	fCurrentSaturation = Palette::default_saturation;
	fCurrentHue = Palette::default_hue;
	fCurrentContrast = Palette::default_contrast;
	fCurrentBrightness = Palette::default_brightness;
	fCurrentGamma = Palette::default_gamma;	
	
	fParent->set_palette(Palette::intensity,
		Palette::NTSC(
			fCurrentSaturation,
			fCurrentHue,
			fCurrentContrast,
			fCurrentBrightness,
			fCurrentGamma));
		
	fWorkPalette = fPalette;
	Invalidate();
}


void
PaletteView::DrawSwatch (BPoint where, rgb_color fill)
{
	rgb_color const no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color const lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT);
	rgb_color const darken1 = tint_color(no_tint, B_DARKEN_1_TINT); 
	rgb_color const darken4 = tint_color(no_tint, B_DARKEN_4_TINT);
	
	BRect rect (where.x, where.y, where.x+fSwatchSize, where.y+fSwatchSize);
	
	SetHighColor (darken1); 
	StrokeLine (rect.LeftBottom(), rect.LeftTop()); 
	StrokeLine (rect.LeftTop(), rect.RightTop()); 
	SetHighColor (lightenmax); 
	StrokeLine (BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom()); 
	StrokeLine (rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f)); 
	rect.InsetBy (1, 1);
	
	SetHighColor (darken4); 
	StrokeLine (rect.LeftBottom(), rect.LeftTop()); 
	StrokeLine (rect.LeftTop(), rect.RightTop()); 
	SetHighColor (no_tint); 
	StrokeLine (BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom()); 
	StrokeLine (rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f)); 
	
	rect.InsetBy (1,1);
	SetHighColor (fill);
	FillRect (rect);	
}


void 
PaletteView::DrawSwatchRow (BPoint start, int32 size, int32 rowlen)
{
	if (fPalette == NULL || fWorkPalette == NULL || size <= 0 || rowlen <= 0) {
		return;
	}
	
	for (int32 i = 0; i < rowlen; i++) {
		DrawSwatch (start, fWorkPalette[i]);
		start.x += size+4;
	}
}


void
PaletteView::DrawSwatchMatrix (BPoint start, int32 size, int32 ncols, int32 nrows)
{
	if (nrows <= 0 || size <= 0 || fPalette == NULL || fWorkPalette == NULL) {
		return;
	}
	
	for (int32 y = 0; y < nrows; y++) {
		DrawSwatchRow (start, size, ncols);
		start.y += size+4;
		fWorkPalette += nrows * sizeof(rgb_color);
	}
}


void
PaletteView::DrawIndexes()
{
	char const nybbles[] = "0123456789ABCDEF";
	
	SetHighColor(0,0,0);
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

void 
PaletteView::SetPalette()
{
	fParent->set_palette(Palette::intensity, Palette::NTSC(
		fCurrentSaturation,
		fCurrentHue,
		fCurrentContrast,
		fCurrentBrightness,
		fCurrentGamma));
		
		Invalidate();
}
