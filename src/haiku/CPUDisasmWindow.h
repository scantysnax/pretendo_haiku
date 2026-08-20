
#ifndef _CPU_DISASM_WINDOW_H_
#define _CPU_DISASM_WINDOW_H_

#include <Window.h>

#include "CPUDisasmView.h"
#include "PretendoWindow.h"


class CPUDisasmView;


// -----------------------------------------------------------------------------
// CPUDisasmWindow
//
// Floating debugger window that owns a CPUDisasmView.  The window displays live
// 6502 disassembly near the current program counter.
// -----------------------------------------------------------------------------
class CPUDisasmWindow : public BWindow
{
	public:
			CPUDisasmWindow (PretendoWindow *parent);
	virtual ~CPUDisasmWindow();

	virtual bool QuitRequested();
	
	public:
	void JumpToAddress (uint16 address);

	private:
	PretendoWindow *fParent = nullptr;
	CPUDisasmView *fView = nullptr;
};


#endif // CPU_DISASM_WINDOW_H_

