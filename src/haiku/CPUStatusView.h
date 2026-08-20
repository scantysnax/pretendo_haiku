
#ifndef _CPU_STATUS_VIEW_H_
#define _CPU_STATUS_VIEW_H_

#include <View.h>

#include <cmath>

#include "Bus.h"
#include "Cart.h"
#include "Cpu.h"
#include "CPUDisasm.h"
#include "DebugHelpers.h"
#include "Ppu.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// CPUStatusView
//
// Debugger view for inspecting live 6502 CPU register state.  The view displays
// the program counter, public registers, stack pointer, processor status flags,
// current instruction latch, current instruction cycle, and total executed CPU
// cycles.
// -----------------------------------------------------------------------------

class CPUStatusView : public BView
{
	public:
			CPUStatusView (BRect frame, PretendoWindow *parent);
	virtual ~CPUStatusView();

	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();

	private:
	void DrawHeaderPanel();
	void DrawRegisterPanel();
	void DrawFlagsPanel();
	void DrawTimingPanel();
	
	private:
	void DrawFlag (BRect rect, const char *name, bool active);
	
	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage (BRect panel);

	private:
	PretendoWindow *fParent = nullptr;
};


#endif // _CPU_STATUS_VIEW_H_
