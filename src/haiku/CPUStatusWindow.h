#ifndef _CPU_STATUS_WINDOW_H_
#define _CPU_STATUS_WINDOW_H_

#include <Window.h>

#include "CPUStatusView.h"
#include "PretendoWindow.h"

class CPUStatusView;


// -----------------------------------------------------------------------------
// CPUStatusWindow
//
// Floating debugger window that owns a CPUStatusView.  The window displays live
// 6502 CPU register, flag, instruction, and cycle state.
// -----------------------------------------------------------------------------
class CPUStatusWindow : public BWindow
{
	public:
			CPUStatusWindow (PretendoWindow *parent);
	virtual ~CPUStatusWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	CPUStatusView *fView = nullptr;
};


#endif // _CPU_STATUS_WINDOW_H_
