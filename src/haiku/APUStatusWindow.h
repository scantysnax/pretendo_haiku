#ifndef _APU_STATUS_WINDOW_H_
#define _APU_STATUS_WINDOW_H_

#include <Window.h>


class APUStatusView;
class PretendoWindow;


class APUStatusWindow : public BWindow {
	public:
	APUStatusWindow(PretendoWindow *mainWindow);

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fMainWindow = nullptr;
	APUStatusView *fView = nullptr;
};


#endif // _APU_STATUS_WINDOW_H_
