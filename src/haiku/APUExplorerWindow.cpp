#include "APUExplorerWindow.h"


// -----------------------------------------------------------------------------
// APUExplorerWindow::APUExplorerWindow
//
// Creates the APU Explorer debugger window and installs a tabbed interface.
//
// The existing APUExplorerView is placed on the Native APU tab. The Expansion
// tab is then synchronized with the currently loaded mapper through
// UpdateExpansionTab().
//
// Parameters:
//   parent - Owning PretendoWindow. Used for lifecycle notification.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerWindow::APUExplorerWindow (PretendoWindow *parent)
	: BWindow (BRect(180.0f, 40.0f, 1060.0f, 800.0f),
				"APU Explorer", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fParent = parent;

	fTabView = new BTabView(Bounds(), "apu_explorer_tabs");

	/*
	 * Native APU tab.
	 */
	BView *nativeContainer = new BView(fTabView->Bounds(), "native_apu_container",
									   B_FOLLOW_ALL, B_WILL_DRAW);
									   nativeContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fView = new APUExplorerView(nativeContainer->Bounds(), parent);
	nativeContainer->AddChild(fView);
	fTabView->AddTab(nativeContainer);

	BTab *nativeTab = fTabView->TabAt(0);
	if (nativeTab != nullptr) {
		nativeTab->SetLabel("Native APU");
	}

	/*
	 * Add or remove the Expansion tab according to the currently
	 * loaded mapper's expansion-audio capabilities.
	 */
	UpdateExpansionTab();

	AddChild(fTabView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// APUExplorerWindow::~APUExplorerWindow
//
// Destroys the APU Explorer debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUExplorerWindow::~APUExplorerWindow()
{
}


// -----------------------------------------------------------------------------
// APUExplorerWindow::QuitRequested
//
// Notifies the owning PretendoWindow that the APU Explorer has closed so its
// tool-window and input state can be updated.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUExplorerWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUExplorerWindowClosed();
	}

	return true;
}


// -----------------------------------------------------------------------------
// APUExplorerWindow::UpdateExpansionTab
//
// Synchronizes the Expansion tab with the currently loaded mapper.
//
// The tab is present only when the current mapper exposes one or more
// expansion-audio channels through the debugger interface. It is added when
// expansion audio becomes available and removed when it is no longer present.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUExplorerWindow::UpdateExpansionTab()
{
	bool hasExpansionAudio = false;

	if (nes::cart.mapper() != nullptr) {
		const mapper_audio_debug_state_t state = nes::cart.mapper()->debug_audio_state();
		hasExpansionAudio = state.channel_count != 0;
	}

	const bool hasExpansionTab = fTabView->CountTabs() > 1;

	if (hasExpansionAudio && !hasExpansionTab) {
		fExpansionView = new ExpansionAudioExplorerView(fTabView->Bounds(), fParent);
		fTabView->AddTab(fExpansionView);

		BTab *expansionTab = fTabView->TabAt(1);

		if (expansionTab != nullptr) {
			expansionTab->SetLabel("Expansion");
		}

	} else if (!hasExpansionAudio && hasExpansionTab) {
		/*
		 * Return to the Native APU tab before removing the
		 * mapper-dependent Expansion tab.
		 */
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



