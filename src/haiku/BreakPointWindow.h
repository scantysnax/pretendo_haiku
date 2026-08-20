
#ifndef _BREAKPOINT_WINDOW_H_
#define _BREAKPOINT_WINDOW_H_

#include <Window.h>

#include "BreakPointView.h"

class PretendoWindow;
class BreakPointView;


// -----------------------------------------------------------------------------
// BreakpointWindow
//
// Floating debugger window containing BreakpointView.
// -----------------------------------------------------------------------------
class BreakPointWindow : public BWindow
{
	public:
			BreakPointWindow (PretendoWindow *parent);
	virtual	~BreakPointWindow();

	private:
	PretendoWindow *fParent = nullptr;
	BreakPointView *fView = nullptr;
};


#endif
