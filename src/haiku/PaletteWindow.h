
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
	virtual void MessageReceived (BMessage *message);
	
	private:
	PaletteView *fPaletteView = nullptr;
	PretendoWindow *fParent = nullptr;
};


#endif //_PALETTE_WINDOW_H_
