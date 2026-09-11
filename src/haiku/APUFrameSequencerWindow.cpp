#include "APUFrameSequencerWindow.h"

#include "APUFrameSequencerView.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// APUFrameSequencerWindow::APUFrameSequencerWindow
//
// Creates the APU Frame Sequencer debugger window.
//
// The window is intentionally tall enough to accommodate the header, live
// timeline, sequencer-state panel, and recent-event history without crowding.
//
// Parameters:
//   parent - Parent Pretendo window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
APUFrameSequencerWindow::APUFrameSequencerWindow(PretendoWindow *parent)
	: BWindow(BRect(220.0f, 140.0f, 700.0f, 870.0f), "APU Frame Sequencer",
			  B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fParent(parent),
	fView(nullptr)
{
	fView = new APUFrameSequencerView(Bounds(), parent);
	AddChild(fView);

	SetPulseRate(16667); // ~60Hz
}


// -----------------------------------------------------------------------------
// APUFrameSequencerWindow::~APUFrameSequencerWindow
//
// Destroys the APU Frame Sequencer debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUFrameSequencerWindow::~APUFrameSequencerWindow()
{
}


// -----------------------------------------------------------------------------
// APUFrameSequencerWindow::QuitRequested
//
// Notifies PretendoWindow that the Frame Sequencer debugger has closed.
//
// Parameters:
//   None.
//
// Returns:
//   true so the window may close.
// -----------------------------------------------------------------------------
bool
APUFrameSequencerWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUFrameSequencerWindowClosed();
	}

	return true;
}

