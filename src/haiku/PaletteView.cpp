
#include "Palette.h"
#include "PaletteView.h"

#include <cstdio>


PaletteView::PaletteView (PretendoWindow *parent, BRect frame, int32 swatchSize)
	: BView (frame, "palette_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fParent = parent;
	fSwatchSize = swatchSize;
	
	fPalette = new rgb_color[64];
	
	//SetDefaultPalette();
}


PaletteView::~PaletteView()
{
	if (fPalette != nullptr) {
		delete[] fPalette;
	}
}


void
PaletteView::AttachedToWindow()
{
	SetViewColor (ui_color(B_PANEL_BACKGROUND_COLOR));
	
	int32 x, y, width, height;
	x = fSwatchSize;
	y = (fSwatchSize*5)+32;
	width = (fSwatchSize*16)+64;
	fHorizSplitter = new HorizontalSplitter(x, y, width);
	AddChild(fHorizSplitter);
	
	float const left = 16.0f;
	float const right = Frame().Width() * 0.66f;
	BRect r;
	                
	r.Set(left, fHorizSplitter->Frame().top+32, right, 0);
	fHueSlider = new BSlider (r, "hue_slider", "Hue", new BMessage(messages::CHANGE_HUE),
		-10000, +10000);
	fHueSlider->SetLimitLabels("-1.0 (-30°)", "1.0 (30°)");
	fHueSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);	
	fHueSlider->SetHashMarkCount(25);
	fHueSlider->SetValue(0);
	fHueSlider->SetTarget(this);
	AddChild(fHueSlider);

		
	r.Set(left, fHueSlider->Frame().bottom+32, right, 0);
	fSaturationSlider = new BSlider (r, "sat_slider", "Saturation", 
		new BMessage(messages::CHANGE_SATURATION), 0, 50000);
	fSaturationSlider->SetLimitLabels("0.0 (grayscale)", "5.0");
	fSaturationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSaturationSlider->SetHashMarkCount(25);
	fSaturationSlider->SetTarget(this);
	fSaturationSlider->SetValue(10000);
	AddChild(fSaturationSlider);

	r.Set(left, fSaturationSlider->Frame().bottom+32, right, 0);
	fContrastSlider = new BSlider (r, "contrast_slider", "Contrast", 
		new BMessage(messages::CHANGE_CONTRAST), 5000, 20000);
	fContrastSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fContrastSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fContrastSlider->SetHashMarkCount(25);
	fContrastSlider->SetTarget(this);
	fContrastSlider->SetValue(10000);
	AddChild(fContrastSlider);

	r.Set(left, fContrastSlider->Frame().bottom+32, right, 0);
	fBrightnessSlider = new BSlider (r, "brightness_slider", "Brightness", 
		new BMessage(messages::CHANGE_BRIGHTNESS), 5000, 20000);
	fBrightnessSlider->SetLimitLabels("0.5 (reduced)", "2.0");
	fBrightnessSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBrightnessSlider->SetHashMarkCount(25);
	fBrightnessSlider->SetTarget(this);
	fBrightnessSlider->SetValue(10000);
	AddChild(fBrightnessSlider);

	r.Set(left, fBrightnessSlider->Frame().bottom+32, right, 0);
	fGammaSlider = new BSlider (r, "gamma_slider", "Gamma", 
		new BMessage(messages::CHANGE_GAMMA), 10000, 25000);
	fGammaSlider->SetLimitLabels("1.0", "2.5");
	fGammaSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGammaSlider->SetHashMarkCount(25);
	fGammaSlider->SetTarget(this);
	fGammaSlider->SetValue(20000);
	AddChild(fGammaSlider);		

	x = fHueSlider->Frame().right + 16;
	y = fHueSlider->Frame().top - 4;
	height = fGammaSlider->Frame().bottom + 16;
	fVertSplitter = new VerticalSplitter(x, y, height);
	AddChild(fVertSplitter);
	
	r.Set(fVertSplitter->Frame().right + 0, 
			fVertSplitter->Frame().top,
			fVertSplitter->Frame().right, 0);
	fSaveButton = new BButton(r,"save_button","Save", new BMessage(messages::SAVE_PALETTE));
	fSaveButton->ResizeToPreferred();
	//fSaveButton->MakeDefault(true);
	fSaveButton->SetTarget(this);
	AddChild(fSaveButton);
	
	r.Set(fVertSplitter->Frame().right,
			fSaveButton->Frame().bottom + 16,
			0, 0);
	fDefaultButton = new BButton(r, "default_button", "Default", new BMessage(messages::SET_DEFAULT));
	fDefaultButton->ResizeToPreferred();
	fDefaultButton->SetTarget(this);
	AddChild(fDefaultButton);
	
	r.Set(fVertSplitter->Frame().right,
			fDefaultButton->Frame().bottom + 16,
			0, 0);
	fCancelButton = new BButton(r, "cancel_button", "Cancel", new BMessage(messages::CANCEL));
	fCancelButton->ResizeToPreferred();
	fCancelButton->SetTarget(this);
	AddChild(fCancelButton);


	float windowWidth = Window()->Frame().Width() - fVertSplitter->Frame().right;
	float diff = windowWidth - fDefaultButton->Frame().Width() + fVertSplitter->Frame().Width();
	float const x2 = diff / 2;
	
	float buttonHeight = fDefaultButton->Frame().Height();
	float buttonSpace = fCancelButton->Frame().top - fDefaultButton->Frame().bottom; 
	float totalSpace = (buttonHeight * 3) + (buttonSpace * 2); // three buttons, two spaces
	float totalHeight = fVertSplitter->Frame().Height();
	float const y2 = (totalHeight - totalSpace) / 2;
	
	fSaveButton->MoveBy(x2, y2);
	fDefaultButton->MoveBy(x2, y2);
	fCancelButton->MoveBy(x2, y2);

	// first try to read the palette from the config file.  if we can't
	// then we'll use the defaults
	
	// if (...) {
	//		read from config
	// } else {
		SetDefaultPalette();
	//}
}


void
PaletteView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case messages::CHANGE_HUE:	
			fCurrentHue = static_cast<float>(fHueSlider->Value() / 10000.0f);
			SetPalette();
			break;
		
		case messages::CHANGE_SATURATION:
			fCurrentSaturation = static_cast<float>(fSaturationSlider->Value() / 10000.0f);
			SetPalette();
			break;

		case messages::CHANGE_CONTRAST:
			fCurrentContrast = static_cast<float>(fContrastSlider->Value() / 10000.0f);
			SetPalette();	
			break;
			
		case messages::CHANGE_BRIGHTNESS:
			fCurrentBrightness = static_cast<float>(fBrightnessSlider->Value() / 10000.0f);
			SetPalette();
			break;
		
		case messages::CHANGE_GAMMA:
			fCurrentGamma = static_cast<float>(fGammaSlider->Value() / 10000.0f);
			SetPalette();
			break;

		case messages::SAVE_PALETTE:
			std::cout << "SAVE" << std::endl;
			break;
		
		case messages::SET_DEFAULT:
			SetDefaultPalette();
			fHueSlider->SetValue(0);
			fSaturationSlider->SetValue(10000);
			fContrastSlider->SetValue(10000);
			fBrightnessSlider->SetValue(10000);
			fGammaSlider->SetValue(20000);
			break;
		
		case messages::CANCEL:
			std::cout << "CANCEL" << std::endl;
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
