
#ifndef _ZERO_PAGE_WINDOW_H_
#define _ZERO_PAGE_WINDOW_H_

#include "PretendoWindow.h"
#include "ZeroPageView.h"


class ZeroPageView;


// -----------------------------------------------------------------------------
// ZeroPageWindow
//
// Floating debugger window that owns a ZeroPageView.
// -----------------------------------------------------------------------------
class ZeroPageWindow : public BWindow
{
	public:
			ZeroPageWindow(PretendoWindow *parent);
	virtual ~ZeroPageWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	ZeroPageView *fView = nullptr;
};


#endif // _ZERO_PAGE_WINDOW_H_

