#include "APUScopeWindow.h"

#include <TabView.h>

#include "APUScopeView.h"
#include "Cart.h"
#include "ExpansionAudioScopeView.h"
#include "PretendoWindow.h"


namespace {

constexpr float kWindowWidth = 780.0f;
constexpr float kWindowHeight = 614.0f;

}


// -----------------------------------------------------------------------------
// APUScopeWindow::APUScopeWindow
//
// Constructs the APU Scope debugger window.
//
// The native NES APU oscilloscope is always present. A second Expansion tab is
// added when the active cartridge mapper reports expansion-audio channels.
//
// The scope view is sized to the BTabView content container rather than to the
// complete BTabView bounds. This prevents the bottom of the scope from extending
// underneath the tab view's non-content area.
//
// Parameters:
//   parent - Main Pretendo application window.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
APUScopeWindow::APUScopeWindow (PretendoWindow *parent)
	: BWindow(
		BRect(200.0f, 120.0f, 200.0f + kWindowWidth, 120.0f + kWindowHeight),
		"APU Scope", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),

	  fParent(parent)
{
	fTabView = new BTabView(Bounds(), "apu_scope_tabs");

	const BRect contentBounds = fTabView->ContainerView()->Bounds();

	fView = new APUScopeView(contentBounds, parent);
	fTabView->AddTab(fView);

	BTab *nativeTab = fTabView->TabAt(0);

	if (nativeTab != nullptr) {
		nativeTab->SetLabel("Native APU");
	}

	UpdateExpansionTab();

	AddChild(fTabView);

	SetPulseRate(16667);
}


// -----------------------------------------------------------------------------
// APUScopeWindow::~APUScopeWindow
//
// Destroys the APU Scope debugger window.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
APUScopeWindow::~APUScopeWindow()
{
}


// -----------------------------------------------------------------------------
// APUScopeWindow::UpdateExpansionTab
//
// Updates the presence of the mapper expansion-audio oscilloscope tab.
//
// The Expansion tab is present only when the active mapper exposes one or more
// oscilloscope channels through debug_audio_scope_sample().
//
// The expansion scope view is sized to the BTabView content container so its
// bottom edge remains fully visible.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
APUScopeWindow::UpdateExpansionTab()
{
	bool hasExpansionAudio = false;

	if (nes::cart.mapper() != nullptr) {
		const mapper_audio_scope_sample_t sample = nes::cart.mapper()->debug_audio_scope_sample();

		hasExpansionAudio = sample.channel_count != 0;
	}

	const bool hasExpansionTab = fTabView->CountTabs() > 1;

	if (hasExpansionAudio && !hasExpansionTab) {
		const BRect contentBounds = fTabView->ContainerView()->Bounds();

		fExpansionView = new ExpansionAudioScopeView(contentBounds);
		fTabView->AddTab(fExpansionView);

		BTab *expansionTab = fTabView->TabAt(1);

		if (expansionTab != nullptr) {
			expansionTab->SetLabel("Expansion");
		}

	} else if (!hasExpansionAudio && hasExpansionTab) {
		fTabView->Select(0);
		BTab *expansionTab = fTabView->RemoveTab(1);

		/*
		 * Deleting the BTab also deletes its associated BView.
		 * Do not delete fExpansionView separately.
		 */
		fExpansionView = nullptr;

		delete expansionTab;
	}
}


// -----------------------------------------------------------------------------
// APUScopeWindow::QuitRequested
//
// Notifies the main Pretendo window that the APU Scope window has closed.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
APUScopeWindow::QuitRequested()
{
	if (fParent) {
		fParent->APUScopeWindowClosed();
	}

	return true;
}
