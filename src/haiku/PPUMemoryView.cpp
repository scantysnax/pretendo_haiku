
#include "PPUMemoryView.h"


// -----------------------------------------------------------------------------
// PPUMemoryScrollBar
//
// Small scrollbar subclass that forwards value changes back to PPUMemoryView.
//
// The scrollbar is intentionally not attached to PPUMemoryView as a normal
// scrolling target.  The PPU memory viewer has fixed header/inspector panels,
// so moving the scrollbar should change fBaseAddress only; it should not
// physically scroll the BView contents.
// -----------------------------------------------------------------------------
class PPUMemoryScrollBar : public BScrollBar
{
	public:
	PPUMemoryScrollBar (BRect frame, const char *name, PPUMemoryView *owner)
		: BScrollBar(frame, name, nullptr, 0.0f, 0.0f, B_VERTICAL),
		fOwner(owner)
	{
	}

	virtual void ValueChanged (float value)
	{
		if (fOwner) {
			fOwner->ScrollBarChanged(value);
		}
	}

	private:
	PPUMemoryView *fOwner = nullptr;
};


// -----------------------------------------------------------------------------
// PPUMemoryRegionLabel
//
// Returns a compact label for a PPU memory address range.
//
// Parameters:
//   address - PPU memory address.
//
// Returns:
//   Human-readable PPU memory region label.
// -----------------------------------------------------------------------------
static const char*
PPUMemoryRegionLabel (uint16 address)
{
	address &= 0x3fff;

	if (address <= 0x1fff) {
		return "CHR pattern";
	}

	if (address <= 0x2fff) {
		return "Name table";
	}

	if (address <= 0x3eff) {
		return "Name table mirror";
	}

	if (address <= 0x3fff) {
		return "Palette";
	}

	return "PPU memory";
}

// -----------------------------------------------------------------------------
// PPUMemoryPrintableChar
//
// Converts a byte value into a printable ASCII character for the PPU memory
// viewer.  Non-printable values are shown as dots.
//
// Parameters:
//   value - Byte value.
//
// Returns:
//   Printable character.
// -----------------------------------------------------------------------------
static char
PPUMemoryPrintableChar (uint8 value)
{
	if (value >= 32 && value <= 126) {
		return static_cast<char>(value);
	}

	return '.';
}


PPUMemoryView::PPUMemoryView (BRect frame, PretendoWindow *parent)
	: BView(frame, "ppu_memory_view", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS)
{
	fParent = parent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
	
	fScrollBar = new PPUMemoryScrollBar(BRect(0.0f, 0.0f, 0.0f, 0.0f), "ppu memory scroll", this);
	AddChild(fScrollBar);
}


PPUMemoryView::~PPUMemoryView()
{
}


// -----------------------------------------------------------------------------
// PPUMemoryView::AttachedToWindow
//
// Completes PPU Memory view setup after attachment to a window.
//
// The base BView attachment hook is invoked first, then the scrollbar, event
// mask, and keyboard focus are initialized.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);

	LayoutScrollBar();
	UpdateScrollBar();

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	MakeFocus(true);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::MouseDown
//
// Locks or unlocks the PPU memory byte under the mouse.
//
// Clicking a byte locks it for inspection. Clicking the same locked byte again
// unlocks it and returns the inspector to normal hover behavior.
//
// The view is given keyboard focus when clicked so navigation and freeze
// shortcuts can be used immediately.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::MouseDown (BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		return;
	}

	address &= 0x3fff;

	if (fHasLockedAddress && (fLockedAddress == address)) {
		fHasLockedAddress = false;
		fLockedAddress = 0x0000;
	} else {
		fHasLockedAddress = true;
		fLockedAddress = address;
	}

	fHasHoveredAddress = true;
	fHoveredAddress = address;

	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::MouseMoved
//
// Updates the hovered PPU memory byte while the pointer moves over the memory
// grid.
//
// Hover inspection follows the pointer whenever it is inside the view.  Leaving
// the view clears transient hover state but preserves any locked byte.
//
// Parameters:
//   where       - Mouse position in view coordinates.
//   transit     - BView mouse transit state.
//   dragMessage - Optional drag message; currently unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::MouseMoved (BPoint where, uint32 transit, const BMessage *dragMessage)
{
    (void)dragMessage;

    if (transit == B_ENTERED_VIEW) {
        MakeFocus(true);
    }

    if (transit == B_EXITED_VIEW) {
        if (fHasHoveredAddress) {
            fHasHoveredAddress = false;
            fHoveredAddress = 0x0000;
            Invalidate();
        }

        return;
    }

    if (!HasROMLoaded()) {
        return;
    }

    if (HoverAddressForPoint(where)) {
        Invalidate();
    }
}


