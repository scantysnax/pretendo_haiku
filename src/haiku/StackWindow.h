#ifndef _STACK_WINDOW_H_
#define _STACK_WINDOW_H_

#include <Window.h>

#include "StackView.h"
#include "PretendoWindow.h"


class StackView;


// -----------------------------------------------------------------------------
// StackWindow
//
// Floating debugger window that owns a StackView.
// -----------------------------------------------------------------------------
class StackWindow : public BWindow
{
	public:
			StackWindow (PretendoWindow *parent);
	virtual ~StackWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	StackView *fView = nullptr;
};



#endif // _STACK_WINDOW_H_


