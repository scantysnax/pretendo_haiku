#ifndef _APU_EXPLORER_WINDOW_H_
#define _APU_EXPLORER_WINDOW_H_


#include "APUExplorerView.h"
#include "PretendoWindow.h"


class APUExplorerView;


// -----------------------------------------------------------------------------
// APUExplorerWindow
//
// Floating debugger window that owns an APUExplorerView.  The window displays
// raw APU register programming together with the current effective channel
// state.
// -----------------------------------------------------------------------------
class APUExplorerWindow : public BWindow
{
	public:
			APUExplorerWindow (PretendoWindow *parent);
	virtual ~APUExplorerWindow();

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	APUExplorerView *fView = nullptr;
};


#endif // _APU_EXPLORER_WINDOW_H_
