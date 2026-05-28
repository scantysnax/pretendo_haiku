#include "OAMDebugWindow.h"

#include "CHRExplorerView.h"
#include "OAMDebugView.h"
#include "PatternTableWindow.h"
#include "PretendoWindow.h"


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


OAMDebugWindow::~OAMDebugWindow()
{
}


bool
OAMDebugWindow::QuitRequested()
{
	if (fParent) {
		fParent->OAMDebugWindowClosed();
	}

	return true;
}


void
OAMDebugWindow::SetPatternTables(PatternTableWindow *pt0, PatternTableWindow *pt1)
{
	if (fView) {
		fView->SetPatternTables(pt0, pt1);
	}
}

