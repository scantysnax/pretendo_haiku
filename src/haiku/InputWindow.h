
#ifndef _INPUT_WINDOW_H_
#define _INPUT_WINDOW_H_

#include <Window.h>

#include "InputView.h"
#include "Settings.h"
<<<<<<< HEAD
=======

>>>>>>> 17416c7e6a29d65ae4c69582bf8e7d9c6f1c05de

class PretendoWindow;


class InputWindow : public BWindow
{
	public:
			InputWindow (PretendoWindow *parent);
	virtual ~InputWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	public:
	void SetDefaultKeys();
	
	private:
	void LoadSettings();
	void SaveSettings();
	BMessage *fSettingsMessage = nullptr;
	
	private:
	PretendoWindow *fParent = nullptr;
	InputView *fInputView = nullptr;
	
};


#endif // _INPUT_WINDOW_H_
	
	
