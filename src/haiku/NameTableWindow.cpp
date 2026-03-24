// NameTableWindow.cpp

#include <File.h>
#include <String.h>

#include "CHRExplorerView.h"
#include "NameTableView.h"
#include "NameTableWindow.h"
#include "PretendoWindow.h"


NameTableWindow::NameTableWindow(PretendoWindow *parent, int32 which)
	: BWindow(BRect(200, 200, 200, 200),
	          nullptr,
	          B_FLOATING_WINDOW_LOOK,
	          B_NORMAL_WINDOW_FEEL,
	          B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;
	fWhich = which;
	fSettingsMessage = new BMessage;

	float const kExplorerH = 180.0f;

	// Window size: 256x240 + explorer
	ResizeTo(nametable_size::WIDTH,
	         nametable_size::HEIGHT + kExplorerH);

	switch (which) {
		case 0: SetTitle("Name Table 1 ($2000)"); break;
		case 1: SetTitle("Name Table 2 ($2400)"); break;
		case 2: SetTitle("Name Table 3 ($2800)"); break;
		case 3: SetTitle("Name Table 4 ($2C00)"); break;
		default: SetTitle("Name Table"); break;
	}

	BRect nameFrame(0, 0,
	                nametable_size::WIDTH - 1,
	                nametable_size::HEIGHT - 1);

	BRect explorerFrame(0,
	                     nametable_size::HEIGHT,
	                     nametable_size::WIDTH - 1,
	                     nametable_size::HEIGHT + kExplorerH - 1);

	// explorer first (so it sits below)
	fExplorer = new CHRExplorerView(explorerFrame);
	fExplorer->SetHostPalette(fParent->Palette());
	AddChild(fExplorer);

	// Main view
	fView = new NameTableView(nameFrame, fParent, which, fExplorer);
	AddChild(fView);

	SetPulseRate(16667); // approximately 60Hz

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
NameTableWindow::LoadSettings()
{
    // eli: load window position/flags etc from settings
}


void
NameTableWindow::SaveSettings()
{
    // eli: save window position/flags etc to settings
}

