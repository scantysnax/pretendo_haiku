
#ifndef _PALETTE_VIEW_H_
#define _PALETTE_VIEW_H_

#include <Button.h>
#include <Slider.h>
#include <View.h>

#include "PretendoWindow.h"
#include "Splitters.h"


class PaletteView : public BView
{
	private:
	typedef enum {
		CHANGE_HUE = 		'HUE ',
		CHANGE_SATURATION = 'SAT ',
		CHANGE_CONTRAST = 	'CONT',
		CHANGE_BRIGHTNESS = 'BRIT',
		CHANGE_GAMMA = 		'GAMA',
		APPLY_PALETTE =		'APLY',
		SET_DEFAULT = 		'DFLT',
		CANCEL = 			'CNCL'
	} messages;
	
	public:
			PaletteView (PretendoWindow *mainWindow, BRect frame, int32 swatchSize);
	virtual ~PaletteView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:
	void DrawSwatchRow (BPoint start, int32 size, int32 rowlen);
	void DrawSwatch (BPoint where, rgb_color fill);
	void DrawSwatchMatrix (BPoint start, int32 size, int32 ncols, int32 nrows);
	void DrawIndexes();
	
	public:
	void SetDefaultPalette();
	void UpdatePalette();
	void UpdateSliders();
	
	private:
	PretendoWindow *fMainWindow = nullptr;
		
	private:
	BSlider *fHueSlider = nullptr;
	BSlider *fSaturationSlider = nullptr;
	BSlider *fContrastSlider = nullptr;
	BSlider *fBrightnessSlider = nullptr;
	BSlider *fGammaSlider = nullptr;

	private:
	HorizontalSplitter *fHorizSplitter = nullptr;
	VerticalSplitter *fVertSplitter = nullptr;
	
	private:
	BButton *fApplyButton = nullptr;
	BButton *fCancelButton = nullptr;
	BButton *fDefaultButton = nullptr;
	
	private:
	int32 fSwatchSize = 0;
	rgb_color *fPalette = nullptr;
	rgb_color *fWorkPalette = nullptr;
	
	private:
	float fCurrentSaturation;
	float fCurrentHue;
	float fCurrentContrast;
	float fCurrentBrightness;
	float fCurrentGamma;
	
	private:
	float fPrevSaturation;
	float fPrevHue;
	float fPrevContrast;
	float fPrevBrightness;
	float fPrevGamma;

	public:
	float Saturation() {
		return fCurrentSaturation;
	}
	
	float Hue() {
		return fCurrentHue;
	}
	
	float Contrast() {
		return fCurrentContrast;
	}
	
	float Brightness() {
		return fCurrentBrightness;
	}
	
	float Gamma() {
		return fCurrentGamma;
	}
	
	public:
	void SetHue (float hue) {
		fCurrentHue = hue;
	}
	
	void SetSaturation (float saturation) {
		fCurrentSaturation = saturation;
	}
	
	void SetContrast (float contrast) {
		fCurrentContrast = contrast;
	}
	
	void SetBrightness (float brightness) {
		fCurrentBrightness = brightness;
	}
	
	void SetGamma (float gamma) {
		fCurrentGamma = gamma;
	}
	
};

#endif // _PALETTE_VIEW_H_
