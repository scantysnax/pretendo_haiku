// NameTableWindow.cpp

#include <File.h>
#include <String.h>

#include "CHRExplorerView.h"
#include "NameTableView.h"
#include "NameTableWindow.h"
#include "PretendoWindow.h"
#include "PatternTableWindow.h"


NameTableWindow::NameTableWindow(PretendoWindow *parent, int32 which,
								 PatternTableWindow *pt0, PatternTableWindow *pt1)
	: BWindow(BRect(200, 200, 200, 200),
	          nullptr,
	          B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
	          B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;
	fWhich = which;
	fSettingsMessage = new BMessage;

	const float kNameW = 280.0f;
	const float kExplorerW = CHRExplorerView::PreferredWidth();
	const float kWindowH = CHRExplorerView::PreferredHeightForNameTable();

	ResizeTo(kNameW + kExplorerW, kWindowH);

	switch (which) {
		case 0: SetTitle("Name Table 1 ($2000)"); break;
		case 1: SetTitle("Name Table 2 ($2400)"); break;
		case 2: SetTitle("Name Table 3 ($2800)"); break;
		case 3: SetTitle("Name Table 4 ($2C00)"); break;
		default: SetTitle("Name Table"); break;
	}

	BRect nameFrame(0, 0, kNameW - 1, kWindowH - 1);
	BRect explorerFrame(kNameW, 0, kNameW + kExplorerW - 1, kWindowH - 1);

	fView = new NameTableView(nameFrame, fParent, which, nullptr);
	AddChild(fView);

	fExplorer = new CHRExplorerView(explorerFrame);
	fExplorer->SetHostPalette(fParent->Palette());
	AddChild(fExplorer);

	fView->SetExplorer(fExplorer);
	fView->SetPatternTables(pt0, pt1);

	SetPulseRate(16667);

	LoadSettings();
}

NameTableWindow::~NameTableWindow()
{
    SaveSettings();
    delete fSettingsMessage;
}


bool
NameTableWindow::QuitRequested()
{
    return true;
}


void
NameTableWindow::MessageReceived(BMessage *msg)
{
    BWindow::MessageReceived(msg);
}


void
NameTableWindow::SetPatternTables(PatternTableWindow* pt0, PatternTableWindow* pt1)
{
	if (fView)
		fView->SetPatternTables(pt0, pt1);
}


void
NameTableWindow::LoadSettings()
{
    // eli: load window position/flags etc from settings
}


void
NameTableWindow::SaveSettings()
{
    // eli: save window position/flags etc to settings
}

