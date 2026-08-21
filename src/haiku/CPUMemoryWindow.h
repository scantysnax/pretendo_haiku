
#ifndef _CPU_MEMORY_WINDOW_H_
#define _CPU_MEMORY_WINDOW_H_

#include <Window.h>

class CPUMemoryView;
class PretendoWindow;

class CPUMemoryWindow : public BWindow
{
	public:
			CPUMemoryWindow (PretendoWindow *parent);
	virtual ~CPUMemoryWindow();

	virtual bool QuitRequested();

	public:
	CPUMemoryView *View() const;
	
	private:
	PretendoWindow *fParent = nullptr;
	CPUMemoryView *fView = nullptr;
};


#endif // _CPU_MEMORY_WINDOW_H_
