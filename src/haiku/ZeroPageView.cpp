
#include "ZeroPageView.h"


// -----------------------------------------------------------------------------
// ZeroPageView::ZeroPageView
//
// Creates the Zero Page debugger view.
//
// Parameters:
//   frame  - View frame.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
ZeroPageView::ZeroPageView (BRect frame, PretendoWindow *parent)
	: BView(
		frame,
		"zero page view",
		B_FOLLOW_ALL,
		B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE
	),
	fParent(parent)
{
	(void)fParent;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(B_TRANSPARENT_COLOR);
}


// -----------------------------------------------------------------------------
// ZeroPageView::~ZeroPageView
//
// Destroys the Zero Page debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
ZeroPageView::~ZeroPageView()
{
}


// -----------------------------------------------------------------------------
// ZeroPageView::AttachedToWindow
//
// Initializes the view after it is attached to a window.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::AttachedToWindow()
{
	BView::AttachedToWindow();

	MakeFocus(true);

	if (HasROMLoaded()) {
		CaptureZeroPageSnapshot();
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// ZeroPageView::Draw
//
// Draws the complete Zero Page debugger.
//
// Parameters:
//   updateRect - Area being redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(216, 216, 216);
	FillRect(Bounds());

	DrawHeaderUI();

	if (!HasROMLoaded()) {
		BRect panel(
			4.0f,
			88.0f,
			Bounds().right - 4.0f,
			Bounds().bottom - 8.0f
		);

		::DrawDebugPanel(this, panel, "Zero Page RAM");
		DrawNoROMMessage(panel);
		return;
	}

	DrawZeroPageGrid();
	DrawSelectedBytePanel();
}


// -----------------------------------------------------------------------------
// ZeroPageView::KeyDown
//
// Handles keyboard shortcuts for the Zero Page debugger.
//
// Parameters:
//   bytes    - Key bytes from BeAPI.
//   numBytes - Number of bytes in the key sequence.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::KeyDown (const char *bytes, int32 numBytes)
{
	if (!bytes || numBytes <= 0) {
		return;
	}

	if (!HasROMLoaded()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case ' ':
			fFreezeUpdates = !fFreezeUpdates;

			if (fFreezeUpdates) {
				CaptureZeroPageSnapshot();
			}

			Invalidate();
			break;

		case 'r':
		case 'R':
			CaptureZeroPageSnapshot();
			Invalidate();
			break;

		case B_LEFT_ARROW:
			MoveSelection(-1);
			break;

		case B_RIGHT_ARROW:
			MoveSelection(1);
			break;

		case B_UP_ARROW:
			MoveSelection(-16);
			break;

		case B_DOWN_ARROW:
			MoveSelection(16);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


// -----------------------------------------------------------------------------
// ZeroPageView::MouseDown
//
// Selects a zero-page byte for inspection.  Clicking the currently selected
// byte again clears the selection.
//
// Parameters:
//   where - Mouse position in view coordinates.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::MouseDown (BPoint where)
{
	MakeFocus(true);

	if (!HasROMLoaded()) {
		return;
	}

	uint16 address = 0x0000;

	if (!AddressForPoint(where, address)) {
		return;
	}

	if (fHasSelectedAddress && fSelectedAddress == address) {
		fHasSelectedAddress = false;
		fSelectedAddress = 0x0000;
	} else {
		fHasSelectedAddress = true;
		fSelectedAddress = address;
	}

	Invalidate();
}


// -----------------------------------------------------------------------------
// ZeroPageView::Pulse
//
// Periodically refreshes the live zero-page snapshot.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::Pulse()
{
	if (!HasROMLoaded()) {
		return;
	}

	if (fFreezeUpdates) {
		return;
	}

	CaptureZeroPageSnapshot();
	Invalidate();
}


// -----------------------------------------------------------------------------
// ZeroPageView::DrawHeaderUI
//
// Draws the controls/help panel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::DrawHeaderUI()
{
	BRect panel(
		4.0f,
		4.0f,
		Bounds().right - 4.0f,
		76.0f
	);

	::DrawDebugPanel(this, panel, "Controls");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 1.0f;

	const float labelX = panel.left + 8.0f;
	const float valueX = labelX + 64.0f;

	float y = panel.top + 34.0f;

	auto drawKV = [&](const char *label, const char *value) {
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(labelX, y));

		SetHighColor(35, 35, 35);
		DrawString(value, BPoint(valueX, y));

		y += lineH;
	};

	drawKV("Mouse:", "click select / same click clear");

	drawKV("Keys:", "Arrows move selection   R refresh");

	drawKV("Space:", fFreezeUpdates
		? "live updates"
		: "freeze snapshot");
}


// -----------------------------------------------------------------------------
// ZeroPageView::DrawZeroPageGrid
//
// Draws the 16x16 zero-page byte grid.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::DrawZeroPageGrid()
{
	BRect panel(
		4.0f,
		88.0f,
		Bounds().right - 4.0f,
		448.0f
	);

	::DrawDebugPanel(this, panel, "Zero Page RAM");

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);
	SetFont(&mono);

	const float rowLabelX = panel.left + 10.0f;
	const float firstCellX = panel.left + 52.0f;
	const float firstCellY = panel.top + 58.0f;

	const float cellW = 27.0f;
	const float cellH = 17.0f;

	BString s;

	SetHighColor(80, 80, 80);

	for (int32 col = 0; col < 16; col++) {
		s.SetToFormat("%X", static_cast<unsigned>(col));
		DrawString(
			s.String(),
			BPoint(firstCellX + col * cellW + 6.0f, panel.top + 36.0f)
		);
	}

	for (int32 row = 0; row < 16; row++) {
		s.SetToFormat("$%02X", static_cast<unsigned>(row * 16));
		SetHighColor(80, 80, 80);
		DrawString(s.String(), BPoint(rowLabelX, firstCellY + row * cellH));

		for (int32 col = 0; col < 16; col++) {
			const uint16 address = static_cast<uint16>((row * 16) + col);
			const uint8 value = fBytes[address];

			const float x = firstCellX + col * cellW;
			const float y = firstCellY + row * cellH;

			BRect cellRect(
				x - 2.0f,
				y - 12.0f,
				x + cellW - 4.0f,
				y + 4.0f
			);

			const bool selected = fHasSelectedAddress
				&& fSelectedAddress == address;

			if (selected) {
				SetHighColor(190, 215, 245);
				FillRect(cellRect);

				SetHighColor(70, 120, 180);
				StrokeRect(cellRect);
			} else if (fChanged[address]) {
				SetHighColor(255, 245, 170);
				FillRect(cellRect);
			}

			SetHighColor(0, 0, 0);
			s.SetToFormat("%02X", value);
			DrawString(s.String(), BPoint(x, y));
		}
	}

	SetFont(&prevFont);

	SetHighColor(90, 90, 90);

	BString footer;

	footer.SetToFormat(
		"%s view. Yellow = recently changed. Changed bytes: %ld",
		fFreezeUpdates ? "Frozen" : "Live",
		static_cast<long>(ChangedByteCount())
	);

	DrawString(
		footer.String(),
		BPoint(panel.left + 10.0f, panel.bottom - 14.0f)
	);
}


// -----------------------------------------------------------------------------
// ZeroPageView::DrawSelectedBytePanel
//
// Draws the selected-byte inspector.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::DrawSelectedBytePanel()
{
	BRect panel(
		4.0f,
		458.0f,
		Bounds().right - 4.0f,
		Bounds().bottom - 8.0f
	);

	::DrawDebugPanel(this, panel, "Selected Byte");

	SetFontSize(11.0f);

	font_height fh;
	GetFontHeight(&fh);
	const float lineH = ceilf(fh.ascent + fh.descent + fh.leading) + 3.0f;

	BFont prevFont;
	GetFont(&prevFont);

	BFont mono(be_fixed_font);
	mono.SetSize(11.0f);

	const float labelX = panel.left + 10.0f;
	const float valueX = labelX + 92.0f;
	const float col2X = panel.left + 260.0f;
	const float col2ValueX = col2X + 80.0f;

	float y = panel.top + 36.0f;

	auto drawKV = [&](const char *label, const char *value,
					 bool monoValue, float lx, float vx) {
		SetFont(&prevFont);
		SetHighColor(80, 80, 80);
		DrawString(label, BPoint(lx, y));

		SetFont(monoValue ? &mono : &prevFont);
		SetHighColor(0, 0, 0);
		DrawString(value, BPoint(vx, y));
	};

	if (!fHasSelectedAddress) {
		SetHighColor(90, 90, 90);
		DrawString(
			"Click a zero-page byte to inspect it.",
			BPoint(labelX, y)
		);

		SetFont(&prevFont);
		return;
	}

	const uint16 address = fSelectedAddress & 0xff;
	const uint8 value = fBytes[address];

	BString s;

	s.SetToFormat("$%02X", address);
	drawKV("Address:", s.String(), true, labelX, valueX);

	s.SetToFormat("$%02X", value);
	drawKV("Hex:", s.String(), true, col2X, col2ValueX);

	y += lineH;

	s.SetToFormat("%u", static_cast<unsigned>(value));
	drawKV("Unsigned:", s.String(), false, labelX, valueX);

	int32 signedValue = static_cast<int32>(
		static_cast<int8>(value)
	);
	s.SetToFormat("%ld", static_cast<long>(signedValue));
	drawKV("Signed:", s.String(), false, col2X, col2ValueX);

	y += lineH;

	BString binary;

	for (int32 bit = 7; bit >= 0; bit--) {
		binary << (((value >> bit) & 0x1) ? "1" : "0");

		if (bit == 4) {
			binary << " ";
		}
	}

	drawKV("Binary:", binary.String(), true, labelX, valueX);

	if (address < 0xff) {
		const uint8 lo = fBytes[address];
		const uint8 hi = fBytes[address + 1];
		const uint16 pointer = static_cast<uint16>(lo | (hi << 8));

		s.SetToFormat("$%04X", pointer);
		drawKV("Ptr pair:", s.String(), true, col2X, col2ValueX);
	} else {
		drawKV("Ptr pair:", "--", true, col2X, col2ValueX);
	}

	y += lineH;

	s.SetToFormat(
		"%s",
		fChanged[address] ? "changed" : "unchanged"
	);
	drawKV("State:", s.String(), false, labelX, valueX);

	drawKV("Mode:", fFreezeUpdates ? "frozen" : "live", false,
		col2X, col2ValueX);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// ZeroPageView::DrawNoROMMessage
//
// Draws the no-ROM empty state.
//
// Parameters:
//   panel - Panel bounds.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::DrawNoROMMessage (BRect panel)
{
	BFont prevFont;
	GetFont(&prevFont);

	BFont font = prevFont;
	font.SetSize(12.0f);
	SetFont(&font);

	const char *title = "No ROM loaded";
	const char *detail = "Load a cartridge to inspect zero page RAM.";

	font_height fh;
	GetFontHeight(&fh);

	const float centerX = panel.left + panel.Width() * 0.5f;
	const float centerY = panel.top + panel.Height() * 0.5f;

	SetHighColor(80, 80, 80);
	DrawString(
		title,
		BPoint(
			centerX - StringWidth(title) * 0.5f,
			centerY - 8.0f
		)
	);

	SetHighColor(120, 120, 120);
	DrawString(
		detail,
		BPoint(
			centerX - StringWidth(detail) * 0.5f,
			centerY + fh.ascent + 8.0f
		)
	);

	SetFont(&prevFont);
}


// -----------------------------------------------------------------------------
// ZeroPageView::CaptureZeroPageSnapshot
//
// Captures all 256 zero-page bytes and records which bytes changed since the
// previous snapshot.  Changed-byte highlights linger briefly so rapid changes
// remain visible to the user.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::CaptureZeroPageSnapshot()
{
	static const uint8 kChangeHoldFrames = 8;

	for (uint32 i = 0; i < 0x100; i++) {
		fPreviousBytes[i] = fBytes[i];
	}

	for (uint32 i = 0; i < 0x100; i++) {
		const uint8 value = nes::bus::debug_read_memory(static_cast<uint16>(i));

		fBytes[i] = value;

		if (!fHaveSnapshot) {
			fChangeAge[i] = 0;
			fChanged[i] = false;
			continue;
		}

		if (fPreviousBytes[i] != fBytes[i]) {
			fChangeAge[i] = kChangeHoldFrames;
		} else if (fChangeAge[i] > 0) {
			fChangeAge[i]--;
		}

		fChanged[i] = fChangeAge[i] > 0;
	}

	fHaveSnapshot = true;
}


// -----------------------------------------------------------------------------
// ZeroPageView::HasROMLoaded
//
// Returns whether a cartridge mapper is currently available.
//
// Parameters:
//   None.
//
// Returns:
//   true if a ROM is loaded.
// -----------------------------------------------------------------------------
bool
ZeroPageView::HasROMLoaded() const
{
	return nes::cart.mapper() != nullptr;
}


// -----------------------------------------------------------------------------
// ZeroPageView::AddressForPoint
//
// Converts a mouse point over the zero-page grid to an address.
//
// Parameters:
//   where   - Mouse point in view coordinates.
//   address - Receives the zero-page address.
//
// Returns:
//   true if the point hits a byte cell.
// -----------------------------------------------------------------------------
bool
ZeroPageView::AddressForPoint (BPoint where, uint16 &address) const
{
	BRect panel(
		4.0f,
		88.0f,
		Bounds().right - 4.0f,
		418.0f
	);

	if (!panel.Contains(where)) {
		return false;
	}

	const float firstCellX = panel.left + 52.0f;
	const float firstCellY = panel.top + 58.0f;

	const float cellW = 27.0f;
	const float cellH = 17.0f;

	const int32 col = static_cast<int32>((where.x - firstCellX) / cellW);
	const int32 row = static_cast<int32>((where.y - (firstCellY - 12.0f))
		/ cellH);

	if (col < 0 || col >= 16 || row < 0 || row >= 16) {
		return false;
	}

	address = static_cast<uint16>((row * 16) + col);
	return true;
}


// -----------------------------------------------------------------------------
// ZeroPageView::MoveSelection
//
// Moves the selected zero-page address by a signed delta.
//
// Parameters:
//   delta - Signed movement amount.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
ZeroPageView::MoveSelection (int32 delta)
{
	int32 address = fHasSelectedAddress
		? static_cast<int32>(fSelectedAddress)
		: 0;

	address += delta;

	if (address < 0) {
		address = 0;
	}

	if (address > 0xff) {
		address = 0xff;
	}

	fHasSelectedAddress = true;
	fSelectedAddress = static_cast<uint16>(address);

	Invalidate();
}

// -----------------------------------------------------------------------------
// ZeroPageView::ChangedByteCount
//
// Counts how many zero-page bytes are currently marked as recently changed.
//
// Parameters:
//   None.
//
// Returns:
//   Number of highlighted changed bytes.
// -----------------------------------------------------------------------------
int32
ZeroPageView::ChangedByteCount() const
{
	int32 count = 0;

	for (uint32 i = 0; i < 0x100; i++) {
		if (fChanged[i]) {
			count++;
		}
	}

	return count;
}

