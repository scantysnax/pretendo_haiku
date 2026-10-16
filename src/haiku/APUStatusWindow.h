
#ifndef _APU_STATUS_WINDOW_H_
#define _APU_STATUS_WINDOW_H_

#include <Window.h>


class APUStatusView;
class BTabView;
class ExpansionAudioStatusView;
class PretendoWindow;


// -----------------------------------------------------------------------------
// APUStatusWindow
//
// Hosts the APU Status debugger.
//
// The debugger uses a tabbed layout so native NES APU state and mapper-provided
// expansion-audio state can be presented within the same debugger window.
// -----------------------------------------------------------------------------
class APUStatusWindow : public BWindow
{
	public:
			APUStatusWindow (PretendoWindow *mainWindow);
	virtual ~APUStatusWindow();

	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);

	public:
	void UpdateExpansionTab();

	private:
	PretendoWindow *fMainWindow = nullptr;

	BTabView *fTabView = nullptr;

	APUStatusView *fView = nullptr;
	ExpansionAudioStatusView *fExpansionView = nullptr;
};


#endif // _APU_STATUS_WINDOW_H_
