#ifndef _CPU_TRACE_WINDOW_H_
#define _CPU_TRACE_WINDOW_H_

#include <Window.h>

#include "CPUTraceView.h"
#include "PretendoWindow.h"

class CPUTraceView;
class PretendoWindow;


// -----------------------------------------------------------------------------
// CPUTraceWindow
//
// Floating debugger window that owns a CPUTraceView.
// -----------------------------------------------------------------------------
class CPUTraceWindow : public BWindow
{
	public:
	typedef enum : uint32 {
		TOGGLE_FREEZE 	= 'TGLF',
		CLEAR_TRACE		= 'CLTR',
		FOLLOW_NEWEST	= 'CFLN'
	} messages;
	
	public:
			CPUTraceWindow (PretendoWindow *parent);
	virtual ~CPUTraceWindow();

	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);

	private:
	PretendoWindow *fParent = nullptr;
	CPUTraceView *fView = nullptr;
	
};


#endif // _CPU_TRACE_WINDOW_H_
