#ifndef PPU_WRITE_LOG_WINDOW_H_
#define PPU_WRITE_LOG_WINDOW_H_

#include <Window.h>


class PretendoWindow;
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


#endif // PPU_WRITE_LOG_WINDOW_H_
