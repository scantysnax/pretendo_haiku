// NameTableWindow.cpp

#include <File.h>
#include <String.h>

#include "CHRExplorerView.h"
#include "NameTableView.h"
#include "NameTableWindow.h"
#include "PretendoWindow.h"
#include "PatternTableWindow.h"


// -----------------------------------------------------------------------------
// NameTableWindow::NameTableWindow
//
// Creates a Name Table debugger window and its associated NameTableView and
// CHRExplorerView.
//
// The window is sized to contain the name-table display and CHR explorer side
// by side. The title reflects the selected NES name-table base address.
//
// Pattern-table windows are supplied so the NameTableView can coordinate
// cross-highlighting with the corresponding pattern-table debugger views.
//
// Parameters:
//   parent - Owning PretendoWindow.
//   which  - Name-table index:
//              0 = $2000
//              1 = $2400
//              2 = $2800
//              3 = $2C00
//   pt0    - Pattern Table 0 debugger window.
//   pt1    - Pattern Table 1 debugger window.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
NameTableWindow::NameTableWindow(PretendoWindow *parent, int32 which,
								 PatternTableWindow *pt1,
								 PatternTableWindow *pt2)
	: BWindow(BRect(200, 200, 200, 200), nullptr, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
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
		case 0:
			SetTitle("Name Table 1 ($2000)");
			break;

		case 1:
			SetTitle("Name Table 2 ($2400)");
			break;

		case 2:
			SetTitle("Name Table 3 ($2800)");
			break;

		case 3:
			SetTitle("Name Table 4 ($2C00)");
			break;

		default:
			SetTitle("Name Table");
			break;
	}

	BRect nameFrame(0, 0, kNameW - 1, kWindowH - 1);
	BRect explorerFrame(kNameW, 0, kNameW + kExplorerW - 1, kWindowH - 1);

	fView = new NameTableView(nameFrame, fParent, which, nullptr);
	AddChild(fView);

	fExplorer = new CHRExplorerView(explorerFrame);
	fExplorer->SetHostPalette(fParent->Palette());
	AddChild(fExplorer);

	fView->SetExplorer(fExplorer);
	fView->SetPatternTables(pt1, pt2);

	SetPulseRate(16667);

	LoadSettings();
}


// -----------------------------------------------------------------------------
// NameTableWindow::~NameTableWindow
//
// Saves persistent window settings and releases resources owned directly by
// the NameTableWindow.
//
// Child views are owned and destroyed by BWindow.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
NameTableWindow::~NameTableWindow()
{
	SaveSettings();

	delete fSettingsMessage;
}


// -----------------------------------------------------------------------------
// NameTableWindow::QuitRequested
//
// Handles a request to close the Name Table debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
NameTableWindow::QuitRequested()
{
	if (fParent) {
		fParent->NameTableWindowClosed(fWhich);
	}

	return true;
}

// -----------------------------------------------------------------------------
// NameTableWindow::MessageReceived
//
// Handles messages delivered to the Name Table debugger window.
//
// Messages not handled directly by this class are forwarded to BWindow.
//
// Parameters:
//   msg - Message received by the window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableWindow::MessageReceived(BMessage *msg)
{
	BWindow::MessageReceived(msg);
}


// -----------------------------------------------------------------------------
// NameTableWindow::SetPatternTables
//
// Updates the Pattern Table debugger windows associated with this Name Table
// view.
//
// The NameTableView uses these references for debugger coordination such as
// cross-highlighting selected or hovered CHR tiles.
//
// Parameters:
//   pt0 - Pattern Table 0 debugger window.
//   pt1 - Pattern Table 1 debugger window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableWindow::SetPatternTables(PatternTableWindow *pt1, PatternTableWindow *pt2)
{
	if (fView) {
		fView->SetPatternTables(pt1, pt2);
	}
}


// -----------------------------------------------------------------------------
// NameTableWindow::LoadSettings
//
// Restores persistent Name Table window settings.
//
// The settings implementation is currently a placeholder.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableWindow::LoadSettings()
{
	// eli: load window position/flags etc from settings
}


// -----------------------------------------------------------------------------
// NameTableWindow::SaveSettings
//
// Saves persistent Name Table window settings.
//
// The settings implementation is currently a placeholder.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
NameTableWindow::SaveSettings()
{
	// eli: save window position/flags etc to settings
}

