
#ifndef _PALETTEVIEW_H_
#define _PALETTEVIEW_H_

#include <View.h>
#include <Box.h>
#include <Slider.h>
#include <Button.h>

#include "PretendoWindow.h"


class PaletteView : public BView
{
	public:
	PaletteView (PretendoWindow *parent, BRect frame, int32 swatchSize);
	virtual ~PaletteView();
	
	public:
	virtual void AttachedToWindow (void);
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:
	void DrawSwatchRow (BPoint start, int32 size, int32 rowlen);
	void SetDefaultPalette (void);
	void DrawSwatch (BPoint where, rgb_color fill);
	void DrawSwatchMatrix (BPoint start, int32 size, int32 ncols, int32 nrows);
	void DrawIndexes (void);
	void SetPalette (void);
		
	private:
	BSlider *fHueSlider;
	BSlider *fSaturationSlider;
	BSlider *fContrastSlider;
	BSlider *fBrightnessSlider;
	BSlider *fGammaSlider;
	
	private:
	BBox *fHorizSeparator;
	BBox *fVertSeparator;
	
	private:
	BButton *fSaveButton;
	BButton *fRevertButton;
	BButton *fDefaultButton;
	
	private:
	int32 fSwatchSize;
	rgb_color *const fPalette;
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

#endif