// -----------------------------------------------------------------------------
// PPUMemoryView::MessageReceived
//
// Handles mouse-wheel scrolling for the PPU memory viewer.  Since the scrollbar
// is not attached as a normal BView scrolling target, wheel events are converted
// into row scrolling manually.
//
// One wheel notch scrolls one 16-byte row.  Larger wheel deltas scroll multiple
// rows.
//
// Parameters:
//   message - Incoming BeAPI message.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaY = 0.0f;

			if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK) {
				break;
			}

			if (deltaY == 0.0f) {
				break;
			}

			int32 lines = static_cast<int32>(deltaY);

			if (lines == 0) {
				lines = deltaY > 0.0f ? 1 : -1;
			}

			ScrollLines(lines);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


// -----------------------------------------------------------------------------
// PPUMemoryView::Pulse
//
// Refreshes the PPU Memory debugger.
//
// When a ROM is first loaded, one initial full-memory snapshot is captured even
// if the viewer is currently frozen.  In live mode, subsequent pulses refresh
// that snapshot once per pulse.  Frozen mode preserves the current snapshot.
//
// Hover inspection remains synchronized with the current mouse position while
// the pointer is inside the view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::Pulse()
{
	if (!HasROMLoaded()) {
		if (fHavePPUMemorySnapshot || fHasHoveredAddress || fHasLockedAddress || fBaseAddress != 0x0000) {
			Clear();
		}

		return;
	}

	/*
	 * A newly loaded ROM needs one valid snapshot even if the user
	 * previously left this debugger frozen.
	 */
	if (!fHavePPUMemorySnapshot) {
		CapturePPUMemorySnapshot();
	} else if (!fFreezeUpdates) {
		/*
		 * Live mode: refresh exactly once per pulse.
		 */
		CapturePPUMemorySnapshot();
	}

	BPoint where;
	uint32 buttons = 0;

	GetMouse(&where, &buttons, false);

	if (!Bounds().Contains(where)) {
		if (fHasHoveredAddress) {
			fHasHoveredAddress = false;
			fHoveredAddress = 0x0000;
		}

		Invalidate();
		return;
	}

	HoverAddressForPoint(where);

	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::KeyDown
//
// Handles keyboard controls for the PPU Memory viewer.
//
// Space toggles coherent PPU-memory freezing.  Entering freeze captures the
// complete PPU address space immediately so the frozen image represents the
// state at the moment Space was pressed.
//
// Address shortcuts and scrolling remain available while frozen so the user can
// inspect any portion of the captured 16 KB PPU address space.
//
// Parameters:
//   bytes    - Key bytes received from the keyboard event.
//   numBytes - Number of bytes in the key event.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		if (bytes[0] == ' ') {
			fFreezeUpdates = !fFreezeUpdates;
			Invalidate();
			return;
		}

		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case ' ':
			if (!fFreezeUpdates) {
				CapturePPUMemorySnapshot();
				fFreezeUpdates = true;
			} else {
				fFreezeUpdates = false;
			}

			Invalidate();
			break;

		case '1':
			SetBaseAddress(0x0000);
			break;

		case '2':
			SetBaseAddress(0x1000);
			break;

		case 'n':
		case 'N':
			SetBaseAddress(0x2000);
			break;

		case 'm':
		case 'M':
			SetBaseAddress(0x2400);
			break;

		case ',':
			SetBaseAddress(0x2800);
			break;

		case '.':
			SetBaseAddress(0x2c00);
			break;

		case 'p':
		case 'P':
			SetBaseAddress(0x3f00);
			break;

		case B_UP_ARROW:
			ScrollRows(-1);
			break;

		case B_DOWN_ARROW:
			ScrollRows(1);
			break;

		case B_PAGE_UP:
			ScrollRows(-16);
			break;

		case B_PAGE_DOWN:
			ScrollRows(16);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// PPUMemoryView::Draw
