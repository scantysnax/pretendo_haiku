
#ifndef _PPU_STATUS_WINDOW_H_
#define _PPU_STATUS_WINDOW_H_

#include <Window.h>

#include "PPUStatusView.h"
#include "PretendoWindow.h"

class PPUStatusView;
class PretendoWindow;


// -----------------------------------------------------------------------------
// PPUStatusWindow
//
// Floating debugger window that owns a PPUStatusView.  The window presents live
// PPU register, render-state, mask, and scroll decode information.
// -----------------------------------------------------------------------------
class PPUStatusWindow : public BWindow
{
	public:
			PPUStatusWindow (PretendoWindow *parent);
	virtual ~PPUStatusWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	PPUStatusView *fView = nullptr;
};


#endif // _PPU_STATUS_WINDOW_H_
