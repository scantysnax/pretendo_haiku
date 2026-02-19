
#ifndef _INPUT_WINDOW_H_
#define _INPUT_WINDOW_H_


#include "Settings.h"

#include "PretendoWindow.h"
#include "InputView.h"

class PretendoWindow;

class InputWindow : public BWindow
{
	public:
			InputWindow (PretendoWindow *parent);
	virtual ~InputWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	private:
	void LoadSettings();
	void SaveSettings();
	BMessage *fSettingsMessage = nullptr;
	
	private:
	PretendoWindow *fParent = nullptr;
	InputView *fInputView = nullptr;
};

#endif // _INPUT_WINDOW_H_
	
	
