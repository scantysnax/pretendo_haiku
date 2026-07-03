
#include "DebugHelpers.h"
#include "OAMDebugView.h"
#include "PatternTableView.h"
#include "PatternTableWindow.h"
#include "PretendoWindow.h"

#include "CHRExplorerView.h"
#include "Cart.h"
#include "Mapper.h"
#include "Ppu.h"


// -----------------------------------------------------------------------------
// OAMSpriteScrollBar
//
// Private helper scroll bar used by OAMDebugView.  It snaps scrollbar movement
// to 8-sprite pages so the visible OAM rows remain aligned to sprite groups
// 00-07, 08-15, and so on.
// -----------------------------------------------------------------------------
class OAMSpriteScrollBar : public BScrollBar
{
	public:
	// -------------------------------------------------------------------------
	// OAMSpriteScrollBar::OAMSpriteScrollBar
	//
	// Creates the sprite-page scrollbar used by the OAM viewer.
	//
	// Parameters:
	//   frame - Scrollbar frame in the OAMDebugView coordinate space.
	//   owner - OAMDebugView that receives snapped page changes.
	//
	// Returns:
	//   Constructor; no return value.
	// -------------------------------------------------------------------------
	OAMSpriteScrollBar (BRect frame, OAMDebugView *owner)
		: BScrollBar(frame, "oam_sprite_scrollbar", nullptr, 0.0f, 56.0f, B_VERTICAL),
			fOwner(owner)
	{
		SetSteps(8.0f, 8.0f);
		SetProportion(8.0f / 64.0f);
	}

	// -------------------------------------------------------------------------
	// OAMSpriteScrollBar::ValueChanged
	//
	// Handles scrollbar movement and forwards the selected OAM page to the
	// owning OAMDebugView.  The value is snapped to 8-sprite boundaries so the
	// list always shows complete OAM pages.
	//
	// Parameters:
	//   value - New scrollbar value requested by the user.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	virtual void ValueChanged (float value)
	{
		BScrollBar::ValueChanged(value);

		if (!fOwner) {
			return;
		}

		int32 firstSprite = static_cast<int32>(value);

		// Snap to 8-sprite pages so the rows stay stable.
		firstSprite = (firstSprite / 8) * 8;

		if (firstSprite < 0) {
			firstSprite = 0;
		}
		
		if (firstSprite > 56) {
			firstSprite = 56;
		}

		fOwner->SetFirstSpriteFromScrollBar(firstSprite);
	}

	private:
	OAMDebugView *fOwner;
};


// -----------------------------------------------------------------------------
// SetPatternWindowHighlight
//
// Safely applies an external tile highlight to a PatternTableWindow.  The
// window is locked before accessing its view because pattern table windows are
// separate BWindow instances.
//
// Parameters:
//   window    - Pattern table window to update.
//   whichPT   - Pattern table index, 0 for $0000 or 1 for $1000.
//   tileIndex - Tile index to highlight.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static inline void
SetPatternWindowHighlight(PatternTableWindow *window, int32 whichPT, int32 tileIndex)
{
	if (!window) {
		return;
	}

	if (window->Lock()) {
		if (window->View())
			window->View()->SetExternalHighlight(whichPT, tileIndex);

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
// enables keyboard focus, mouse-wheel tracking, creates the OAM page scrollbar,
// captures the initial OAM snapshot, and starts the viewer in stable snapshot
// mode.
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

	SetMouseEventMask(
		B_POINTER_EVENTS | B_MOUSE_WHEEL_CHANGED,
		B_NO_POINTER_HISTORY
	);

	if (!fSpriteScrollBar) {
		BRect listPanel(
			4.0f,
			174.0f,
			Bounds().right - 4.0f,
			432.0f
		);

		BRect scrollFrame(
			listPanel.right - 18.0f,
			listPanel.top + 26.0f,
			listPanel.right - 4.0f,
			listPanel.bottom - 22.0f
		);

		fSpriteScrollBar = new OAMSpriteScrollBar(scrollFrame, this);
		AddChild(fSpriteScrollBar);
		fSpriteScrollBar->SetValue(fFirstSprite);
	}
	
	CaptureOAMSnapshot();
	fFreezeUpdates = true;

	UpdatePaletteDebuggerHighlight();
	UpdatePatternTableHighlight();
	UpdateCHRExplorer();

	Invalidate();
}


// -----------------------------------------------------------------------------
// OAMDebugView::Draw
//
// Draws the complete OAM debugger view: controls, summary, sprite list, and
// selected sprite details.
//
// Parameters:
//   updateRect - Invalidated rectangle supplied by the app_server.
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

	DrawHeaderUI();
	DrawOAMSummaryPanel();
	DrawSpriteListPanel();
	DrawSelectedSpritePanel();
}


