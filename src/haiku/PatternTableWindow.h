#ifndef _PATTERNTABLE_WINDOW_H_
#define _PATTERNTABLE_WINDOW_H_

#include <Window.h>

#include "PatternTableView.h"


class PretendoWindow;


class PatternTableWindow : public BWindow
{
	public:
			PatternTableWindow(PretendoWindow *parent, int32 which);
	virtual ~PatternTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	virtual void Zoom (BPoint origin, float width, float height);
	
	public:
	void LoadSettings();
	void SaveSettings();
		
	private:
	PatternTableView *fView = nullptr;
	PretendoWindow *fParent = nullptr;
	
	BMessage *fSettingsMessage = nullptr;
};

#endif // _PATTERNTABLE_WINDOW_H_
