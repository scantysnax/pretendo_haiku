
#ifndef _APU_SCOPE_WINDOW_H_
#define _APU_SCOPE_WINDOW_H_

#include <Window.h>

class APUScopeView;
class PretendoWindow;

class APUScopeWindow : public BWindow
{
	public:
			APUScopeWindow(
				PretendoWindow *parent);

	virtual ~APUScopeWindow();

	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	APUScopeView *fView = nullptr;
};

#endif
