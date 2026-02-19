
#ifndef _NAMETABLE_WINDOW_H_
#define _NAMETABLE_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>

class NameTableView;
class PretendoWindow;

class NameTableWindow : public BWindow
{
	public:
			NameTableWindow(PretendoWindow *parent, int32 which);
	virtual ~NameTableWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	public:
	void blah() { };
	

	
	private:
	void LoadSettings();
	void SaveSettings();
	
	private:
	int32 fWhich = 0;
	NameTableView *fView = nullptr;
	PretendoWindow *fParent = nullptr;
	
	
	private:
	BMessage *fSettingsMessage;
};

#endif // _NAMETABLE_WINDOW_H_
