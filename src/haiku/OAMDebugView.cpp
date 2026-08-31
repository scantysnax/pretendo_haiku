
#include "DebugHelpers.h"
#include "OAMDebugView.h"
#include "PatternTableView.h"
#include "PatternTableWindow.h"
#include "PretendoWindow.h"

#include "Cart.h"
#include "CHRExplorerView.h"

#include "Mapper.h"
#include "Ppu.h"

#include <cmath>


static const int32 kOAMSpriteCount = 64;
static const int32 kVisibleOAMRows = 16;
static const int32 kMaxFirstOAMSprite = kOAMSpriteCount - kVisibleOAMRows;


// -----------------------------------------------------------------------------
// OAMSpriteScrollBar
//
// Private helper scroll bar used by OAMDebugView.  It scrolls through the 64
// OAM sprites using the first visible sprite index.
// -----------------------------------------------------------------------------
class OAMSpriteScrollBar : public BScrollBar
{
	public:
	// -------------------------------------------------------------------------
	// OAMSpriteScrollBar::OAMSpriteScrollBar
	//
	// Creates the sprite scrollbar used by the OAM viewer.
	//
	// Parameters:
	//   frame - Scrollbar frame in the OAMDebugView coordinate space.
	//   owner - OAMDebugView that receives first-sprite changes.
	//
	// Returns:
	//   Constructor; no return value.
	// -------------------------------------------------------------------------
	OAMSpriteScrollBar(BRect frame, OAMDebugView *owner)
		:
		BScrollBar(frame, "oam_sprite_scrollbar", nullptr, 0.0f, kMaxFirstOAMSprite, B_VERTICAL),
		fOwner(owner)
	{
		SetSteps(1.0f, kVisibleOAMRows);
		SetProportion(static_cast<float>(kVisibleOAMRows) / static_cast<float>(kOAMSpriteCount));
	}

	// -------------------------------------------------------------------------
	// OAMSpriteScrollBar::ValueChanged
	//
	// Handles scrollbar movement and forwards the selected first visible OAM
	// sprite to the owning OAMDebugView.
	//
	// Parameters:
	//   value - New scrollbar value requested by the user.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	virtual void ValueChanged(float value)
	{
		if (!fOwner) {
			return;
		}

		int32 firstSprite = static_cast<int32>(value + 0.5f);

		if (firstSprite < 0) {
			firstSprite = 0;
		}

		if (firstSprite > kMaxFirstOAMSprite) {
			firstSprite = kMaxFirstOAMSprite;
		}

		fOwner->SetFirstSpriteFromScrollBar(firstSprite);
	}