//
// Draws the PPU memory viewer.  If no ROM is loaded, the header remains visible,
// the scrollbar is hidden, and the memory panel shows a friendly empty-state
// message instead of zeroed memory.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	const bool hasROM = HasROMLoaded();

	if (fScrollBar) {
		if (hasROM && fScrollBar->IsHidden()) {
			fScrollBar->Show();
		} else if (!hasROM && !fScrollBar->IsHidden()) {
			fScrollBar->Hide();
		}
	}

	DrawHeaderPanel();

	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
						  ? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	if (!hasROM) {
		if (fBaseAddress != 0x0000) {
			fBaseAddress = 0x0000;
			UpdateScrollBar();
		}
		
		BRect panel(4.0f, 88.0f, rightEdge, Bounds().bottom - 8.0f);
		::DrawDebugPanel(this, panel, "PPU Memory");
		DrawNoROMMessage(panel);
		return;
	}

	DrawMemoryPanel();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::FrameResized
//
// Updates the PPU Memory viewer layout after the view size changes.
//
// The vertical scrollbar is repositioned to match the new bounds, then the
// resize event is forwarded to BView. The scrollbar's address range does not
// depend on the window dimensions, so it does not need to be recalculated here.
//
// Parameters:
//   width  - New view width.
//   height - New view height.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::FrameResized (float width, float height)
{
	(void)width;
	(void)height;

	LayoutScrollBar();

	BView::FrameResized(width, height);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::LayoutScrollBar
//
// Positions the vertical scrollbar along the right side of the PPU memory view.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::LayoutScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const float scrollBarWidth = B_V_SCROLL_BAR_WIDTH;
	BRect frame(Bounds().right - scrollBarWidth, 88.0f, Bounds().right, Bounds().bottom - 8.0f);
	
	fScrollBar->MoveTo(frame.LeftTop());
	fScrollBar->ResizeTo(frame.Width(), frame.Height());
	fScrollBar->Show();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::UpdateScrollBar
//
// Updates the PPU memory scrollbar.  The scrollbar value is a 16-byte row index.
// Since the viewer shows exactly 16 rows, the maximum scroll position is $3F00,
// allowing the final visible page to be $3F00-$3FFF.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::UpdateScrollBar()
{
	if (!fScrollBar) {
		return;
	}

	const float maxRow = static_cast<float>(0x3f00 / 16);
	const float currentRow = static_cast<float>((fBaseAddress & 0x3fff) / 16);

	fUpdatingScrollBar = true;

	fScrollBar->SetRange(0.0f, maxRow);
	fScrollBar->SetSteps(1.0f, 16.0f);
	fScrollBar->SetProportion(16.0f / 0x400);
	fScrollBar->SetValue(currentRow);

	fUpdatingScrollBar = false;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ScrollBarChanged
//
// Handles user scrollbar movement.  The scrollbar value is converted from a
// 16-byte row index into a PPU memory base address.
//
// Parameters:
//   value - New scrollbar row value.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::ScrollBarChanged (float value)
{
	if (fUpdatingScrollBar) {
		return;
	}

	int32 row = static_cast<int32>(value + 0.5f);

	if (row < 0) {
		row = 0;
	}

	if (row > 0x3f00 / 16) {
		row = 0x3f00 / 16;
	}

	fBaseAddress = static_cast<uint16>((row * 16) & 0x3fff);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ScrollLines
//
// Scrolls the PPU memory view by a number of 16-byte rows.  One line is $10
// bytes.  Sixteen lines is one full 256-byte page.
//
// Parameters:
//   lines - Signed row count to scroll.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::ScrollLines (int32 lines)
{
	int32 row = static_cast<int32>((fBaseAddress & 0x3fff) / 16);
	row += lines;

	if (row < 0) {
		row = 0;
	}

	if (row > 0x3f00 / 16) {
		row = 0x3f00 / 16;
	}

	fBaseAddress = static_cast<uint16>((row * 16) & 0x3fff);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawHeaderPanel
//
// Draws the title/help panel for the PPU memory viewer.
//
// The visible memory-window start address is labeled explicitly as "First" so
// it is not confused with the currently hovered or locked byte.  The address is
// drawn in a fixed-width font for easier hexadecimal reading.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawHeaderPanel()
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
						  ? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	BRect panel(4.0f, 4.0f, rightEdge, 76.0f);
	::DrawDebugPanel(this, panel, "PPU Memory");

	BFont previousFont;
	GetFont(&previousFont);

	BFont normal = previousFont;
	normal.SetSize(11.0f);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	const float y = panel.top + 42.0f;
	float x = panel.left + 8.0f;

	SetFont(&normal);
	SetHighColor(35, 35, 35);

	DrawString("First: ", BPoint(x, y));
	x += normal.StringWidth("First: ");

	BString address;
	address.SetToFormat("$%04X", fBaseAddress);

	SetFont(&fixed);
	DrawString(address.String(), BPoint(x, y));
	x += fixed.StringWidth("$0000") + 10.0f;

	SetFont(&normal);
	BString state;state.SetToFormat("%s   Space: %s", RegionName(fBaseAddress), fFreezeUpdates
									? "resume live" : "freeze");
	DrawString(state.String(), BPoint(x, y));
	DrawString("1/2 Pattern   N/M/,/. NTs   P Palette   Up/Down row   PgUp/PgDn page",
			   BPoint(panel.left + 8.0f, panel.top + 60.0f));

	SetFont(&previousFont);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawMemoryPanel
//
// Draws the PPU memory hex grid.  Bytes are drawn as individual cells so hover
// and locked-byte inspection can be highlighted like the CPU memory viewer.
//
// The memory grid is capped at 16 rows, which shows exactly 256 bytes at a time.
// The byte inspector uses a reserved bottom area so it never overlaps the grid
// rows.  Both the hex byte cells and ASCII character cells participate in
// hover/lock drawing.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawMemoryPanel()
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
						  ? fScrollBar->Frame().left - 4.0f : Bounds().right - 4.0f;

	BRect panel(4.0f, 88.0f, rightEdge, Bounds().bottom - 8.0f);
	::DrawDebugPanel(this, panel, "PPU Memory");

	if (!HasROMLoaded()) {
		DrawNoROMMessage(panel);
		return;
	}

	const float inspectorHeight = 92.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;
	const float gridBottom = inspectorTop - 8.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	SetFont(&fixed);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;
	const float charW = fixed.StringWidth("M");

	const float addrX = panel.left + 8.0f;
	const float byteX = addrX + 72.0f;
	const float byteStep = 24.0f;
	const float asciiX = byteX + byteStep * 16.0f + 12.0f;
	float y = panel.top + 42.0f;

	SetHighColor(80, 80, 80);
	DrawString("Address", BPoint(addrX, y));

	BString s;

	for (uint32 col = 0; col < 16; col++) {
		s.SetToFormat("+%X", col);
		DrawString(s.String(), BPoint(byteX + byteStep * col, y));
	}

	DrawString("ASCII", BPoint(asciiX, y));

	y += lineH + 8.0f;

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(panel.left + 8.0f, y - 8.0f), BPoint(panel.right - 8.0f, y - 8.0f));

	y += 4.0f;

	uint32 rows = static_cast<uint32>((gridBottom - y) / lineH);

	if (rows > 16) {
		rows = 16;
	}

	uint16 address = (fBaseAddress & 0x3fff);

	for (uint32 row = 0; row < rows; row++) {
		s.SetToFormat("$%04X", address);
		SetHighColor(75, 75, 75);
		DrawString(s.String(), BPoint(addrX, y));

		for (uint32 col = 0; col < 16; col++) {
			const uint16 cellAddress = static_cast<uint16>((address + col) & 0x3fff);
			const uint8 value = DisplayPPUMemory(cellAddress);
			const char ascii = PPUMemoryPrintableChar(value);

			const bool hovered = fHasHoveredAddress && (fHoveredAddress == cellAddress);
			const bool locked = fHasLockedAddress && (fLockedAddress == cellAddress);

			DrawByteCell(byteX + byteStep * col, y, cellAddress, value,
						 hovered, locked);

			DrawASCIICharCell(asciiX + charW * col, y, cellAddress, ascii,
							  hovered, locked);
		}

		address = static_cast<uint16>((address + 16) & 0x3fff);
		y += lineH;
	}

	SetFont(&prevFont);

	SetHighColor(170, 170, 170);
	StrokeLine(BPoint(panel.left + 8.0f, inspectorTop - 4.0f), 
			   BPoint(panel.right - 8.0f, inspectorTop - 4.0f));
	DrawSelectedByteInfo(panel.left + 8.0f, inspectorTop + 14.0f);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::HasROMLoaded
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
PPUMemoryView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawNoROMMessage
//
// Draws a friendly empty-state message when the PPU Memory window is opened
// without a loaded ROM.
//
// Parameters:
//   panel - Bounds in which the empty-state message should be centered.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded"; 
	const char *detail = "Load a cartridge to inspect PPU memory.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + (panel.Width() * 0.5f);
	const float centerY = panel.top + (panel.Height() * 0.5f);

	SetHighColor(80, 80, 80, 255);
	DrawString(title, BPoint(centerX - (StringWidth(title) * 0.5f), centerY - 8.0f));

	SetHighColor(120, 120, 120, 255);
	DrawString(detail, BPoint(centerX - (StringWidth(detail) * 0.5f), centerY + fh.ascent + 8.0f));

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::SetBaseAddress
//
// Sets the starting PPU address displayed by the memory viewer.
//
// The base is aligned to a 16-byte row boundary and clamped so a complete
// 256-byte / 16-row page always remains within the $0000-$3FFF PPU address
// space.  The final legal starting address is therefore $3F00.
//
// Parameters:
//   address - Requested PPU memory base address.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::SetBaseAddress (uint16 address)
{
	uint32 newAddress = static_cast<uint32>(address) & 0x3ff0;

	if (newAddress > 0x3f00) {
		newAddress = 0x3f00;
	}

	fBaseAddress = static_cast<uint16>(newAddress);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ScrollRows
//
// Scrolls the memory viewer by a signed number of 16-byte rows.
//
// Scrolling is clamped so the complete 16-row / 256-byte display always remains
// inside the PPU address space.  The final legal base address is $3F00.
//
// Parameters:
//   rows - Number of rows to scroll. Negative values scroll upward; positive
//          values scroll downward.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::ScrollRows (int32 rows)
{
	int32 address = static_cast<int32>(fBaseAddress & 0x3ff0);
	address += rows * 16;

	if (address < 0x0000) {
		address = 0x0000;
	}

	if (address > 0x3f00) {
		address = 0x3f00;
	}

	fBaseAddress = static_cast<uint16>(address);

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::RegionName
//
// Returns a short human-readable label for a PPU address region.
//
// Parameters:
//   address - PPU address.
//
// Returns:
//   Region label.
// -----------------------------------------------------------------------------
const char*
PPUMemoryView::RegionName (uint16 address) const
{
	address &= 0x3fff;

	if (address < 0x1000) {
		return "Pattern Table 0";
	}

	if (address < 0x2000) {
		return "Pattern Table 1";
	}
	
	if (address < 0x2400) {
		return "Nametable 0";
	}

	if (address < 0x2800) {
		return "Nametable 1";
	}

	if (address < 0x2c00) {
		return "Nametable 2";
	}

	if (address < 0x3000) {
		return "Nametable 3";
	}

	if (address < 0x3f00) {
		return "Nametable Mirrors";
	}

	return "Palette RAM";
}


// -----------------------------------------------------------------------------
// PPUMemoryView::ActiveInspectAddress
//
// Returns the active byte address for the inspector.  A locked byte takes
// priority over the current hover byte.
//
// Parameters:
//   address - Receives the active PPU memory address.
//
// Returns:
//   true if there is an active byte to inspect.
// -----------------------------------------------------------------------------
bool
PPUMemoryView::ActiveInspectAddress (uint16 &address) const
{
	if (fHasLockedAddress) {
		address = fLockedAddress;
		return true;
	}

	if (fHasHoveredAddress) {
		address = fHoveredAddress;
		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::AddressForPoint
//
// Converts a mouse position over the hex byte grid or ASCII byte grid into a
// PPU memory address.
//
// The bottom byte-inspector area is excluded from hit testing so hovering over
// inspector text does not accidentally select memory bytes.  Hit testing is
// capped at 16 visible rows, matching DrawMemoryPanel().
//
// Parameters:
//   where   - Mouse position in view coordinates.
//   address - Receives the PPU memory address under the pointer.
//
// Returns:
//   true if the point maps to a visible byte cell.
// -----------------------------------------------------------------------------
bool
PPUMemoryView::AddressForPoint(BPoint where, uint16& address) const
{
	const float rightEdge = (fScrollBar && !fScrollBar->IsHidden())
						  ? fScrollBar->Frame().left - 4.0f
						  : Bounds().right - 4.0f;

	BRect panel(4.0f, 88.0f, rightEdge, Bounds().bottom - 8.0f);
	
	if (!panel.Contains(where)) {
		return false;
	}

	const float inspectorHeight = 92.0f;
	const float inspectorTop = panel.bottom - inspectorHeight;
	const float gridBottom = inspectorTop - 8.0f;

	if (where.y >= inspectorTop) {
		return false;
	}

	BFont prevFont;
	const_cast<PPUMemoryView *>(this)->GetFont(&prevFont);

	BFont fixed(be_fixed_font);
	fixed.SetSize(10.0f);
	const_cast<PPUMemoryView *>(this)->SetFont(&fixed);

	font_height fh;
	const_cast<PPUMemoryView *>(this)->GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;
	const float charW = fixed.StringWidth("M");

	const_cast<PPUMemoryView *>(this)->SetFont(&prevFont);

	const float addrX = panel.left + 8.0f;
	const float byteX = addrX + 72.0f;
	const float byteStep = 24.0f;
	const float asciiX = byteX + byteStep * 16.0f + 12.0f;
	float y = panel.top + 42.0f;
	
	y += lineH + 8.0f;
	y += 4.0f;

	uint32 rows = static_cast<uint32>((gridBottom - y) / lineH);

	if (rows > 16) {
		rows = 16;
	}

	for (uint32 row = 0; row < rows; row++) {
		const float rowTop = y - 11.0f;
		const float rowBottom = y + 3.0f;

		if (where.y >= rowTop && where.y <= rowBottom) {
			for (uint32 col = 0; col < 16; col++) {
				const float x = byteX + byteStep * col;
				const float cellLeft = x - 2.0f;
				const float cellRight = x + 18.0f;

				if (where.x >= cellLeft && where.x <= cellRight) {
					address = static_cast<uint16>((fBaseAddress + row * 16 + col) & 0x3fff);
					return true;
				}
			}

			for (uint32 col = 0; col < 16; col++) {
				const float x = asciiX + charW * col;
				const float cellLeft = x - 2.0f;
				const float cellRight = x + charW + 2.0f;

				if ((where.x >= cellLeft) && (where.x <= cellRight)) {
					address = static_cast<uint16>((fBaseAddress + row * 16 + col) & 0x3fff);
					return true;
				}
			}
		}

		y += lineH;
	}

	return false;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::HoverAddressForPoint
//
// Updates the hovered PPU memory address from a mouse position.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   true if hover state changed.
// -----------------------------------------------------------------------------
bool
PPUMemoryView::HoverAddressForPoint (BPoint where)
{
	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		if (!fHasHoveredAddress) {
			return false;
		}

		fHasHoveredAddress = false;
		fHoveredAddress = 0x0000;
		return true;
	}

	address &= 0x3fff;

	if (fHasHoveredAddress && (fHoveredAddress == address)) {
		return false;
	}

	fHasHoveredAddress = true;
	fHoveredAddress = address;
	return true;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::CapturePPUMemorySnapshot
//
// Captures the complete 16 KB PPU address space represented by the PPU Memory
// debugger.
//
// A full-memory snapshot allows the viewer to remain coherent while frozen even
// when the user scrolls to a different region after freezing.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::CapturePPUMemorySnapshot()
{
	if (!HasROMLoaded()) {
		fHavePPUMemorySnapshot = false;
		return;
	}

	for (uint32 address = 0; address < 0x4000; address++) {
		fSnapshotPPUMemory[address] = nes::ppu::debug_read_ppu_memory(static_cast<uint16>(address));
	}

	fHavePPUMemorySnapshot = true;
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DisplayPPUMemory
//
// Reads one byte from the PPU memory state currently represented by the viewer.
//
// Once a valid snapshot exists, display and inspection use the captured value.
// Before the first snapshot is available, the current live PPU memory value is
// used as a fallback.
//
// Parameters:
//   address - PPU memory address to read.
//
// Returns:
//   Snapshot or live byte value at the requested PPU address.
// -----------------------------------------------------------------------------
uint8
PPUMemoryView::DisplayPPUMemory (uint16 address) const
{
	address &= 0x3fff;

	if (fHavePPUMemorySnapshot) {
		return fSnapshotPPUMemory[address];
	}

	return nes::ppu::debug_read_ppu_memory(address);
}


// -----------------------------------------------------------------------------
// PPUMemoryView::Clear
//
// Clears all ROM-specific PPU Memory debugger state while preserving the
// viewer's freeze preference.
//
// The current 16 KB PPU-memory snapshot, hover/lock selection, and base address
// are discarded so data from an unloaded cartridge cannot remain visible after
// another ROM is loaded.
//
// Freeze mode is intentionally preserved.  If a new ROM is loaded while the
// viewer is still frozen, Pulse() will capture one initialization snapshot
// before preserving the frozen state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::Clear()
{
	fHavePPUMemorySnapshot = false;
	memset(fSnapshotPPUMemory, 0, sizeof(fSnapshotPPUMemory));

	fHasHoveredAddress = false;
	fHoveredAddress = 0x0000;

	fHasLockedAddress = false;
	fLockedAddress = 0x0000;

	fBaseAddress = 0x0000;

	UpdateScrollBar();
	Invalidate();
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawByteCell
//
// Draws one hexadecimal byte cell in the PPU memory grid.
//
// Hovered bytes receive a clearly visible light-gray fill and medium-gray
// outline. Locked bytes use a brighter fill and black outline and take visual
// priority over hover.
//
// Parameters:
//   x       - Text x position.
//   y       - Text baseline.
//   address - PPU memory address for the cell.
//   value   - Byte value to draw.
//   hovered - Whether this byte is currently hovered.
//   locked  - Whether this byte is currently locked.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawByteCell (float x, float y, uint16 address, uint8 value, bool hovered, bool locked)
{
	(void)address;
	
	BRect cell(x - 2.0f, y - 11.0f, x + 18.0f, y + 3.0f);

	if (hovered && !locked) {
		SetHighColor(240, 240, 240);
		FillRect(cell);

		SetHighColor(120, 120, 120);
		StrokeRect(cell);
	}

	if (locked) {
		SetHighColor(245, 245, 245);
		FillRect(cell);

		SetHighColor(0, 0, 0);
		StrokeRect(cell);

		// Make the bottom edge reliable on BeAPI's inclusive rectangle drawing.
		StrokeLine(BPoint(cell.left, cell.bottom), BPoint(cell.right, cell.bottom));
	}

	BString s;
	s.SetToFormat("%02X", value);

	SetHighColor(0, 0, 0);
	DrawString(s.String(), BPoint(x, y));
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawASCIICharCell
//
// Draws one ASCII character cell in the PPU memory grid.
//
// Hovered characters receive the same visible hover treatment as their
// corresponding hexadecimal byte. Locked characters use the stronger locked
// appearance.
//
// Parameters:
//   x       - Text x position.
//   y       - Text baseline.
//   address - PPU memory address represented by the character.
//   value   - Printable ASCII character.
//   hovered - Whether this byte is currently hovered.
//   locked  - Whether this byte is currently locked.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawASCIICharCell (float x, float y, uint16 address, char value, bool hovered, bool locked)
{
	(void)address;

	BFont font;
	GetFont(&font);

	const float charW = font.StringWidth("M");
	BRect cell(x - 1.0f, y - 11.0f, x + charW, y + 3.0f);

	if (hovered && !locked) {
		SetHighColor(240, 240, 240);
		FillRect(cell);

		SetHighColor(120, 120, 120);
		StrokeRect(cell);
	}

	if (locked) {
		SetHighColor(245, 245, 245);
		FillRect(cell);

		SetHighColor(0, 0, 0);
		StrokeRect(cell);
		StrokeLine(BPoint(cell.left, cell.bottom), BPoint(cell.right, cell.bottom));
	}

	char text[2];
	text[0] = value;
	text[1] = '\0';

	SetHighColor(75, 75, 75);
	DrawString(text, BPoint(x, y));
}


// -----------------------------------------------------------------------------
// PPUMemoryView::DrawSelectedByteInfo
//
// Draws details for the active PPU memory byte.  A locked byte takes priority
// over a hovered byte.  If no byte is active, a short hint is shown instead.
//
// Hexadecimal values are drawn with a fixed-width font so addresses, byte
// values, offsets, palette indexes, and CHR details align cleanly.
//
// Parameters:
//   x - Left text position.
//   y - First text baseline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PPUMemoryView::DrawSelectedByteInfo (float x, float y)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont normal;
	GetFont(&normal);
	normal.SetSize(11.0f);

	BFont fixed(be_fixed_font);
	fixed.SetSize(11.0f);

	auto drawNormal = [&](const char *text, float drawX, float drawY, rgb_color color) {
		SetFont(&normal);
		SetHighColor(color);
		DrawString(text, BPoint(drawX, drawY));
	};

	auto drawFixed = [&](const char *text, float drawX, float drawY, rgb_color color) {
		SetFont(&fixed);
		SetHighColor(color);
		DrawString(text, BPoint(drawX, drawY));
	};

	BString s;

	drawNormal("Byte Inspector", x, y, rgb_color { 0, 0, 0, 255 });

	y += 16.0f;

	uint16 address = 0x0000;

	if (!ActiveInspectAddress(address)) {
		drawNormal("Hover a byte, or click to lock.", x, y, rgb_color {100, 100, 100, 255 });

		SetFont(&prevFont);
		return;
	}

	address &= 0x3fff;
	const uint8 value = DisplayPPUMemory(address);

	if (fHasLockedAddress && (fLockedAddress == address)) {
		drawNormal("State: locked", x, y, rgb_color { 0, 0, 0, 255 });
	} else {
		drawNormal("State: hover", x, y, rgb_color {90, 90, 90, 255 });
	}

	y += 14.0f;

	const float labelX = x;
	const float valueX = x + 72.0f;

	drawNormal("Address:", labelX, y, rgb_color { 0, 0, 0, 255 });
	s.SetToFormat("$%04X", address);
	drawFixed(s.String(), valueX, y, rgb_color {0, 0, 0, 255 });

	y += 14.0f;

	drawNormal("Value:", labelX, y, rgb_color {0, 0, 0, 255 });
	s.SetToFormat("$%02X", value);
	drawFixed(s.String(), valueX, y, rgb_color {0, 0, 0, 255 });

	s.SetToFormat("  %u", value);
	drawFixed(s.String(), valueX + fixed.StringWidth("$00"), y, rgb_color { 80, 80, 80, 255 });

	y += 14.0f;

	drawNormal("Region:", labelX, y, rgb_color { 0, 0, 0, 255 });
	drawNormal(PPUMemoryRegionLabel(address), valueX, y, rgb_color {0, 0, 0, 255 });

	y += 14.0f;

	if (address >= 0x2000 && address <= 0x2fff) {
		const uint16 nameTable = static_cast<uint16>((address - 0x2000) / 0x400);
		const uint16 offset = static_cast<uint16>((address - 0x2000) & 0x3ff);

		drawNormal("NT:", labelX, y, rgb_color{0, 0, 0, 255});

		s.SetToFormat("%u", nameTable);
		drawFixed(s.String(), valueX, y, rgb_color { 0, 0, 0, 255 });
		drawNormal(" offset ", valueX + fixed.StringWidth("0") + 10.0f, y, rgb_color { 0, 0, 0, 255 });

		s.SetToFormat("$%03X", offset);
		drawFixed(s.String(), valueX + fixed.StringWidth("0") + 58.0f, y, rgb_color {0, 0, 0, 255 });
	} else if (address >= 0x3f00 && address <= 0x3fff) {
		const uint16 paletteIndex = static_cast<uint16>((address - 0x3f00) & 0x1f);

		drawNormal("Palette:", labelX, y, rgb_color {0, 0, 0, 255 });

		s.SetToFormat("index $%02X", paletteIndex);
		drawFixed(s.String(), valueX, y, rgb_color {0, 0, 0, 255 });
	} else if (address <= 0x1fff) {
		const uint16 table = static_cast<uint16>(address / 0x1000);
		const uint16 tile = static_cast<uint16>((address & 0xfff) / 16);
		const uint16 planeByte = static_cast<uint16>(address & 0xf);

		drawNormal("CHR:", labelX, y, rgb_color { 0, 0, 0, 255 });

		s.SetToFormat("PT %u  tile $%02X  byte %u", table, tile, planeByte);
		drawFixed(s.String(), valueX, y, rgb_color {0, 0, 0, 255 });
	} else {
		drawNormal("Mirror:", labelX, y, rgb_color {0, 0, 0, 255 });

		s.SetToFormat("$%04X", static_cast<uint16>(address & 0x2fff));
		drawFixed(s.String(), valueX, y, rgb_color {0, 0, 0, 255 });
	}

	SetFont(&prevFont);
}

