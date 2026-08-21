
#include "CPUMemoryWindow.h"

#include "CPUMemoryView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// CPUMemoryWindow::CPUMemoryWindow
//
// Creates the CPU memory viewer window and installs the memory view.  The window
// is tall enough to show a complete 256-byte page, such as zero page $0000-$00FF,
// without scrolling when positioned at the start of that page.  The window is
// also wide enough to show address, hex bytes, ASCII, region labels, and the
// vertical scrollbar without clipping.
//
// Parameters:
//   parent - Main emulator window that owns this tool window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUMemoryWindow::CPUMemoryWindow(PretendoWindow *parent)
	:
	BWindow(BRect(80.0f, 80.0f, 960.0f, 628.0f),
			"CPU Memory", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fParent(parent)
{
	SetPulseRate(100000);
	SetSizeLimits(760.0f, 32767.0f, 540.0f, 32767.0f);

	fView = new CPUMemoryView(Bounds(), parent);
	AddChild(fView);
}

// -----------------------------------------------------------------------------
// CPUMemoryWindow::~CPUMemoryWindow
//
// Destroys the CPU memory viewer window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
CPUMemoryWindow::~CPUMemoryWindow()
{
}


// -----------------------------------------------------------------------------
// CPUMemoryWindow::View
//
// Returns the CPU memory view owned by this tool window.
//
// Parameters:
//   None.
//
// Returns:
//   Pointer to the CPUMemoryView contained in this window.
// -----------------------------------------------------------------------------
CPUMemoryView*
CPUMemoryWindow::View() const
{
	return fView;
}


// -----------------------------------------------------------------------------
// CPUMemoryWindow::QuitRequested
//
// Notifies the main emulator window that the CPU memory viewer has closed so the
// parent can clear its stored pointer and release tool-input ownership.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
CPUMemoryWindow::QuitRequested()
{
	if (fParent) {
		fParent->CPUMemoryWindowClosed();
	}

	return true;
}
