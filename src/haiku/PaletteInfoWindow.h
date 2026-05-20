#ifndef _PALETTE_INFO_WINDOW_H_
#define _PALETTE_INFO_WINDOW_H_

#include "Settings.h"
#include "PretendoWindow.h"
#include "PaletteInfoView.h"

class PretendoWindow;


class PaletteInfoWindow : public BWindow
{
	public:
			PaletteInfoWindow (PretendoWindow *parent);
	virtual ~PaletteInfoWindow();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual bool QuitRequested();
	
	private:
	void LoadSettings();
	void SaveSettings();
	//BMessage *fSettingsMessage = nullptr;
	
	private:
	PretendoWindow *fParent = nullptr;
	PaletteInfoView *fPaletteInfoView = nullptr;
};


#endif // _PALETTE_INFO_WINDOW_H_
