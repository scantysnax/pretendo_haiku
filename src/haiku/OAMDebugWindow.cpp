#include "OAMDebugWindow.h"

#include "CHRExplorerView.h"
#include "OAMDebugView.h"
#include "PatternTableWindow.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// OAMDebugWindow::OAMDebugWindow
//
// Creates the OAM debugger window.  The left side contains the OAMDebugView,
// and the right side contains a CHRExplorerView used to inspect the selected
// sprite tile.
//
// The constructor sizes the window, creates both child views, shares the host
// palette from the parent Pretendo window, connects the CHR explorer to the OAM
// view, and enables regular pulse updates.
//
// Parameters:
//   parent - Owning PretendoWindow.  Used for palette access and debugger
//            window lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
OAMDebugWindow::OAMDebugWindow(PretendoWindow* parent)
	: BWindow(BRect(240.0f, 240.0f, 240.0f, 240.0f),
				"OAM Viewer",
				B_FLOATING_WINDOW_LOOK,
				B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			)
{
	fParent = parent;

	const float kOAMW = 450.0f;
	const float kExplorerW = CHRExplorerView::PreferredWidth();

	const float kWindowW = kOAMW + kExplorerW;
	const float kWindowH = 590.0f;

	ResizeTo(kWindowW, kWindowH);
	MoveTo(240.0f, 240.0f);

	BRect oamFrame(
		0.0f,
		0.0f,
		kOAMW - 1.0f,
		kWindowH - 1.0f
	);

	BRect explorerFrame(
		kOAMW,
		0.0f,
		kOAMW + kExplorerW - 1.0f,
		kWindowH - 1.0f
	);

	fView = new OAMDebugView(oamFrame, parent);

	if (parent) {
		fView->SetHostPalette(parent->Palette());
	}

	AddChild(fView);

	fExplorer = new CHRExplorerView(explorerFrame);

	if (parent) {
		fExplorer->SetHostPalette(parent->Palette());
	}

	AddChild(fExplorer);

	fView->SetExplorer(fExplorer);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// OAMDebugWindow::~OAMDebugWindow
//
// Destroys the OAM debugger window.  Child views are owned by the BWindow child
// hierarchy and are cleaned up by the window system.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
OAMDebugWindow::~OAMDebugWindow()
{
}


// -----------------------------------------------------------------------------
// OAMDebugWindow::QuitRequested
//
// Handles the OAM debugger window close request.  The parent Pretendo window is
// notified so it can clear its OAM window pointer and allow the debugger window
// to be opened again later.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
OAMDebugWindow::QuitRequested()
{
	if (fParent) {
		fParent->OAMDebugWindowClosed();
	}

	return true;
}


// -----------------------------------------------------------------------------
// OAMDebugWindow::SetPatternTables
//
// Forwards PatternTableWindow references to the contained OAMDebugView.  This
// lets the OAM debugger highlight the pattern table tile used by the currently
// selected or hovered sprite.
//
// Parameters:
//   pt0 - Pattern table window for pattern table 0 / CHR $0000.
//   pt1 - Pattern table window for pattern table 1 / CHR $1000.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugWindow::SetPatternTables(PatternTableWindow *pt0, PatternTableWindow *pt1)
{
	if (fView) {
		fView->SetPatternTables(pt0, pt1);
	}
}

