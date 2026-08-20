#ifndef _PPU_WRITE_LOG_WINDOW_H_
#define _PPU_WRITE_LOG_WINDOW_H_

#include <Window.h>

#include "PPUWriteLogView.h"
#include "PretendoWindow.h"

class PPUWriteLogView;

// -----------------------------------------------------------------------------
// PPUWriteLogWindow
//
// Floating debugger window that owns a PPUWriteLogView.  The window displays a
// rolling log of recent CPU writes to PPU-facing registers.
// -----------------------------------------------------------------------------
class PPUWriteLogWindow : public BWindow
{
	public:
			PPUWriteLogWindow (PretendoWindow *parent);
	virtual ~PPUWriteLogWindow();

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	PPUWriteLogView *fView = nullptr;
};


#endif // _PPU_WRITE_LOG_WINDOW_H_
