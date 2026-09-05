
#ifndef _APU_WRITE_LOG_WINDOW_H_
#define _APU_WRITE_LOG_WINDOW_H_

#include <Window.h>

#include "APUWriteLogView.h"
#include "PretendoWindow.h"

class APUWriteLogView;

// -----------------------------------------------------------------------------
// APUWriteLogWindow
//
// Floating debugger window that owns an APUWriteLogView.  The window displays a
// rolling log of recent CPU writes to APU registers.
// -----------------------------------------------------------------------------
class APUWriteLogWindow : public BWindow
{
	public:
			APUWriteLogWindow (PretendoWindow *parent);
	virtual ~APUWriteLogWindow();

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	APUWriteLogView *fView = nullptr;
};


#endif // _APU_WRITE_LOG_WINDOW_H_
