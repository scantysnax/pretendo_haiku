
#ifndef _PALETTE_VIEW_H_
#define _PALETTE_VIEW_H_

#include <View.h>
#include <Slider.h>
#include <Button.h>

#include "PretendoWindow.h"
#include "Splitters.h"

#include <iostream>


class PaletteView : public BView
{
	public:
	PaletteView (PretendoWindow *parent, BRect frame, int32 swatchSize);
	virtual ~PaletteView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:
	void DrawSwatchRow (BPoint start, int32 size, int32 rowlen);
	void SetDefaultPalette();
	void DrawSwatch (BPoint where, rgb_color fill);
	void DrawSwatchMatrix (BPoint start, int32 size, int32 ncols, int32 nrows);
	void DrawIndexes();
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
	BButton *fRevertButton = nullptr;
	BButton *fDefaultButton = nullptr;
	
	private:
	int32 fSwatchSize;
	rgb_color *fPalette;
	rgb_color *fWorkPalette;
	
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
	BMessage *fMsgChangePalette;
	
	private:
	PretendoWindow *fParent;
};

#endif // _PALETTE_VIEW_H_
