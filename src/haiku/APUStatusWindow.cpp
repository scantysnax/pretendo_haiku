#include "APUStatusWindow.h"

#include <TabView.h>

#include "APUStatusView.h"
#include "Cart.h"
#include "ExpansionAudioStatusView.h"
#include "PretendoWindow.h"


namespace {

constexpr float kWindowWidth = 850.0f;
constexpr float kWindowHeight = 648.0f;

}


// -----------------------------------------------------------------------------
// APUStatusWindow::APUStatusWindow
//
// Creates the APU Status debugger window.
//
// The native NES APU status view is installed as the first page of a tab view.
// A mapper-specific expansion-audio status tab is added dynamically when the
// loaded cartridge exposes expansion-audio debugger state.
//
// Parameters:
//   mainWindow - Main Pretendo emulator window.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUStatusWindow::APUStatusWindow (PretendoWindow *mainWindow)
	: BWindow(BRect(100.0f, 100.0f, 100.0f + kWindowWidth, 100.0f + kWindowHeight),
		"APU Status", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),

	  fMainWindow(mainWindow)
{
	BRect bounds = Bounds();

	fTabView = new BTabView(bounds, "apu_status_tabs");
	fView = new APUStatusView(fTabView->Bounds());
	fTabView->AddTab(fView);

	BTab *nativeTab = fTabView->TabAt(0);

	if (nativeTab != nullptr) {
		nativeTab->SetLabel("Native APU");
	}

	UpdateExpansionTab();

	AddChild(fTabView);
}


// -----------------------------------------------------------------------------
// APUStatusWindow::~APUStatusWindow
//
// Destroys the APU Status debugger window.
//
// Child views and their tabs are owned by the window hierarchy and are
// destroyed with the window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUStatusWindow::~APUStatusWindow()
{
}


// -----------------------------------------------------------------------------
// APUStatusWindow::UpdateExpansionTab
//
// Updates the optional expansion-audio status tab.
//
// The tab is present only when the currently loaded mapper exposes one or more
// expansion-audio channels through debug_audio_state().  If expansion audio is
// no longer available, the tab is removed and the native APU tab is selected.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusWindow::UpdateExpansionTab()
{
	bool hasExpansionAudio = false;

	if (nes::cart.mapper() != nullptr) {
		const mapper_audio_debug_state_t state = nes::cart.mapper()->debug_audio_state();

		hasExpansionAudio = state.channel_count != 0;
	}

	const bool hasExpansionTab = fTabView->CountTabs() > 1;

	if (hasExpansionAudio && !hasExpansionTab) {
		fExpansionView = new ExpansionAudioStatusView(fTabView->Bounds());
		fTabView->AddTab(fExpansionView);

		BTab *expansionTab = fTabView->TabAt(1);

		if (expansionTab != nullptr) {
			expansionTab->SetLabel("Expansion");
		}

	} else if (!hasExpansionAudio && hasExpansionTab) {
		fTabView->Select(0);
		BTab *expansionTab = fTabView->RemoveTab(1);

		/*
		 * Deleting the BTab also deletes its associated BView,
		 * so do not delete fExpansionView separately.
		 */
		fExpansionView = nullptr;

		delete expansionTab;
	}
}


// -----------------------------------------------------------------------------
// APUStatusWindow::QuitRequested
//
// Notifies the main Pretendo window that the APU Status debugger is closing so
// it can clear its stored window pointer.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUStatusWindow::QuitRequested()
{
	if (fMainWindow) {
		fMainWindow->APUStatusWindowClosed();
	}

	return true;
}


// -----------------------------------------------------------------------------
// APUStatusWindow::MessageReceived
//
// Handles messages sent to the APU Status window.
//
// Channel enable/disable messages from the native APU Status checkboxes are
// forwarded to PretendoWindow so the existing audio menu handlers remain the
// single point responsible for muting and unmuting native APU channels.
//
// Parameters:
//   message - Message received by the window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUStatusWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case PretendoWindow::messages::ENABLE_SQ1:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_SQ1);
			}
			break;

		case PretendoWindow::messages::ENABLE_SQ2:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_SQ2);
			}
			break;

		case PretendoWindow::messages::ENABLE_TRI:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_TRI);
			}
			break;

		case PretendoWindow::messages::ENABLE_NOISE:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_NOISE);
			}
			break;

		case PretendoWindow::messages::ENABLE_DMC:
			if (fMainWindow) {
				fMainWindow->PostMessage(PretendoWindow::messages::ENABLE_DMC);
			}
			break;
	}

	BWindow::MessageReceived(message);
}

