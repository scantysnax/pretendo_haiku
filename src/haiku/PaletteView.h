
#ifndef _PALETTE_VIEW_H_
#define _PALETTE_VIEW_H_

#include <Button.h>
#include <Slider.h>
#include <View.h>

#include "PretendoWindow.h"
#include "Splitters.h"

#include <iostream>


class PaletteView : public BView
{
	private:
	typedef enum {
		CHANGE_HUE = 		'HUE ',
		CHANGE_SATURATION = 'SAT ',
		CHANGE_CONTRAST = 	'CONT',
		CHANGE_BRIGHTNESS = 'BRIT',
		CHANGE_GAMMA = 		'GAMA',
		SAVE_PALETTE = 		'SAVE',
		SET_DEFAULT = 		'DFLT',
		CANCEL = 			'CNCL'
	} messages;
	
	public:
			PaletteView (PretendoWindow *parent, BRect frame, int32 swatchSize);
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
	void SetPalette();
		
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
	BButton *fSaveButton = nullptr;
	BButton *fCancelButton = nullptr;
	BButton *fDefaultButton = nullptr;
	
	private:
	int32 fSwatchSize = 0;
	rgb_color *fPalette = nullptr;
	rgb_color *fWorkPalette = nullptr;
	
	private:
	float fPrevHue;
	float fPrevSaturation;
	float fPrevContrast;
	float fPrevBrightness;
	float fPrevGamma;
	
	private:
	float fCurrentHue;
	float fCurrentSaturation;
	float fCurrentContrast;
	float fCurrentBrightness;
	float fCurrentGamma;

	private:
	PretendoWindow *fParent;
	
	public:
	float Hue() {
		return fCurrentHue;
	}
	
	float Saturation() {
		return fCurrentSaturation;
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
