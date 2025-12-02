
#ifndef _PALETTE_WINDOW_H_
#define _PALETTE_WINDOW_H_

#include <Window.h>


class PretendoWindow;
class PaletteView;


class PaletteWindow : public BWindow
{	
	public:
			PaletteWindow (PretendoWindow *parent);
	virtual ~PaletteWindow();
	
	public:
	virtual bool QuitRequested();
	
	private:
	void LoadSettings();
	void SaveSettings();
	
	private:
	PaletteView *fPaletteView = nullptr;
	PretendoWindow *fParent = nullptr;
	
	private:
	BMessage *fSettingsMessage = nullptr;
};


#endif //_PALETTE_WINDOW_H_
