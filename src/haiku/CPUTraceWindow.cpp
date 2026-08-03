
#include "CPUTraceWindow.h"


// -----------------------------------------------------------------------------
// CPUTraceWindow::CPUTraceWindow
//
// Creates the CPU trace debugger window and installs the CPUTraceView child.
//
// Parameters:
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
CPUTraceWindow::CPUTraceWindow(PretendoWindow *parent)
	:
	BWindow(
		BRect(180.0f, 180.0f, 900.0f, 720.0f),
		"CPU Trace",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	),
	fParent(parent)
{
	BRect viewFrame = Bounds();

	fView = new CPUTraceView(viewFrame, parent);
	AddChild(fView);

	AddShortcut(' ', 0, new BMessage(messages::TOGGLE_FREEZE), this);
	AddShortcut('c', 0, new BMessage(messages::CLEAR_TRACE), this);
	AddShortcut('C', 0, new BMessage(messages::CLEAR_TRACE), this);
	AddShortcut(B_END, 0, new BMessage(messages::FOLLOW_NEWEST), this);
	
	AddShortcut(B_ENTER, 0, new BMessage(messages::TRACE_JUMP_DISASM), this);
	AddShortcut('d', 0, new BMessage(messages::TRACE_JUMP_DISASM), this);
	AddShortcut('D', 0, new BMessage(messages::TRACE_JUMP_DISASM), this);
	

	SetPulseRate(100000);
}

// -----------------------------------------------------------------------------
// CPUTraceWindow::~CPUTraceWindow
//
// Destructor.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUTraceWindow::~CPUTraceWindow()
{
}


// -----------------------------------------------------------------------------
// CPUTraceWindow::MessageReceived
//
// Handles explicit CPU trace window commands.  Keyboard shortcuts are installed
// with AddShortcut(), so freeze and clear do not depend on child-view keyboard
// focus or scrollbar focus.
//
// Parameters:
//   message - Incoming BeAPI message.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
CPUTraceWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case messages::TOGGLE_FREEZE:
			if (fView) {
				fView->ToggleFreeze();
			}
			break;

		case messages::CLEAR_TRACE:
			if (fView) {
				fView->ClearTrace();
			}
			break;

		case messages::FOLLOW_NEWEST:
			if (fView) {
				fView->FollowNewest();
			}
			break;
			
		case messages::TRACE_JUMP_DISASM:
			if (fView && fParent) {
				uint16 address = 0x0000;

				if (fView->SelectedTraceAddress(address)) {
					fParent->JumpCPUDisasmToAddress(address);
				}
			}
			break;	
			
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// -----------------------------------------------------------------------------
// CPUTraceWindow::QuitRequested
//
// Notifies the parent window that the CPU trace window is closing.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
CPUTraceWindow::QuitRequested()
{
	if (fParent) {
		fParent->CPUTraceWindowClosed();
	}

	return true;
}