	private:
	OAMDebugView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// SetPatternWindowHighlight
//
// Safely applies an external Pattern Table highlight to a PatternTableWindow.
//
// The window is locked before accessing its view because Pattern Table windows
// are separate BWindow instances.
//
// Sprite-oriented callers can request an 8x16 pair highlight and also propagate
// whether the originating external selection is currently locked.
//
// Parameters:
//   window             - Pattern Table window to update.
//   whichPT            - Pattern-table index, 0 for $0000 or 1 for $1000.
//   tileIndex          - Tile index to highlight.
//   highlight8x16Pair  - true to highlight the complete even/odd 8x16 pair.
//   externalLocked     - true when the originating external selection is locked.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static inline void
SetPatternWindowHighlight (PatternTableWindow *window, int32 whichPT, int32 tileIndex,
							bool highlight8x16Pair, bool externalLocked)
{
	if (!window) {
		return;
	}

	if (window->Lock()) {
		if (window->View()) {
			window->View()->SetExternalHighlight(whichPT, tileIndex, highlight8x16Pair, externalLocked);
		}

		window->Unlock();
	}
}


// -----------------------------------------------------------------------------
// ClearPatternWindowHighlight
//
// Safely clears any external tile highlight from a PatternTableWindow.  The
// window is locked before accessing its view because pattern table windows are
// separate BWindow instances.
//
// Parameters:
//   window - Pattern table window to update.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static inline void
ClearPatternWindowHighlight (PatternTableWindow *window)
{
	if (!window) {
		return;
	}

	if (window->Lock()) {
		if (window->View())
			window->View()->ClearExternalHighlight();

		window->Unlock();
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::OAMDebugView
//
// Creates the OAM debugger view.  The view displays a snapshot/live view of NES
// sprite OAM, selected sprite details, and links the selected sprite to the
// palette debugger, pattern table windows, and CHR explorer.
//
// Parameters:
//   frame  - View frame inside the OAM debugger window.
//   parent - Owning PretendoWindow used for palette/debugger coordination.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
OAMDebugView::OAMDebugView (BRect frame, PretendoWindow *parent)
	: BView(frame, "oam_debug_view", B_FOLLOW_ALL_SIDES,
	B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// OAMDebugView::~OAMDebugView
//
// Destroys the OAM debugger view and clears any external debugger highlights
// owned by the OAM viewer.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
OAMDebugView::~OAMDebugView()
{
	if (fParent) {
		fParent->ClearPaletteDebuggerHighlight();
	}

	ClearPatternTableHighlight();
}


// -----------------------------------------------------------------------------
// OAMDebugView::AttachedToWindow
//
// Performs setup that requires the view to be attached to a window.  This
// enables keyboard focus, mouse-wheel tracking, creates the OAM sprite
// scrollbar, captures the initial OAM snapshot, and starts the viewer in
// live-update mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS | B_MOUSE_WHEEL_CHANGED, B_NO_POINTER_HISTORY);

	if (!fSpriteScrollBar) {
		BRect listPanel(4.0f, 174.0f, Bounds().right - 4.0f, 526.0f);
		BRect scrollFrame(listPanel.right - 18.0f, listPanel.top + 26.0f,
						  listPanel.right - 4.0f,
						  listPanel.bottom - 22.0f);

		fSpriteScrollBar = new OAMSpriteScrollBar(scrollFrame, this);
		AddChild(fSpriteScrollBar);

		fSpriteScrollBar->SetValue(fFirstSprite);
	}

	/*
	 * Start in live mode.  Space can then be used to freeze the current
	 * coherent OAM/PPU snapshot when desired.
	 */
	fFreezeUpdates = false;

	if (HasROMLoaded()) {
		CaptureOAMSnapshot();

		UpdatePaletteDebuggerHighlight();
		UpdatePatternTableHighlight();
		UpdateCHRExplorer();
	} else {
		if (fParent) {
			fParent->ClearPaletteDebuggerHighlight();
		}

		ClearPatternTableHighlight();

		if (fCHRExplorer) {
			fCHRExplorer->Clear();
		}
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::Draw
//
// Draws the OAM debugger.  If no ROM is loaded, the scrollbar is hidden and the
// body shows a friendly empty-state message instead of stale/default sprite data.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	const bool hasROM = HasROMLoaded();

	if (fSpriteScrollBar) {
		if (hasROM && fSpriteScrollBar->IsHidden()) {
			fSpriteScrollBar->Show();
		} else if (!hasROM && !fSpriteScrollBar->IsHidden()) {
			fSpriteScrollBar->Hide();
		}
	}

	DrawHeaderUI();

	const float rightEdge = (fSpriteScrollBar && !fSpriteScrollBar->IsHidden())
		? fSpriteScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	if (!hasROM) {
		BRect panel(4.0f, 88.0f, rightEdge, Bounds().bottom - 8.0f);
		::DrawDebugPanel(this, panel, "OAM Sprites");
		DrawNoROMMessage(panel);
		return;
	}

	DrawOAMSummaryPanel();
	DrawSpriteListPanel();
	DrawSelectedSpritePanel();
}


// -----------------------------------------------------------------------------
// OAMDebugView::FrameResized
//
// Repositions and resizes the sprite scrollbar when the view frame changes.
//
// Parameters:
//   width  - New view width.
//   height - New view height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::FrameResized (float width, float height)
{
	BView::FrameResized(width, height);

	(void)width;
	(void)height;

	if (!fSpriteScrollBar) {
		return;
	}

	BRect listPanel(4.0f, 174.0f, Bounds().right - 4.0f, 526.0f);
	BRect scrollFrame(listPanel.right - 18.0f, listPanel.top + 26.0f,
					  listPanel.right - 4.0f, listPanel.bottom - 22.0f);

	fSpriteScrollBar->MoveTo(scrollFrame.LeftTop());
	fSpriteScrollBar->ResizeTo(scrollFrame.Width(), scrollFrame.Height());
}


// -----------------------------------------------------------------------------
// OAMDebugView::KeyDown
//
// Handles OAM viewer keyboard shortcuts.  Space toggles live/snapshot mode, R
// refreshes the current snapshot, arrow keys move the active sprite, and [/] or
// ,/. page through the 64 OAM entries.
//
// Parameters:
//   bytes    - Key bytes provided by the BeAPI input system.
//   numBytes - Number of bytes in the key sequence.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	auto clampFirstSprite = [&]() {
		if (fFirstSprite < 0) {
			fFirstSprite = 0;
		}

		if (fFirstSprite > kMaxFirstOAMSprite) {
			fFirstSprite = kMaxFirstOAMSprite;
		}
	};

	auto syncAfterSelectionChange = [&]() {
		clampFirstSprite();

		if (fSpriteScrollBar) {
			fSpriteScrollBar->SetValue(fFirstSprite);
		}

		UpdatePaletteDebuggerHighlight();
		UpdatePatternTableHighlight();
		UpdateCHRExplorer();

		Invalidate();
	};

	auto moveActiveSprite = [&](int32 delta) {
		int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

		if (active < 0 || active >= kOAMSpriteCount) {
			active = fFirstSprite;
		}

		active += delta;

		if (active < 0) {
			active = 0;
		}

		if (active >= kOAMSpriteCount) {
			active = kOAMSpriteCount - 1;
		}

		if (fSpriteLocked) {
			fLockedSprite = active;
		}

		fHoverSprite = active;

		if (active < fFirstSprite) {
			fFirstSprite = active;
		} else if (active >= fFirstSprite + kVisibleOAMRows) {
			fFirstSprite = active - kVisibleOAMRows + 1;
		}

		syncAfterSelectionChange();
	};

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;

			CaptureOAMSnapshot();

			UpdatePaletteDebuggerHighlight();
			UpdatePatternTableHighlight();
			UpdateCHRExplorer();

			Invalidate();
			break;

		case B_UP_ARROW:
			moveActiveSprite(-1);
			break;

		case B_DOWN_ARROW:
			moveActiveSprite(1);
			break;

		case '[':
		case ',':
			fFirstSprite -= kVisibleOAMRows;

			if (fFirstSprite < 0) {
				fFirstSprite = 0;
			}

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite
					|| fHoverSprite >= fFirstSprite + kVisibleOAMRows) {
					fHoverSprite = fFirstSprite;
				}
			}

			syncAfterSelectionChange();
			break;

		case ']':
		case '.':
			fFirstSprite += kVisibleOAMRows;

			if (fFirstSprite > kMaxFirstOAMSprite) {
				fFirstSprite = kMaxFirstOAMSprite;
			}

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite || fHoverSprite >= fFirstSprite + kVisibleOAMRows) {
					fHoverSprite = fFirstSprite;
				}
			}

			syncAfterSelectionChange();
			break;

		case 'r':
		case 'R':
			CaptureOAMSnapshot();

			UpdatePaletteDebuggerHighlight();
			UpdatePatternTableHighlight();
			UpdateCHRExplorer();

			Invalidate();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::MessageReceived
//
// Handles messages delivered to the view.  The OAM viewer handles mouse-wheel
// messages to scroll through the sprite list and forwards all other messages to
// BView.
//
// Parameters:
//   message - Message delivered to the view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			if (!HasROMLoaded()) {
				BView::MessageReceived(message);
				return;
			}

			float deltaY = 0.0f;

			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK) {
				BView::MessageReceived(message);
				return;
			}

			int32 oldFirstSprite = fFirstSprite;

			if (deltaY > 0.0f) {
				fFirstSprite += 1;
			} else if (deltaY < 0.0f) {
				fFirstSprite -= 1;
			}

			if (fFirstSprite < 0) {
				fFirstSprite = 0;
			}

			if (fFirstSprite > kMaxFirstOAMSprite) {
				fFirstSprite = kMaxFirstOAMSprite;
			}

			if (fFirstSprite != oldFirstSprite) {
				if (fSpriteScrollBar) {
					fSpriteScrollBar->SetValue(fFirstSprite);
				}

				if (!fSpriteLocked) {
					if (fHoverSprite < fFirstSprite
						|| fHoverSprite >= fFirstSprite + kVisibleOAMRows) {
						fHoverSprite = fFirstSprite;
					}
				}

				UpdatePaletteDebuggerHighlight();
				UpdatePatternTableHighlight();
				UpdateCHRExplorer();

				Invalidate();
			}

			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::MouseDown
