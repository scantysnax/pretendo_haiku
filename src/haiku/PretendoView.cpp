// PretendoView.cpp

#include "PretendoView.h"


// -----------------------------------------------------------------------------
// KeyCodeFromCurrentMessage
//
// Extracts the Haiku raw key code from the current key message.
//
// Parameters:
//   view - View receiving the key event.
//
// Returns:
//   Raw key code, or -1 if unavailable.
// -----------------------------------------------------------------------------
static int32
KeyCodeFromCurrentMessage (BView *view)
{
	if (!view || !view->Window()) {
		return -1;
	}

	BMessage *message = view->Window()->CurrentMessage();

	if (!message) {
		return -1;
	}

	int32 key = -1;

	if (message->FindInt32("key", &key) != B_OK) {
		return -1;
	}

	return key;
}


// -----------------------------------------------------------------------------
// PretendoView::PretendoView
//
// Constructs the main Pretendo display view and associates it with the parent
// Pretendo window.
//
// The view follows all window edges, supports custom drawing and frame-resize
// events, and uses a transparent background.
//
// Parameters:
//   frame  - Initial view frame rectangle.
//   parent - Parent Pretendo window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PretendoView::PretendoView (BRect frame, PretendoWindow *parent)
	: BView (frame, "pretendo_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fParent = parent;
	SetViewColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// PretendoView::~PretendoView
//
// Destroys the Pretendo display view and releases the cached last-frame bitmap.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PretendoView::~PretendoView()
{
	delete fLastFrameBitmap;
}


// -----------------------------------------------------------------------------
// PretendoView::MessageReceived
//
// Handles messages delivered to the Pretendo display view.
//
// Dropped filesystem references are converted into ROM-load messages containing
// the resolved file path and forwarded to the parent Pretendo window.  All
// messages are then passed to the base BView implementation.
//
// Parameters:
//   message - Message delivered to the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
PretendoView::MessageReceived (BMessage *message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		
		if (message->FindRef("refs", 0, &ref) == B_OK) {
			BEntry entry;
			BPath path;
			BMessage msg(PretendoWindow::messages::ROM_LOADED);
			
			entry.SetTo(&ref, true);
			entry.GetPath(&path);
			
			msg.AddString("rom_path", path.Path());
			fParent->PostMessage(&msg);
		}
	}
	
	BView::MessageReceived(message);
}


// -----------------------------------------------------------------------------
// PretendoView::MouseDown
//
// Gives the emulator view keyboard focus when clicked so controller input is
// only read by the emulator view in windowed mode.
//
// Parameters:
//   where - Mouse position.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::MouseDown (BPoint where)
{
	(void)where;

	MakeFocus(true);
}


// -----------------------------------------------------------------------------
// PretendoView::KeyDown
//
// Sends focused key-down events to the emulator controller input state.  This is
// used in windowed mode; fullscreen mode uses PretendoWindow::ReadKeyStates().
//
// Parameters:
//   bytes    - Key bytes.
//   numBytes - Number of key bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::KeyDown (const char *bytes, int32 numBytes)
{
	int32 key = KeyCodeFromCurrentMessage(this);

	if (key < 0) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	PretendoWindow *parent = dynamic_cast<PretendoWindow *>(Window());

	if (parent) {
		parent->HandleEmulatorKey(key, true);
	}
}


// -----------------------------------------------------------------------------
// PretendoView::KeyUp
//
// Sends focused key-up events to the emulator controller input state.  This is
// used in windowed mode; fullscreen mode uses PretendoWindow::ReadKeyStates().
//
// Parameters:
//   bytes    - Key bytes.
//   numBytes - Number of key bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::KeyUp (const char *bytes, int32 numBytes)
{
	int32 key = KeyCodeFromCurrentMessage(this);

	if (key < 0) {
		BView::KeyUp(bytes, numBytes);
		return;
	}

	PretendoWindow *parent = dynamic_cast<PretendoWindow *>(Window());

	if (parent) {
		parent->HandleEmulatorKey(key, false);
	}
}


// -----------------------------------------------------------------------------
// PretendoView::Draw
//
// Redraws the emulator display.  During normal running, the window message path
// draws directly, but Draw() must also be able to redraw the last image when
// the window is exposed after being obscured.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::Draw (BRect updateRect)
{
	(void)updateRect;

	if (fLastFrameBitmap && fLastFrameBitmap->IsValid()) {
		DrawBitmap(fLastFrameBitmap, Bounds());
		return;
	}

	if (fDisplayBitmap && fDisplayBitmap->IsValid()) {
		DrawBitmap(fDisplayBitmap, Bounds());
		return;
	}

	SetHighColor(0, 0, 0);
	FillRect(Bounds());
}


// -----------------------------------------------------------------------------
// PretendoView::CaptureLastFrame
//
// Copies the currently presented emulator frame into a persistent bitmap.  This
// bitmap is used by Draw() whenever the window is exposed while the emulator is
// paused.
//
// Parameters:
//   source - Completed emulator frame bitmap to preserve.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::CaptureLastFrame (BBitmap *source)
{
	if (!source || !source->IsValid()) {
		return;
	}

	BRect bounds = source->Bounds();

	if (!fLastFrameBitmap
		|| !fLastFrameBitmap->IsValid()
		|| fLastFrameBitmap->Bounds() != bounds
		|| fLastFrameBitmap->ColorSpace() != source->ColorSpace()) {
		delete fLastFrameBitmap;

		fLastFrameBitmap = new BBitmap(bounds, source->ColorSpace());
	}

	if (!fLastFrameBitmap || !fLastFrameBitmap->IsValid()) {
		return;
	}

	const uint8 *src = static_cast<const uint8 *>(source->Bits());
	uint8 *dst = static_cast<uint8 *>(fLastFrameBitmap->Bits());

	const size_t srcRowBytes = source->BytesPerRow();
	const size_t dstRowBytes = fLastFrameBitmap->BytesPerRow();
	const size_t copyBytes = (srcRowBytes < dstRowBytes) ? srcRowBytes : dstRowBytes;
	const int32 rows = static_cast<int32>(bounds.IntegerHeight()) + 1;

	for (int32 y = 0; y < rows; y++) {
		memcpy(dst, src, copyBytes);

		src += srcRowBytes;
		dst += dstRowBytes;
	}
}


// -----------------------------------------------------------------------------
// PretendoView::SetDisplayBitmap
//
// Stores a non-owned pointer to the emulator display bitmap.  This gives Draw()
// a persistent source to redraw from when the view is exposed.
//
// Parameters:
//   bitmap - Existing display bitmap owned by PretendoWindow.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::SetDisplayBitmap (BBitmap *bitmap)
{
	fDisplayBitmap = bitmap;
}


// -----------------------------------------------------------------------------
// PretendoView::ClearLastFrame
//
// Clears the cached display frame used for redrawing while paused.  This should
// be called when unloading or replacing a ROM so the old game's last frame does
// not remain visible.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoView::ClearLastFrame()
{
	delete fLastFrameBitmap;
	fLastFrameBitmap = nullptr;

	fDisplayBitmap = nullptr;

	SetHighColor(0, 0, 0);
	FillRect(Bounds());

	Invalidate();
}



