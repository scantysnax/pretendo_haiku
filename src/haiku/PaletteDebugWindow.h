#ifndef _PALETTE_DEBUG_WINDOW_H_
#define _PALETTE_DEBUG_WINDOW_H_

#include "PaletteDebugView.h"
#include "PretendoWindow.h"
#include "Settings.h"

class PretendoWindow;


class PaletteDebugWindow : public BWindow
{
	public:
			PaletteDebugWindow (PretendoWindow *parent);
	virtual ~PaletteDebugWindow();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual bool QuitRequested();
	
	void SetExternalHighlight (bool sprites, int32 palette, int32 entry = -1);
	void ClearExternalHighlight();
	
	private:
	void LoadSettings();
	void SaveSettings();
	BMessage *fSettingsMessage = nullptr;
	
	private:
	PretendoWindow *fParent = nullptr;
	PaletteDebugView *fView = nullptr;
};


#endif // _PALETTE_DEBUG_WINDOW_H_
