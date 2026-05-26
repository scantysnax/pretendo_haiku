

#include "OAMDebugWindow.h"
#include "OAMDebugView.h"
#include "PretendoWindow.h"
#include "PatternTableWindow.h"


OAMDebugWindow::OAMDebugWindow(PretendoWindow *parent)
	:
	BWindow(BRect(240.0f, 240.0f, 689.0f, 779.0f),
		"OAM Viewer",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;

	const float kWindowW = 450.0f;
	const float kWindowH = 590.0f;

	ResizeTo(kWindowW, kWindowH);
	MoveTo(240.0f, 240.0f);

	BRect viewFrame(
		0.0f,
		0.0f,
		kWindowW - 1.0f,
		kWindowH - 1.0f
	);

	fView = new OAMDebugView(Bounds(), parent);

 	if (parent) {
		fView->SetHostPalette(parent->Palette());
 	}
 	
	AddChild(fView);

	SetPulseRate(16667);
}


OAMDebugWindow::~OAMDebugWindow()
{
}


bool
OAMDebugWindow::QuitRequested()
{
	if (fParent)
		fParent->OAMDebugWindowClosed();

	return true;
}