// -----------------------------------------------------------------------------
// OAMDebugView::FrameResized
//
// Repositions and resizes the sprite-page scrollbar when the view frame
// changes.
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

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	BRect scrollFrame(
		listPanel.right - 18.0f,
		listPanel.top + 26.0f,
		listPanel.right - 4.0f,
		listPanel.bottom - 22.0f
	);

	fSpriteScrollBar->MoveTo(scrollFrame.LeftTop());
	fSpriteScrollBar->ResizeTo(scrollFrame.Width(), scrollFrame.Height());
}


// -----------------------------------------------------------------------------
// OAMDebugView::KeyDown
//
// Handles OAM viewer keyboard shortcuts.  Space toggles live/snapshot mode, R
// refreshes the current snapshot, arrow keys move the active sprite, and [/] or
// ,/. page through the 64 OAM entries in groups of eight.
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

	auto syncAfterSelectionChange = [&]() {
		if (fFirstSprite < 0) {
			fFirstSprite = 0;
		}

		if (fFirstSprite > 56) {
			fFirstSprite = 56;
		}

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

		if (active < 0 || active >= 64) {
			active = fFirstSprite;
		}

		active += delta;

		if (active < 0) {
			active = 0;
		}

		if (active > 63) {
			active = 63;
		}

		if (fSpriteLocked) {
			fLockedSprite = active;
		}

		fHoverSprite = active;

		if (active < fFirstSprite) {
			fFirstSprite = (active / 8) * 8;
		} else if (active >= fFirstSprite + 8) {
			fFirstSprite = (active / 8) * 8;
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
			fFirstSprite -= 8;

			if (fFirstSprite < 0) {
				fFirstSprite = 0;
			}

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite
					|| fHoverSprite >= fFirstSprite + 8) {
					fHoverSprite = fFirstSprite;
				}
			}

			syncAfterSelectionChange();
			break;

		case ']':
		case '.':
			fFirstSprite += 8;

			if (fFirstSprite > 56) {
				fFirstSprite = 56;
			}

			if (!fSpriteLocked) {
				if (fHoverSprite < fFirstSprite
					|| fHoverSprite >= fFirstSprite + 8) {
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
// Handles messages delivered to the view.  The OAM viewer currently handles
// mouse-wheel messages to page through the sprite list and forwards all other
// messages to BView.
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
			float deltaY = 0.0f;

			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK) {
				BView::MessageReceived(message);
				return;
			}

			int32 oldFirstSprite = fFirstSprite;

			if (deltaY > 0.0f) {
				fFirstSprite += 8;
			} else if (deltaY < 0.0f) {
				fFirstSprite -= 8;
			}

			if (fFirstSprite < 0) {
				fFirstSprite = 0;
			}

			if (fFirstSprite > 56) {
				fFirstSprite = 56;
			}

			if (fFirstSprite != oldFirstSprite) {
				if (fSpriteScrollBar)
					fSpriteScrollBar->SetValue(fFirstSprite);

				if (!fSpriteLocked) {
					if (fHoverSprite < fFirstSprite
						|| fHoverSprite >= fFirstSprite + 8) {
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
// Handles mouse clicks in the sprite list.  Clicking a sprite locks it for
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
OAMDebugView::MouseDown (BPoint where)
{
	MakeFocus(true);

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	if (!listPanel.Contains(where))
		return;

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8) {
		return;
	}

	int32 spriteIndex = (fFirstSprite + row);

	if (spriteIndex < 0 || spriteIndex >= 64) {
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
// Handles hover inspection in the sprite list.  When no sprite is locked, moving
// over a row updates the active sprite and synchronizes the linked palette,
// pattern table, and CHR explorer views.
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
OAMDebugView::MouseMoved (BPoint where, uint32 transit, const BMessage* message)
{
	(void)message;

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

	BRect listPanel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

	if (!listPanel.Contains(where)) {
		return;
	}

	const float firstRowY = listPanel.top + 58.0f;
	const float rowH = 17.0f;

	int32 row = static_cast<int32>((where.y - firstRowY) / rowH);

	if (row < 0 || row >= 8) {
		return;
	}

	int32 spriteIndex = (fFirstSprite + row);

	if (spriteIndex < 0 || spriteIndex >= 64) {
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
// Periodic update hook for live OAM mode.  In live mode, this captures one
// coherent OAM snapshot, updates linked debugger views, and redraws the OAM
// view.  In snapshot/frozen mode, no automatic refresh is performed.
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
	if (fFreezeUpdates) {
		return;
	}

	// Take one coherent OAM snapshot for this debugger refresh.
	// All drawing and linked inspectors should read from this snapshot,
	// not directly from live OAM.
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
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		76.0f
	);

	::DrawDebugPanel(this, panel, "Controls:");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 58.0f;

	float y = panel.top + 34.0f;

	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawKV("Mouse:", "hover inspect / click lock");

	drawKV("Space:", fFreezeUpdates
		? "live OAM"
		: "snapshot OAM");

	drawKV("[ ] R:", "prev/next page / refresh snapshot");
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawOAMSummaryPanel
//
// Draws the OAM summary panel.  The summary counts visible and hidden sprites,
// shows whether the viewer is in live or frozen snapshot mode, and reports the
// active PPU sprite size mode.
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
	BRect panel(
		4.0f,
		88.0f,
		Bounds().right - 4.0f,
		164.0f
	);

	::DrawDebugPanel(this, panel, "OAM Summary:");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

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

	auto drawLeftKV = [&](const char *label, const char *value, bool monoValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(monoValue ? &mono : &prevFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value, bool monoValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(monoValue ? &mono : &prevFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	s.SetToFormat("%ld / 64", (long)usedSprites);
	drawLeftKV("OAM Used:", s.String(), false);

	s.SetToFormat("%ld", (long)hiddenSprites);
	drawLeftKV("Hidden:", s.String(), false);

	drawRightKV("State:", fFreezeUpdates ? "FROZEN" : "LIVE", false);

	drawRightKV("Mode:", (nes::ppu::ppuctrl() & 0x20)
		? "8x16 sprites"
		: "8x8 sprites", false);

	s.SetToFormat("$%02X", nes::ppu::ppuctrl());
	drawRightKV("PPUCTRL:", s.String(), true);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSpriteListPanel
//
// Draws the paged OAM sprite list.  Rows show raw OAM fields, decoded summary
// flags, hidden/offscreen state, sprite-zero marking, current selection, and
// same-tile highlighting for the active sprite.
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
	BRect panel(
		4.0f,
		174.0f,
		Bounds().right - 4.0f,
		432.0f
	);

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
	StrokeLine(
		BPoint(panel.left + 8.0f, y - 13.0f),
		BPoint(panel.right - 24.0f, y - 13.0f)
	);

	const float firstRowY = y;
	const float rowH = 17.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	bool haveActiveTile = false;
	uint8 activeTile = 0;

	if (active >= 0 && active < 64) {
		uint32 activeBase = active * 4;
		uint8 activeY = OAMByte(activeBase + 0);

		if (activeY < 0xef) {
			activeTile = OAMByte(activeBase + 1);
			haveActiveTile = true;
		}
	}

	for (int32 row = 0; row < 8; row++) {
		int32 spriteIndex = fFirstSprite + row;

		if (spriteIndex < 0 || spriteIndex >= 64)
			continue;

		uint32 base = (spriteIndex * 4);

		uint8 spriteY = OAMByte(base + 0);
		uint8 tile = OAMByte(base + 1);
		uint8 attr = OAMByte(base + 2);
		uint8 spriteX = OAMByte(base + 3);

		bool spriteZero = (spriteIndex == 0);
		bool hidden = (spriteY >= 0xef);

		uint8 pal = (attr & 0x3);
		bool priority = (attr & 0x20) != 0;
		bool flipH = (attr & 0x40) != 0;
		bool flipV = (attr & 0x80) != 0;

		bool sameTile = haveActiveTile
			&& (spriteIndex != active)
			&& !hidden
			&& (tile == activeTile);

		float rowY = firstRowY + (row * rowH);

		BRect rowRect(
			panel.left + 7.0f,
			rowY - 12.0f,
			panel.right - 24.0f,
			rowY + 4.0f
		);

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

		SetFont(&mono);
		
		BString s;

		s.SetToFormat("%02ld", (long)spriteIndex);
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

			s.SetToFormat("%s%sP%u %s%s%s",
				spriteZero ? "Sprite 0 " : "",
				sameTile ? "same " : "",
				(unsigned)pal,
				priority ? "B" : "F",
				flipH ? " H" : "",
				flipV ? " V" : "");

			DrawString(s.String(), BPoint(xInfo, rowY));
		}
	}

	SetFont(&prevFont);

	BString footer;
	footer.SetToFormat("Showing OAM sprites %02ld-%02ld of 64.",
		(long)fFirstSprite,
		(long)(fFirstSprite + 7)
	);

	SetHighColor(90, 90, 90);
	DrawString(
		footer.String(),
		BPoint(panel.left + 10.0f, panel.bottom - 14.0f)
	);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSelectedSpritePanel
//
// Draws the selected or hovered sprite details panel.  The panel decodes the
// active OAM entry into position, tile, CHR address, palette, priority, flip
// flags, raw OAM byte range, and a small sprite preview.
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
	BRect panel(
		4.0f,
		442.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

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

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	auto drawLeftKV = [&](const char *label, const char *value, bool monoValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(leftLabelX, leftY));

		SetHighColor(0, 0, 0);
		SetFont(monoValue ? &mono : &prevFont);
		DrawString(value, BPoint(leftValueX, leftY));

		leftY += lineH;
	};

	auto drawRightKV = [&](const char *label, const char *value, bool monoValue) {
		SetHighColor(80, 80, 80);
		SetFont(&prevFont);
		DrawString(label, BPoint(rightLabelX, rightY));

		SetHighColor(0, 0, 0);
		SetFont(monoValue ? &mono : &prevFont);
		DrawString(value, BPoint(rightValueX, rightY));

		rightY += lineH;
	};

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	BRect previewRect(
		panel.right - 58.0f,
		panel.top + 34.0f,
		panel.right - 12.0f,
		panel.bottom - 12.0f
	);

	DrawSpritePreview(previewRect, active);

	if (active < 0) {
		drawLeftKV("Sprite:", "--", true);
		drawLeftKV("Raw Y:", "--", true);
		drawLeftKV("Screen Y:", "--", true);
		drawLeftKV("Visible:", "--", false);
		drawLeftKV("X:", "--", true);
		drawLeftKV("Tile:", "--", true);

		drawRightKV("OAM:", "--", true);
		drawRightKV("Bytes:", "--", true);
		drawRightKV("CHR:", "--", true);
		drawRightKV("Pal/P:", "--", false);
		drawRightKV("Flip:", "--", false);
		drawRightKV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

		SetFont(&prevFont);
		return;
	}

	uint32 base = (active * 4);

	uint8 spriteY = OAMByte(base + 0);
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);
	uint8 spriteX = OAMByte(base + 3);

	uint8 pal = attr & 0x3;
	bool priority = (attr & 0x20) != 0;
	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	uint32 chrAddr = 0;
	uint32 chrAddrBottom = 0;

	if (largeSprites) {
		uint32 whichPT = tile & 0x1;
		uint32 topTile = tile & 0xfe;

		chrAddr = (whichPT ? 0x1000 : 0x0000) + (topTile * 16);
		chrAddrBottom = (chrAddr + 16);
	} else {
		uint32 spritePatternBase = (nes::ppu::ppuctrl() & 0x8)
			? 0x1000
			: 0x0000;

		chrAddr = spritePatternBase + (tile * 16);
	}
	
	BString s;

	s.SetToFormat("%02ld", (long)active);
	drawLeftKV("Sprite:", s.String(), true);

	s.SetToFormat("$%02X", spriteY);
	drawLeftKV("Raw Y:", s.String(), true);

	s.SetToFormat("%u", (unsigned)((uint16)spriteY + 1));
	drawLeftKV("Screen Y:", s.String(), true);

	drawLeftKV("Visible:", spriteY < 0xef ? "yes" : "offscreen", false);

	s.SetToFormat("$%02X", spriteX);
	drawLeftKV("X:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%02X/$%02X",
			tile & 0xfe,
			(tile & 0xfe) + 1);
		drawLeftKV("Tiles:", s.String(), true);
	} else {
		s.SetToFormat("$%02X", tile);
		drawLeftKV("Tile:", s.String(), true);
	}

	s.SetToFormat("$%02lX-$%02lX",
		(unsigned long)base,
		(unsigned long)(base + 3));
	drawRightKV("OAM:", s.String(), true);

	s.SetToFormat("%02X %02X %02X %02X",
		spriteY,
		tile,
		attr,
		spriteX);
	drawRightKV("Bytes:", s.String(), true);

	if (largeSprites) {
		s.SetToFormat("$%04lX/$%04lX",
			(unsigned long)chrAddr,
			(unsigned long)chrAddrBottom);
		drawRightKV("CHR:", s.String(), true);
	} else {
		s.SetToFormat("$%04lX", (unsigned long)chrAddr);
		drawRightKV("CHR:", s.String(), true);
	}

	s.SetToFormat("%u / %s",
		(unsigned)pal,
		priority ? "behind" : "front");
	drawRightKV("Pal/P:", s.String(), false);

	if (flipH && flipV) {
		drawRightKV("Flip:", "H + V", false);
	} else if (flipH) {
		drawRightKV("Flip:", "H", false);
	} else if (flipV) {
		drawRightKV("Flip:", "V", false);
	} else {
		drawRightKV("Flip:", "none", false);
	}

	drawRightKV("State:", fSpriteLocked ? "LOCKED" : "HOVER", false);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetFirstSpriteFromScrollBar
//
// Updates the first visible OAM sprite in response to scrollbar movement.  The
// requested value is snapped to an 8-sprite page and linked debugger views are
// synchronized with the new active row.
//
// Parameters:
//   firstSprite - Requested first visible sprite index.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetFirstSpriteFromScrollBar(int32 firstSprite)
{
	firstSprite = (firstSprite / 8) * 8;

	if (firstSprite < 0) {
		firstSprite = 0;
	}
	
	if (firstSprite > 56) {
		firstSprite = 56;
	}

	if (fFirstSprite == firstSprite) {
		return;
	}

	fFirstSprite = firstSprite;

	if (!fSpriteLocked) {
		if (fHoverSprite < fFirstSprite || fHoverSprite >= fFirstSprite + 8)
			fHoverSprite = fFirstSprite;
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
OAMDebugView::SetHostPalette(uint8 *palette)
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
// Converts a decoded sprite pixel value into a Haiku rgb_color using live NES
// sprite palette RAM and the host palette lookup table.  Pixel value 0 uses the
// universal background color for preview purposes.
//
// Parameters:
//   spritePalette - Sprite palette row, 0-3.
//   pixel         - Decoded 2-bit sprite pixel value, 0-3.
//
// Returns:
//   rgb_color for the requested sprite pixel.
// -----------------------------------------------------------------------------
rgb_color
OAMDebugView::SpritePreviewColor(uint8 spritePalette, uint8 pixel) const
{
	uint32 paletteAddress;

	if (pixel == 0) {
		// Sprite pixel 0 is transparent, but for this preview we draw it
		// using the universal background color so the sprite preview matches
		// the current PPU palette.
		paletteAddress = 0x3f00;
	} else {
		// Sprite palettes live at $3f10-$3f1f.
		// Entries 1-3 are visible sprite colors.
		paletteAddress = 0x3f10 + (spritePalette * 4) + pixel;
	}

	uint8 nesColor = nes::ppu::palette_ram(paletteAddress) & 0x3f;

	if (!fHostPalette || !Window()) {
		return rgb_color{0, 0, 0, 255};
	}

	uint8 hostIndex = fHostPalette[nesColor];

	BScreen screen(Window());
	return screen.ColorForIndex(hostIndex);
}


// -----------------------------------------------------------------------------
// OAMDebugView::DrawSpritePreview
//
// Draws a small preview of the selected sprite using the current OAM snapshot,
// CHR data, sprite palette, and OAM flip flags.  Hidden/offscreen sprites are
// reported as OFF SCR instead of being rendered.
//
// Parameters:
//   previewRect - Destination rectangle for the preview box.
//   spriteIndex - OAM sprite index to preview.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::DrawSpritePreview(BRect previewRect, int32 spriteIndex)
{
	// Small shadow so the preview box stands off the gray panel.
	BRect shadowRect = previewRect;
	shadowRect.OffsetBy(2.0f, 2.0f);

	SetHighColor(190, 190, 190);
	FillRect(shadowRect);

	// Outer preview area: light gray debugger background.
	SetHighColor(236, 236, 236);
	FillRect(previewRect);

	SetHighColor(135, 135, 135);
	StrokeRect(previewRect);

	// Inner highlight edge.
	SetHighColor(255, 255, 255);
	StrokeLine(
		BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
		BPoint(previewRect.right - 1.0f, previewRect.top + 1.0f)
	);
	StrokeLine(
		BPoint(previewRect.left + 1.0f, previewRect.top + 1.0f),
		BPoint(previewRect.left + 1.0f, previewRect.bottom - 1.0f)
	);

	if (spriteIndex < 0 || spriteIndex >= 64 || !nes::cart.mapper()) {
		SetHighColor(90, 90, 90);
		DrawString("--", BPoint(previewRect.left + 12.0f,
			previewRect.top + 24.0f));
		return;
	}

	uint32 base = spriteIndex * 4;

	uint8 spriteY = OAMByte(base + 0);
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);

	uint8 palette = (attr & 0x3);

	if (spriteY >= 0xef) {
		SetHighColor(90, 90, 90);
		DrawString("OFF", BPoint(previewRect.left + 11.0f,
			previewRect.top + 27.0f));
		DrawString("SCR", BPoint(previewRect.left + 11.0f,
			previewRect.top + 42.0f));
		return;
	}

	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	const int spriteW = 8;
	const int spriteH = largeSprites ? 16 : 8;

	const float scale = largeSprites ? 3.0f : 4.0f;

	const float drawW = spriteW * scale;
	const float drawH = spriteH * scale;

	const float startX = previewRect.left
		+ floorf((previewRect.Width() + 1.0f - drawW) / 2.0f);
	const float startY = previewRect.top
		+ floorf((previewRect.Height() + 1.0f - drawH) / 2.0f);

	BRect spriteRect(
		startX,
		startY,
		startX + drawW - 1.0f,
		startY + drawH - 1.0f
	);

	// Inner sprite area: live universal background color from $3F00.
	// Transparent sprite pixels show through this color.
	SetHighColor(SpritePreviewColor(palette, 0));
	FillRect(spriteRect);

	for (int py = 0; py < spriteH; py++) {
		int srcY = flipV ? (spriteH - 1 - py) : py;

		uint32 chrAddr;

		if (largeSprites) {
			uint32 whichPT = (tile & 0x1);
			uint32 topTile = (tile & 0xfe);

			chrAddr = (whichPT ? 0x1000 : 0x0000)
				+ (topTile * 16)
				+ ((srcY / 8) * 16)
				+ (srcY & 0x7);
		} else {
			uint32 spritePatternBase = (nes::ppu::ppuctrl() & 0x8)
				? 0x1000
				: 0x0000;

			chrAddr = spritePatternBase + (tile * 16) + srcY;
		}

		uint8 plane0 = nes::cart.mapper()->read_vram(chrAddr);
		uint8 plane1 = nes::cart.mapper()->read_vram(chrAddr + 8);

		for (int px = 0; px < spriteW; px++) {
			int srcX = flipH ? px : (7 - px);

			uint8 pixel = ((plane0 >> srcX) & 0x1)
				| (((plane1 >> srcX) & 0x1) << 1);

			if (pixel == 0) {
				continue;
			}

			BRect r(
				startX + (px * scale),
				startY + (py * scale),
				startX + ((px + 1) * scale) - 1.0f,
				startY + ((py + 1) * scale) - 1.0f
			);

			SetHighColor(SpritePreviewColor(palette, pixel));
			FillRect(r);
		}
	}

	// Inner sprite canvas outline.
	SetHighColor(40, 40, 40);
	StrokeRect(spriteRect);

	SetHighColor(255, 255, 255);
	StrokeLine(
		BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
		BPoint(spriteRect.right - 1.0f, spriteRect.top + 1.0f)
	);
	StrokeLine(
		BPoint(spriteRect.left + 1.0f, spriteRect.top + 1.0f),
		BPoint(spriteRect.left + 1.0f, spriteRect.bottom - 1.0f)
	);

	// Outer preview border, redrawn after all contents.
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
// Synchronizes the embedded CHR explorer with the active OAM sprite.  This
// decodes the sprite pattern table, CHR address, sprite palette, 8x8/8x16 mode,
// and OAM flip flags, then sends the corresponding CHR bytes to the explorer.
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

	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		fCHRExplorer->Clear();
		return;
	}

	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= 64) {
		fCHRExplorer->Clear();
		return;
	}

	uint32 base = (active * 4);

	uint8 spriteY = OAMByte(base + 0);
	uint8 tile = OAMByte(base + 1);
	uint8 attr = OAMByte(base + 2);

	if (spriteY >= 0xef) {
		fCHRExplorer->Clear();
		return;
	}

	uint8 spritePalette = (attr & 0x3);
	bool flipH = (attr & 0x40) != 0;
	bool flipV = (attr & 0x80) != 0;
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	if (largeSprites) {
		int32 whichPT = (tile & 0x1);
		int32 topTile = (tile & 0xfe);

		uint32 chrAddrTop = (whichPT ? 0x1000 : 0x0000)
			+ (topTile * 16);
		uint32 chrAddrBottom = chrAddrTop + 16;

		uint8 chrTop[16];
		uint8 chrBottom[16];

		for (int32 i = 0; i < 16; i++) {
			chrTop[i] = mapper->read_vram(chrAddrTop + i);
			chrBottom[i] = mapper->read_vram(chrAddrBottom + i);
		}

		fCHRExplorer->SetTile8x16(
			whichPT,
			topTile,
			fSpriteLocked,
			chrAddrTop,
			chrTop,
			chrAddrBottom,
			chrBottom,
			spritePalette
		);

		fCHRExplorer->SetUseSpritePalette(true);
		fCHRExplorer->SetSelectedPalette(spritePalette);
		fCHRExplorer->SetTileTransform(flipH, flipV);
	} else {
		int32 whichPT = (nes::ppu::ppuctrl() & 0x8) ? 1 : 0;
		uint32 chrAddr = (whichPT ? 0x1000 : 0x0000) + (tile * 16);
		uint8 chrBytes[16];

		for (int32 i = 0; i < 16; i++)
			chrBytes[i] = mapper->read_vram(chrAddr + i);

		fCHRExplorer->SetTile8x8(
			whichPT,
			tile,
			fSpriteLocked,
			chrAddr,
			chrBytes,
			spritePalette,
			-1,
			0,
			0,
			0,
			0
		);

		fCHRExplorer->SetUseSpritePalette(true);
		fCHRExplorer->SetSelectedPalette(spritePalette);
		fCHRExplorer->SetTileTransform(flipH, flipV);
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetPatternTables
//
// Stores references to the pattern table debugger windows so the OAM viewer can
// highlight the CHR tile used by the active sprite.
//
// Parameters:
//   pt0 - Pattern table window for pattern table 0 / CHR $0000.
//   pt1 - Pattern table window for pattern table 1 / CHR $1000.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetPatternTables(PatternTableWindow *pt0, PatternTableWindow *pt1)
{
	fPatternTable0 = pt0;
	fPatternTable1 = pt1;

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
	ClearPatternWindowHighlight(fPatternTable0);
	ClearPatternWindowHighlight(fPatternTable1);
}


// -----------------------------------------------------------------------------
// OAMDebugView::UpdatePatternTableHighlight
//
// Synchronizes the pattern table debugger windows with the active OAM sprite.
// In 8x8 mode, the sprite pattern table comes from PPUCTRL bit 3.  In 8x16
// mode, the low bit of the OAM tile selects the pattern table and the top tile
// is aligned with tile & $FE.
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
	int32 active = fSpriteLocked ? fLockedSprite : fHoverSprite;

	if (active < 0 || active >= 64) {
		ClearPatternTableHighlight();
		return;
	}

	uint32 base = (active * 4);

	uint8 tile = OAMByte(base + 1);
	bool largeSprites = (nes::ppu::ppuctrl() & 0x20) != 0;

	int32 whichPT;
	int32 tileIndex;

	if (largeSprites) {
		whichPT = tile & 0x1;
		tileIndex = tile & 0xfe;
	} else {
		whichPT = (nes::ppu::ppuctrl() & 0x8) ? 1 : 0;
		tileIndex = tile;
	}

	if (whichPT == 0) {
		SetPatternWindowHighlight(fPatternTable0, whichPT, tileIndex);
		ClearPatternWindowHighlight(fPatternTable1);
	} else {
		ClearPatternWindowHighlight(fPatternTable0);
		SetPatternWindowHighlight(fPatternTable1, whichPT, tileIndex);
	}
}


// -----------------------------------------------------------------------------
// OAMDebugView::SetExplorer
//
// Connects the embedded CHR explorer used by the OAM debugger.  The host
// palette is forwarded immediately if it is already available.
//
// Parameters:
//   explorer - CHRExplorerView used to inspect the active sprite tile.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
OAMDebugView::SetExplorer(CHRExplorerView *explorer)
{
	fCHRExplorer = explorer;

	if (fCHRExplorer && fHostPalette) {
		fCHRExplorer->SetHostPalette(fHostPalette);
	}

	UpdateCHRExplorer();
}

// -----------------------------------------------------------------------------
// OAMDebugView::CaptureOAMSnapshot
//
// Captures all 256 bytes of live PPU OAM into the display snapshot buffer.
// Snapshot mode keeps this buffer stable; live mode refreshes it each pulse.
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
	for (uint32 i = 0; i < 0x100; i++) {
		fFrozenOAM[i] = nes::ppu::oam_ram(i);
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
OAMDebugView::OAMByte(uint32 address) const
{
	if (fHaveFrozenOAM) {
		return fFrozenOAM[address & 0xff];
	}

	return nes::ppu::oam_ram(address);
}