//
// Handles mouse clicks in the sprite list.
//
// The row hit-test is aligned with the actual visual row rectangle so the
// clickable area matches what the user sees. Clicking a sprite locks it for
// inspection; clicking the locked sprite again unlocks it and returns to hover
// inspection.
//
// Parameters:
//   where - Click position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::MouseDown(BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	BRect listPanel(4.0f, 174.0f, Bounds().right - 4.0f, 526.0f);

	if (!listPanel.Contains(where)) {
		return;
	}

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;
	const float rowTop = firstRowY - 12.0f;

	int32 row = static_cast<int32>((where.y - rowTop) / rowH);

	if (row < 0 || row >= kVisibleOAMRows) {
		return;
	}

	int32 spriteIndex = fFirstSprite + row;

	if (spriteIndex < 0 || spriteIndex >= kOAMSpriteCount) {
		return;
	}

	if (fSpriteLocked && fLockedSprite == spriteIndex) {
		fSpriteLocked = false;
		fHoverSprite = spriteIndex;
	} else {
		fSpriteLocked = true;
		fLockedSprite = spriteIndex;
		fHoverSprite = spriteIndex;
	}

	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();
	UpdateCHRExplorer();

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::MouseMoved
//
// Handles hover inspection in the sprite list.
//
// The hover hit-test is aligned with the actual visual row rectangle so moving
// across the list selects the row directly beneath the pointer. When no sprite
// is locked, hover changes synchronize the palette debugger, Pattern Table, and
// CHR Explorer.
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   transit - BeAPI pointer transit code.
//   message - Optional drag message supplied by the input system.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::MouseMoved (BPoint where, uint32 transit, const BMessage *message)
{
	(void)message;

	if (!HasROMLoaded()) {
		fMouseInside = false;
		return;
	}

	if (transit == B_EXITED_VIEW) {
		fMouseInside = false;

		if (!fSpriteLocked) {
			fHoverSprite = -1;

			if (fParent) {
				fParent->ClearPaletteDebuggerHighlight();
			}

			ClearPatternTableHighlight();

			if (fCHRExplorer) {
				fCHRExplorer->Clear();
			}
		}

		Invalidate();
		return;
	}

	fMouseInside = true;

	if (fSpriteLocked) {
		return;
	}

	BRect listPanel(4.0f, 174.0f, Bounds().right - 4.0f, 526.0f);

	if (!listPanel.Contains(where)) {
		return;
	}

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;
	const float rowTop = firstRowY - 12.0f;

	int32 row = static_cast<int32>((where.y - rowTop) / rowH);

	if (row < 0 || row >= kVisibleOAMRows) {
		return;
	}

	int32 spriteIndex = fFirstSprite + row;

	if (spriteIndex < 0 || spriteIndex >= kOAMSpriteCount) {
		return;
	}

	if (fHoverSprite != spriteIndex) {
		fHoverSprite = spriteIndex;

		UpdatePaletteDebuggerHighlight();
		UpdatePatternTableHighlight();
		UpdateCHRExplorer();

		Invalidate();
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::Pulse
//
// Periodically refreshes the OAM debugger.
//
// If no ROM is loaded, stale cartridge-specific OAM state and linked debugger
// highlights are cleared regardless of whether the viewer is frozen.
//
// When a ROM is loaded but no valid OAM snapshot exists, one initial snapshot
// is captured even in frozen mode.  This allows an OAM window that remained
// open across ROM unload/reload, or was opened before a ROM was loaded, to
// initialize correctly.
//
// Once a valid snapshot exists, frozen mode preserves it.  Live mode captures a
// new coherent 256-byte OAM snapshot on each pulse.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::Pulse()
{
	/*
	 * ROM removal must be handled before freeze mode.  Otherwise a frozen
	 * debugger can retain the preceding cartridge's OAM snapshot and linked
	 * highlights indefinitely.
	 */
	if (!HasROMLoaded()) {
		if (fHaveFrozenOAM || fSpriteLocked || fLockedSprite >= 0 || fHoverSprite >= 0) {
			Clear();
		}

		return;
	}

	/*
	 * A newly loaded ROM needs one valid snapshot even when the user has
	 * chosen frozen mode.
	 */
	if (!fHaveFrozenOAM) {
		CaptureOAMSnapshot();

		UpdatePaletteDebuggerHighlight();
		UpdatePatternTableHighlight();
		UpdateCHRExplorer();

		Invalidate();
		return;
	}

	/*
	 * A valid frozen snapshot already exists.  Preserve it until the user
	 * explicitly refreshes it or switches back to live mode.
	 */
	if (fFreezeUpdates) {
		return;
	}

	/*
	 * Live mode: take one coherent OAM snapshot for this debugger refresh.
	 * All drawing and linked inspectors read from the same snapshot.
	 */
	CaptureOAMSnapshot();

	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();
	UpdateCHRExplorer();

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawHeaderUI
//
// Draws the controls/help panel at the top of the OAM debugger.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawHeaderUI()
{
	BRect panel(4.0f, 4.0f, Bounds().right - 4.0f, 76.0f);
	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 58.0f;
	float y = panel.top + 34.0f;

	auto drawLV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawLV("Mouse:", "hover inspect / click lock");
	drawLV("Keys:", "Up/Down sprite   Wheel scroll   [ ] page");
	drawLV("Space:", fFreezeUpdates ? "live OAM   R refresh snapshot" : "snapshot OAM   R refresh snapshot");
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawOAMSummaryPanel
//
// Draws the OAM summary panel.  The summary counts visible and hidden sprites,
// shows whether the viewer is in live or frozen snapshot mode, and reports the
// sprite-size mode and PPUCTRL value represented by the current OAM snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawOAMSummaryPanel()
{
	BRect panel(4.0f, 88.0f, Bounds().right - 4.0f, 164.0f);
	::DrawDebugPanel(this, panel, "OAM Summary");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	int32 usedSprites = 0;
	int32 hiddenSprites = 0;

	for (int32 i = 0; i < 64; i++) {
		uint8 spriteY = OAMByte((i * 4) + 0);

		if (spriteY >= 0xef) {
			hiddenSprites++;
		} else {
			usedSprites++;
		}
	}

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 70.0f;

	const float rightLabelX = panel.left + 210.0f;
	const float rightValueX = rightLabelX + 72.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BString s;

	auto drawLeftLV = [&](const char *label, const char *value, bool fixedValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(fixedValue ? &fixed : &prevFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightLV = [&](const char *label, const char *value, bool fixedValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(fixedValue ? &fixed : &prevFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	s.SetToFormat("%ld / 64", (long)usedSprites);
	drawLeftLV("OAM Used:", s.String(), false);

	s.SetToFormat("%ld", (long)hiddenSprites);
	drawLeftLV("Hidden:", s.String(), false);
	drawRightLV("State:", fFreezeUpdates ? "FROZEN" : "LIVE", false);

	const uint8 ctrl = DisplayPPUCTRL();
	drawRightLV("Mode:", (ctrl & 0x20) ? "8x16 sprites" : "8x8 sprites", false);

	s.SetToFormat("$%02X", ctrl);
	drawRightLV("PPUCTRL:", s.String(), true);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSpriteListPanel
//
// Draws the scrollable OAM sprite list.  Rows show raw OAM fields, decoded
// summary flags, hidden/offscreen state, sprite-zero marking, current selection,
// and same-tile highlighting for the active sprite.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawSpriteListPanel()
{
	BRect panel(4.0f, 174.0f, Bounds().right - 4.0f, 526.0f);
	::DrawDebugPanel(this, panel, "Sprite List");

	SetFontSize(11.0f);

	const float xIndex = panel.left + 10.0f;
	const float xY = panel.left + 50.0f;
	const float xTile = panel.left + 92.0f;
	const float xAttr = panel.left + 144.0f;
	const float xX = panel.left + 198.0f;
	const float xInfo = panel.left + 232.0f;

	float y = panel.top + 36.0f;

	SetHighColor(80, 80, 80);
	DrawString("#", BPoint(xIndex, y));
	DrawString("Y", BPoint(xY, y));
	DrawString("Tile", BPoint(xTile, y));
	DrawString("Attr", BPoint(xAttr, y));
	DrawString("X", BPoint(xX, y));
	DrawString("Info", BPoint(xInfo, y));

	y += 22.0f;

	SetHighColor(150, 150, 150);
	StrokeLine(BPoint(panel.left + 8.0f, y - 13.0f), BPoint(panel.right - 24.0f, y - 13.0f));

	const float firstRowY = y;
	const float rowH = 17.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	bool haveActiveTile = false;
	uint8 activeTile = 0;

	if (active >= 0 && active < kOAMSpriteCount) {
		uint32 activeBase = active * 4;
		uint8 activeY = OAMByte(activeBase + 0);

		if (activeY < 0xef) {
			activeTile = OAMByte(activeBase + 1);
			haveActiveTile = true;
		}
	}

	for (int32 row = 0; row < kVisibleOAMRows; row++) {
		int32 spriteIndex = fFirstSprite + row;

		if (spriteIndex < 0 || spriteIndex >= kOAMSpriteCount) {
			continue;
		}

		uint32 base = spriteIndex * 4;

		uint8 spriteY = OAMByte(base + 0);
		uint8 tile = OAMByte(base + 1);
		uint8 attr = OAMByte(base + 2);
		uint8 spriteX = OAMByte(base + 3);

		bool spriteZero = (spriteIndex == 0);
		bool hidden = spriteY >= 0xef;

		uint8 pal = attr & 0x3;
		bool priority = (attr & 0x20) != 0;
		bool flipH = (attr & 0x40) != 0;
		bool flipV = (attr & 0x80) != 0;

		bool sameTile = haveActiveTile && (spriteIndex != active) && !hidden && (tile == activeTile);
		float rowY = firstRowY + row * rowH;
		BRect rowRect(panel.left + 7.0f, rowY - 12.0f, panel.right - 24.0f, rowY + 4.0);

		if (sameTile) {
			SetHighColor(220, 232, 244);
			FillRect(rowRect);

			SetHighColor(120, 150, 180);
			StrokeRect(rowRect);
		}

		if (spriteIndex == active) {
			if (fSpriteLocked) {
				SetHighColor(255, 230, 245);
				FillRect(rowRect);

				SetHighColor(210, 80, 170);
				StrokeRect(rowRect);
			} else {
				SetHighColor(238, 238, 190);
				FillRect(rowRect);

				SetHighColor(190, 175, 80);
				StrokeRect(rowRect);
			}
		}

		if (hidden) {
			SetHighColor(115, 115, 115);
		} else {
			SetHighColor(0, 0, 0);
		}

		SetFont(&fixed);

		BString s;

		s.SetToFormat("%02ld", static_cast<long>(spriteIndex));
		DrawString(s.String(), BPoint(xIndex, rowY));

		s.SetToFormat("$%02X", spriteY);
		DrawString(s.String(), BPoint(xY, rowY));

		s.SetToFormat("$%02X", tile);
		DrawString(s.String(), BPoint(xTile, rowY));

		s.SetToFormat("$%02X", attr);
		DrawString(s.String(), BPoint(xAttr, rowY));

		s.SetToFormat("$%02X", spriteX);
		DrawString(s.String(), BPoint(xX, rowY));

		SetFont(&prevFont);

		if (hidden) {
			SetHighColor(115, 115, 115);

			if (spriteZero) {
				DrawString("Sprite 0 hidden", BPoint(xInfo, rowY));
			} else {
				DrawString("hidden", BPoint(xInfo, rowY));
			}
		} else {
			SetHighColor(0, 0, 0);

			s.SetToFormat("%s%sP%u %s%s%s", spriteZero ? "Sprite 0 " : "", sameTile ? "same " : "",
							static_cast<unsigned>(pal), priority ? "B" : "F", 
							flipH ? " H" : "", flipV ? " V" : "");

			DrawString(s.String(), BPoint(xInfo, rowY));
		}
	}

	SetFont(&prevFont);

	int32 lastSprite = fFirstSprite + kVisibleOAMRows - 1;

	if (lastSprite >= kOAMSpriteCount) {
		lastSprite = kOAMSpriteCount - 1;
	}

	BString footer;
	footer.SetToFormat("Showing OAM sprites %02ld-%02ld of 64.", static_cast<long>(fFirstSprite),
						static_cast<long>(lastSprite));

	SetHighColor(90, 90, 90);
	DrawString(footer.String(), BPoint(panel.left + 10.0f, panel.bottom - 14.0f));
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSelectedSpritePanel
//
// Draws the selected or hovered sprite details panel.  The panel decodes the
// active OAM entry using the PPU state represented by the same debugger
// snapshot, including sprite-size and pattern-table selection.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawSelectedSpritePanel()
{
	BRect panel(4.0f, 536.0f, Bounds().right - 4.0f, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "Selected Sprite");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float leftLabelX = panel.left + 8.0f;
	const float leftValueX = leftLabelX + 78.0f;

	const float rightLabelX = panel.left + 210.0f;
	const float rightValueX = rightLabelX + 72.0f;

	float leftY = panel.top + 36.0f;
	float rightY = panel.top + 36.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	auto drawLeftLV = [&](const char *label, const char *value, bool fixedValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(fixedValue ? &fixed : &prevFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightLV = [&](const char *label, const char *value, bool fixedValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(fixedValue ? &fixed : &prevFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;
	BRect previewRect(panel.right - 58.0f, panel.top + 34.0f, panel.right - 12.0f, panel.bottom - 12.0f);
	
	DrawSpritePreview(previewRect, active);

	if (active < 0) {
		drawLeftLV("Sprite:", "--", true);
		drawLeftLV("Raw Y:", "--", true);
		drawLeftLV("Screen Y:", "--", true);
		drawLeftLV("Visible:", "--", false);
		drawLeftLV("X:", "--", true);
		drawLeftLV("Tile:", "--", true);

		drawRightLV("OAM:", "--", true);
		drawRightLV("Bytes:", "--", true);
		drawRightLV("CHR:", "--", true);
		drawRightLV("Pal/P:", "--", false);
		drawRightLV("Flip:", "--", false);
		drawRightLV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

		SetFont(&prevFont);
		return;
	}

	uint32 base = active * 4;
	uint8 spriteY = OAMByte(base + 0);
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);
	uint8 spriteX = OAMByte(base + 3);
	uint8 pal = attr & 0x3;
	
	bool priority = (attr & 0x20) != 0;
	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;

	const uint8 ctrl = DisplayPPUCTRL();
	bool largeSprites = (ctrl & 0x20) != 0;

	uint32 chrAddr = 0;
	uint32 chrAddrBottom = 0;

	if (largeSprites) {
		uint32 whichPT = tile & 0x1;
		uint32 topTile = tile & 0xfe;

		chrAddr = (whichPT ? 0x1000 : 0x0000) + topTile * 16;
		chrAddrBottom = chrAddr + 16;
	} else {
		uint32 spritePatternBase = (ctrl & 0x08) ? 0x1000 : 0x0000;
		chrAddr = spritePatternBase + tile * 16;
	}

	BString s;

	s.SetToFormat("%02ld", static_cast<long>(active));
	drawLeftLV("Sprite:", s.String(), true);

	s.SetToFormat("$%02X", spriteY);
	drawLeftLV("Raw Y:", s.String(), true);

	const unsigned screenY = static_cast<unsigned>(spriteY) + 1U;
	s.SetToFormat("%u", screenY);
	drawLeftLV("Screen Y:", s.String(), false);
	drawLeftLV("Visible:", (spriteY < 0xef) ? "yes" : "offscreen", false);

	s.SetToFormat("$%02X", spriteX);
	drawLeftLV("X:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%02X/$%02X", tile & 0xfe, (tile & 0xfe) + 1);
		drawLeftLV("Tiles:", s.String(), true);
	} else {
		s.SetToFormat("$%02X", tile);
		drawLeftLV("Tile:", s.String(), true);
	}

	s.SetToFormat("$%02lX-$%02lX", static_cast<unsigned long>(base), static_cast<unsigned long>(base + 3));
	drawRightLV("OAM:", s.String(), true);

	s.SetToFormat("%02X %02X %02X %02X", spriteY, tile, attr, spriteX);
	drawRightLV("Bytes:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%04lX/$%04lX", static_cast<unsigned long>(chrAddr), 
									   static_cast<unsigned long>(chrAddrBottom));
		drawRightLV("CHR:", s.String(), true);
	} else {
		s.SetToFormat("$%04lX", static_cast<unsigned long>(chrAddr));
		drawRightLV("CHR:", s.String(), true);
	}

	s.SetToFormat("%u / %s", static_cast<unsigned>(pal), priority ? "behind" : "front");
	drawRightLV("Pal/P:", s.String(), false);

	if (flipH && flipV) {
		drawRightLV("Flip:", "H + V", false);
	} else if (flipH) {
		drawRightLV("Flip:", "H", false);
	} else if (flipV) {
		drawRightLV("Flip:", "V", false);
	} else {
		drawRightLV("Flip:", "none", false);
	}

	drawRightLV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetFirstSpriteFromScrollBar
//
// Updates the first visible OAM sprite in response to scrollbar movement.  The
// requested value is clamped to the valid first-sprite range and linked debugger
// views are synchronized with the new active row.
//
// Parameters:
//   firstSprite - Requested first visible sprite index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetFirstSpriteFromScrollBar  (int32 firstSprite)
{
	if (firstSprite < 0) {
		firstSprite = 0;
	}

	if (firstSprite > kMaxFirstOAMSprite) {
		firstSprite = kMaxFirstOAMSprite;
	}

	if (fFirstSprite == firstSprite) {
		return;
	}

	fFirstSprite = firstSprite;

	if (!fSpriteLocked) {
		if (fHoverSprite < fFirstSprite || fHoverSprite >= fFirstSprite + kVisibleOAMRows) {
			fHoverSprite = fFirstSprite;
		}
	}

	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();
	UpdateCHRExplorer();

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetHostPalette
//
// Sets the host color-map palette used to convert NES palette indices into
// Haiku rgb_color values.  The embedded CHR explorer is updated with the same
// host palette.
//
// Parameters:
//   palette - NES-color-index to host-color-index lookup table.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetHostPalette (uint8 *palette)
{
	fHostPalette = palette;

	if (fCHRExplorer) {
		fCHRExplorer->SetHostPalette(palette);
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::SpritePreviewColor
//
// Converts a decoded sprite pixel value into a Haiku rgb_color using the
// palette state represented by the current OAM debugger snapshot.
//
// Pixel value 0 uses the universal background color for preview purposes.
//
// Parameters:
//   spritePalette - Sprite palette row, 0-3.
//   pixel         - Decoded 2-bit sprite pixel value, 0-3.
//
// Returns:
//   rgb_color for the requested sprite pixel.
// -----------------------------------------------------------------------------
rgb_color
OAMDebugView::SpritePreviewColor (uint8 spritePalette, uint8 pixel) const
{
	uint32 paletteAddress;

	if (pixel == 0) {
		/*
		 * Sprite pixel 0 is transparent, but for this preview we draw it
		 * using the universal background color so the sprite preview matches
		 * the palette represented by the current debugger snapshot.
		 */
		paletteAddress = 0x3f00;
	} else {
		/*
		 * Sprite palettes live at $3F10-$3F1F.
		 * Entries 1-3 are visible sprite colors.
		 */
		paletteAddress = 0x3f10 + (spritePalette * 4) + pixel;
	}

	uint8 nesColor = DisplayPaletteByte(paletteAddress) & 0x3f;

	if (!fHostPalette || !Window()) {
		return rgb_color{ 0, 0, 0, 255 };
	}

	uint8 hostIndex = fHostPalette[nesColor];
	BScreen screen(Window());

	return screen.ColorForIndex(hostIndex);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSpritePreview
//
// Draws a small preview of the selected sprite using the OAM, PPUCTRL, CHR,
// palette, and flip state represented by the current debugger snapshot.
//
// Hidden/offscreen sprites are reported as OFF SCR instead of being rendered.
//
// Parameters:
//   previewRect - Destination rectangle for the preview box.
//   spriteIndex - OAM sprite index to preview.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawSpritePreview (BRect previewRect, int32 spriteIndex)
{
	/*
	 * Small shadow so the preview box stands off the gray panel.
	 */
	BRect shadowRect = previewRect;
	shadowRect.OffsetBy(2.0f, 2.0f);
	
	SetHighColor(190, 190, 190);
	FillRect(shadowRect);

	/*
	 * Outer preview area: light gray debugger background.
	 */
	SetHighColor(236, 236, 236);
	FillRect(previewRect);

	SetHighColor(135, 135, 135);
	StrokeRect(previewRect);

	/*
	 * Inner highlight edge.
	 */
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
			   BPoint(previewRect.right - 1.0f, previewRect.top + 1.0f));
	StrokeLine(BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
			   BPoint(previewRect.left + 1.0f, previewRect.bottom - 1.0f));

	if (spriteIndex < 0 || spriteIndex >= 64 || !nes::cart.mapper()) {
		SetHighColor(90, 90, 90);
		DrawString("--", BPoint(previewRect.left + 12.0f, previewRect.top + 24.0f));

		return;
	}

	uint32 base = spriteIndex * 4;
	uint8 spriteY = OAMByte(base + 0);
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);
	uint8 palette = attr & 0x03;

	if (spriteY >= 0xef) {
		SetHighColor(90, 90, 90);
		DrawString("OFF", BPoint(previewRect.left + 11.0f, previewRect.top + 27.0f));
		DrawString("SCR", BPoint(previewRect.left + 11.0f, previewRect.top + 42.0f));

		return;
	}

	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	const uint8 ctrl = DisplayPPUCTRL();
	bool largeSprites = (ctrl & 0x20) != 0;
	const int spriteW = 8;
	const int spriteH = largeSprites ? 16 : 8;
	
	const float scale = largeSprites ? 3.0f : 4.0f;
	const float drawW = spriteW * scale;
	const float drawH = spriteH * scale;

	const float startX = previewRect.left + floorf((previewRect.Width() + 1.0f - drawW) / 2.0f);
	const float startY = previewRect.top + floorf((previewRect.Height() + 1.0f - drawH) / 2.0f);

	BRect spriteRect(startX, startY, startX + drawW - 1.0f, startY + drawH - 1.0f);

	/*
	 * Transparent sprite pixels show the universal background color represented
	 * by the current debugger snapshot.
	 */
	SetHighColor(SpritePreviewColor(palette, 0));
	FillRect(spriteRect);

	for (int py = 0; py < spriteH; py++) {
		int srcY = flipV ? (spriteH - 1 - py) : py;

		uint32 chrAddr;

		if (largeSprites) {
			uint32 whichPT = tile & 0x01;
			uint32 topTile = tile & 0xfe;
			chrAddr = (whichPT ? 0x1000 : 0x0000) + (topTile * 16) + ((srcY / 8) * 16) + (srcY & 0x7);
		} else {
			uint32 spritePatternBase = (ctrl & 0x8) ? 0x1000 : 0x0000;
			chrAddr = spritePatternBase + (tile * 16) + srcY;
		}

		uint8 plane0 = DisplayCHRByte(chrAddr);
		uint8 plane1 = DisplayCHRByte(chrAddr + 8);

		for (int32 px = 0; px < spriteW; px++) {
			int32 srcX = flipH ? px : (7 - px);
			uint8 pixel = ((plane0 >> srcX) & 0x1) | (((plane1 >> srcX) & 0x1) << 1);

			if (pixel == 0) {
				continue;
			}

			BRect r(startX + (px * scale), startY + (py * scale), 
					startX + ((px + 1) * scale) - 1.0f, startY + ((py + 1) * scale) - 1.0f);

			SetHighColor(SpritePreviewColor(palette, pixel));
			FillRect(r);
		}
	}

	/*
	 * Inner sprite canvas outline.
	 */
	SetHighColor(40, 40, 40);
	StrokeRect(spriteRect);

	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
			   BPoint(spriteRect.right - 1.0f, spriteRect.top + 1.0f));
	StrokeLine(BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
			   BPoint(spriteRect.left + 1.0f, spriteRect.bottom - 1.0f));

	/*
	 * Outer preview border, redrawn after all contents.
	 */
	SetHighColor(135, 135, 135);
	StrokeRect(previewRect);
}


// -----------------------------------------------------------------------------
// OAMDebugView::UpdatePaletteDebuggerHighlight
//
// Synchronizes the palette debugger with the active OAM sprite.  The sprite
// palette row encoded in the OAM attribute byte is highlighted.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::UpdatePaletteDebuggerHighlight()
{
	if (!fParent) {
		return;
	}

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= 64) {
		fParent->ClearPaletteDebuggerHighlight();
		return;
	}

	uint32 base = (active * 4);
	uint8 attr = OAMByte(base + 2);
	uint8 spritePalette = (attr & 0x3);

	// true = sprite palette area, palette = 0..3, entry -1 = whole row.
	fParent->HighlightPaletteDebugger(true, spritePalette, -1);
}


// -----------------------------------------------------------------------------
// OAMDebugView::UpdateCHRExplorer
//
// Synchronizes the CHR Explorer with the active OAM sprite.
//
// The function resolves sprite pattern-table selection, CHR address, sprite
// palette, 8x8/8x16 mode, and horizontal/vertical flip state from the current
// coherent OAM debugger snapshot.
//
// Hidden/offscreen OAM entries are still sent to the CHR Explorer because their
// tile, palette, and transform data remain valid and useful for debugging.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::UpdateCHRExplorer()
{
	if (!fCHRExplorer) {
		return;
	}

	if (!nes::cart.mapper()) {
		fCHRExplorer->Clear();
		return;
	}

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= kOAMSpriteCount) {
		fCHRExplorer->Clear();
		return;
	}

	uint32 base = active * 4;
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);
	uint8 spritePalette = attr & 0x3;
	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	const uint8 ctrl = DisplayPPUCTRL();
	bool largeSprites = (ctrl & 0x20) != 0;

	if (largeSprites) {
		int32 whichPT = tile & 0x01;
		int32 topTile = tile & 0xfe;

		uint32 chrAddrTop = (whichPT ? 0x1000 : 0x0000) + (topTile * 16);
		uint32 chrAddrBottom = chrAddrTop + 16;

		uint8 chrTop[16];
		uint8 chrBottom[16];

		for (int32 i = 0; i < 16; i++) {
			chrTop[i] = DisplayCHRByte(chrAddrTop + i);
			chrBottom[i] = DisplayCHRByte(chrAddrBottom + i);
		}

		fCHRExplorer->SetTile8x16(whichPT, topTile, fSpriteLocked, chrAddrTop, chrTop,
								  chrAddrBottom, chrBottom, spritePalette);

		fCHRExplorer->SetUseSpritePalette(true);
		fCHRExplorer->SetSelectedPalette(spritePalette);
		fCHRExplorer->SetTileTransform(flipH, flipV);

		return;
	}

	int32 whichPT = (ctrl & 0x08) ? 1 : 0;
	uint32 chrAddr = (whichPT ? 0x1000 : 0x0000) + (tile * 16);

	uint8 chrBytes[16];

	for (int32 i = 0; i < 16; i++) {
		chrBytes[i] = DisplayCHRByte(chrAddr + i);
	}

	fCHRExplorer->SetTile8x8(whichPT, tile, fSpriteLocked, chrAddr, chrBytes, 
							 spritePalette, -1, 0, 0, 0, 0);

	fCHRExplorer->SetUseSpritePalette(true);
	fCHRExplorer->SetSelectedPalette(spritePalette);
	fCHRExplorer->SetTileTransform(flipH, flipV);
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetPatternTables
//
// Stores references to the pattern table debugger windows so the OAM viewer can
// highlight the CHR tile used by the active sprite.
//
// Parameters:
//   pt1 - Pattern table window for pattern table 1 / CHR $0000.
//   pt2 - Pattern table window for pattern table 2 / CHR $1000.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetPatternTables (PatternTableWindow *pt1, PatternTableWindow *pt2)
{
	fPatternTable1Window = pt1;
	fPatternTable2Window = pt2;

	UpdatePatternTableHighlight();
}


// -----------------------------------------------------------------------------
// OAMDebugView::ClearPatternTableHighlight
//
// Clears OAM-owned external highlights from both pattern table debugger
// windows.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::ClearPatternTableHighlight()
{
	ClearPatternWindowHighlight(fPatternTable1Window);
	ClearPatternWindowHighlight(fPatternTable2Window);
}


// -----------------------------------------------------------------------------
// OAMDebugView::UpdatePatternTableHighlight
//
// Updates the Pattern Table debugger highlight to match the currently active
// OAM sprite.
//
// The active sprite is taken from the locked selection when one exists;
// otherwise the current hover sprite is used.
//
// In 8x8 sprite mode, PPUCTRL bit 3 selects the sprite pattern table and the OAM
// tile byte directly selects the CHR tile.
//
// In 8x16 sprite mode, bit 0 of the OAM tile byte selects the pattern table and
// bits 1-7 select the even-numbered top tile. The following odd-numbered tile
// forms the bottom half of the sprite, so the Pattern Table view is asked to
// highlight the complete pair.
//
// The OAM lock state is propagated to the Pattern Table and CHR Explorer so the
// linked debugger views can report HOVER or LOCKED consistently.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::UpdatePatternTableHighlight()
{
	const int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= kOAMSpriteCount) {
		ClearPatternTableHighlight();
		return;
	}

	const uint32 base = static_cast<uint32>(active) * 4;
	const uint8 tile = OAMByte(base + 1);
	const uint8 ctrl = DisplayPPUCTRL();
	const bool largeSprites = (ctrl & 0x20) != 0;
	int32 whichPT = 0;
	int32 tileIndex = 0;

	if (largeSprites) {
		/*
		 * In 8x16 sprite mode:
		 *
		 *   tile bit 0  -> pattern table
		 *   tile bits 1-7 -> even-numbered top tile
		 *
		 * The following odd-numbered tile forms the bottom half.
		 */
		whichPT = tile & 0x01;
		tileIndex = tile & 0xfe;
	} else {
		/*
		 * In 8x8 sprite mode, PPUCTRL bit 3 selects the sprite
		 * pattern table and the OAM tile byte is the tile index.
		 */
		whichPT = (ctrl & 0x08) ? 1 : 0;
		tileIndex = tile;
	}

	if (whichPT == 0) {
		SetPatternWindowHighlight(fPatternTable1Window, whichPT, tileIndex,
									largeSprites, fSpriteLocked);
		ClearPatternWindowHighlight(fPatternTable2Window);
	} else {
		ClearPatternWindowHighlight(fPatternTable1Window);
		SetPatternWindowHighlight(fPatternTable2Window, whichPT, tileIndex,
									largeSprites, fSpriteLocked);
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetExplorer
//
// Connects the CHR Explorer used by the OAM debugger and immediately
// synchronizes it with the current OAM state.
//
// The host palette is forwarded first.  If a ROM is loaded but no OAM snapshot
// has yet been established, an initial snapshot is captured so the explorer and
// OAM debugger use the same coherent sprite data.
//
// Parameters:
//   explorer - CHRExplorerView used to inspect the active sprite tile.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetExplorer (CHRExplorerView *explorer)
{
	fCHRExplorer = explorer;

	if (!fCHRExplorer) {
		return;
	}

	if (fHostPalette) {
		fCHRExplorer->SetHostPalette(fHostPalette);
	}

	if (!HasROMLoaded()) {
		fCHRExplorer->Clear();
		return;
	}

	if (!fHaveFrozenOAM) {
		CaptureOAMSnapshot();
	}

	UpdateCHRExplorer();
}


// -----------------------------------------------------------------------------
// OAMDebugView::Clear
//
// Clears all ROM-specific OAM debugger state while preserving user-selected
// viewer state.
//
// The current OAM/PPU snapshot, hover/lock selection, and linked debugger
// highlights are discarded so data from an unloaded ROM cannot remain visible
// after another ROM is loaded.
//
// The first visible sprite and live/frozen mode are intentionally preserved.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::Clear()
{
	fHaveFrozenOAM = false;

	memset(fFrozenOAM, 0, sizeof(fFrozenOAM));

	fFrozenPPUCTRL = 0x0;

	memset(fFrozenCHR, 0, sizeof(fFrozenCHR));
	memset(fFrozenPalette, 0, sizeof(fFrozenPalette));

	fSpriteLocked = false;
	fLockedSprite = -1;
	fHoverSprite = -1;

	fMouseInside = false;

	if (fParent) {
		fParent->ClearPaletteDebuggerHighlight();
	}

	ClearPatternTableHighlight();

	if (fCHRExplorer) {
		fCHRExplorer->Clear();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::CaptureOAMSnapshot
//
// Captures one coherent snapshot of all PPU state required by the OAM debugger.
//
// In addition to the 256-byte OAM table, the snapshot records PPUCTRL, the
// complete 8 KB CHR pattern area, and the 32-byte NES palette-RAM area.  This
// ensures that frozen OAM entries retain the same sprite-size interpretation,
// pattern-table selection, tile graphics, and palette colors that existed when
// the OAM bytes were captured.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::CaptureOAMSnapshot()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		fHaveFrozenOAM = false;
		return;
	}

	/*
	 * Capture interpretation state first.
	 */
	fFrozenPPUCTRL = nes::ppu::ppuctrl();

	/*
	 * Capture all 64 OAM entries / 256 bytes.
	 */
	for (uint32 i = 0; i < 0x100; i++) {
		fFrozenOAM[i] = nes::ppu::oam_ram(i);
	}

	/*
	 * Capture the complete CHR pattern area used by sprites.
	 */
	for (uint32 address = 0; address < 0x2000; address++) {
		fFrozenCHR[address] = mapper->read_vram(address);
	}

	/*
	 * Capture the complete NES palette-RAM area.
	 */
	for (uint32 offset = 0; offset < 0x20; offset++) {
		fFrozenPalette[offset] = nes::ppu::palette_ram(0x3f00 + offset);
	}

	fHaveFrozenOAM = true;
}


// -----------------------------------------------------------------------------
// OAMDebugView::OAMByte
//
// Reads an OAM byte for display/debugger use.  Once a snapshot exists, all OAM
// display paths read from the snapshot buffer so each refresh is internally
// consistent.
//
// Parameters:
//   address - OAM byte address.  The address is wrapped to 0-255.
//
// Returns:
//   OAM byte from the current display snapshot, or live OAM before the first
//   snapshot has been captured.
// -----------------------------------------------------------------------------
uint8
OAMDebugView::OAMByte (uint32 address) const
{
	if (fHaveFrozenOAM) {
		return fFrozenOAM[address & 0xff];
	}

	return nes::ppu::oam_ram(address);
}

// -----------------------------------------------------------------------------
// OAMDebugView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM/mapper is currently loaded.
// -----------------------------------------------------------------------------
bool
OAMDebugView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the OAM Viewer is opened without a
// loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect OAM sprites.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80);
	DrawString(title, BPoint(centerX - (StringWidth(title) * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120);
	DrawString(detail, BPoint(centerX - (StringWidth(detail) * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DisplayPPUCTRL
//
// Returns the PPUCTRL value represented by the current OAM debugger snapshot.
//
// Once an OAM snapshot exists, sprite size and pattern-table interpretation
// must use the PPUCTRL value captured with that snapshot.  Before the first
// snapshot is available, the current live PPUCTRL value is returned.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot or live PPUCTRL value.
// -----------------------------------------------------------------------------
uint8
OAMDebugView::DisplayPPUCTRL() const
{
	if (fHaveFrozenOAM) {
		return fFrozenPPUCTRL;
	}

	return nes::ppu::ppuctrl();
}


// -----------------------------------------------------------------------------
// OAMDebugView::DisplayCHRByte
//
// Reads one CHR byte represented by the current OAM debugger snapshot.
//
// Once a snapshot exists, sprite previews and linked debugger views use the
// captured CHR state rather than current mapper state.  Before the first
// snapshot exists, live CHR is used as a fallback.
//
// Parameters:
//   address - CHR address to read.
//
// Returns:
//   Snapshot or live CHR byte.
// -----------------------------------------------------------------------------
uint8
OAMDebugView::DisplayCHRByte (uint32 address) const
{
	address &= 0x1fff;

	if (fHaveFrozenOAM) {
		return fFrozenCHR[address];
	}

	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		return 0;
	}

	return mapper->read_vram(address);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DisplayPaletteByte
//
// Reads one palette-RAM byte represented by the current OAM debugger snapshot.
//
// The address is normalized into the NES $3F00-$3F1F palette range, including
// the special $3F10/$3F14/$3F18/$3F1C aliases.
//
// Parameters:
//   address - PPU palette address.
//
// Returns:
//   Snapshot or live NES palette-RAM byte.
// -----------------------------------------------------------------------------
uint8
OAMDebugView::DisplayPaletteByte(uint32 address) const
{
	uint32 offset = (address - 0x3f00) & 0x1f;

	/*
	 * $3F10/$3F14/$3F18/$3F1C mirror
	 * $3F00/$3F04/$3F08/$3F0C.
	 */
	if ((offset & 0x13) == 0x10) {
		offset &= 0x0f;
	}

	if (fHaveFrozenOAM) {
		return fFrozenPalette[offset];
	}

	return nes::ppu::palette_ram(0x3f00 + offset);
}



