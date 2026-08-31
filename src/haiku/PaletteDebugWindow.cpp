
#include "PaletteDebugWindow.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// PaletteDebugWindow::PaletteDebugWindow
//
// Creates the Palette Viewer window, initializes its PaletteDebugView, sets the
// window size and pulse rate, and restores any saved window settings.
//
// Parameters:
//   parent - Owning PretendoWindow used for debugger coordination.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
PaletteDebugWindow::PaletteDebugWindow (PretendoWindow *parent)
	: BWindow(BRect(0, 0, 0, 0), "Palette Viewer", B_FLOATING_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{			
	fParent = parent;
	fSettingsMessage = new BMessage;
	
	const float kWindowW = 430.0f;
	const float kWindowH = 560.0f;

	ResizeTo(kWindowW, kWindowH);
	MoveTo(200.0f, 200.0f);

	
	BRect viewFrame(0.0f, 0.0f, kWindowW - 1.0f, kWindowH - 1.0f);
	fView = new PaletteDebugView(viewFrame, fParent);
	AddChild(fView);

	SetPulseRate(16667);
	
	LoadSettings();
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::~PaletteDebugWindow
//
// Saves the current Palette Viewer settings and releases the settings message.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
PaletteDebugWindow::~PaletteDebugWindow()
{
	SaveSettings();
	delete fSettingsMessage;
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::MessageReceived
//
// Handles messages delivered to the Palette Viewer window. Messages not handled
// directly by this class are forwarded to BWindow.
//
// Parameters:
//   message - Message delivered to the window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugWindow::MessageReceived (BMessage *message)		
{
	BWindow::MessageReceived(message);
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::QuitRequested
//
// Handles a request to close the Palette Viewer. The owning PretendoWindow is
// notified so it can clear its PaletteDebugWindow reference and perform any
// associated tool-input cleanup.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
bool
PaletteDebugWindow::QuitRequested()
{
	if (fParent) {
		fParent->PaletteDebugWindowClosed();
	}

	return true;
}

// -----------------------------------------------------------------------------
// PaletteDebugWindow::SetExternalHighlight
//
// Forwards an externally requested palette highlight to the PaletteDebugView.
//
// The caller specifies whether the highlight belongs to the sprite or background
// palette area, the palette row, and optionally a specific palette entry.
//
// Parameters:
//   sprites - true for sprite palettes; false for background palettes.
//   palette - Palette row index, typically 0-3.
//   entry   - Palette entry index, or a sentinel value when the whole row should
//             be highlighted.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugWindow::SetExternalHighlight (bool sprites, int32 palette, int32 entry)
{
	if (fView) {
		fView->SetExternalHighlight(sprites, palette, entry);
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::ClearExternalHighlight
//
// Clears any externally requested palette highlight currently displayed by the
// PaletteDebugView.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugWindow::ClearExternalHighlight()
{
	if (fView) {
		fView->ClearExternalHighlight();
	}
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::LoadSettings
//
// Restores saved Palette Viewer window settings.
//
// The function is currently a placeholder for future persistent settings.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// ----------------------------------------------------------------------------- 
void
PaletteDebugWindow::LoadSettings()
{	
}


// -----------------------------------------------------------------------------
// PaletteDebugWindow::SaveSettings
//
// Saves the current Palette Viewer window settings.
//
// The function is currently a placeholder for future persistent settings.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PaletteDebugWindow::SaveSettings()
{	
}	

