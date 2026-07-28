#ifndef _PPU_MEMORY_WINDOW_H_
#define _PPU_MEMORY_WINDOW_H_

#include <Window.h>

#include "PPUMemoryView.h"
#include "PretendoWindow.h"


class PretendoWindow;
class PPUMemoryView;


// -----------------------------------------------------------------------------
// PPUMemoryWindow
//
// Floating debugger window that owns a PPUMemoryView.  The window displays a
// side-effect-free hex view of raw PPU memory.
// -----------------------------------------------------------------------------
class PPUMemoryWindow : public BWindow
{
	public:
			PPUMemoryWindow (PretendoWindow *parent);
	virtual ~PPUMemoryWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	PPUMemoryView *fView = nullptr;
};


#endif // _PPU_MEMORY_WINDOW_H_


