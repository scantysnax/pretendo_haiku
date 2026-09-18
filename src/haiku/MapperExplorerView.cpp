

#include "MapperExplorerView.h"
#include "PretendoWindow.h"
#include "Cart.h"

#include <Font.h>


// -----------------------------------------------------------------------------
// MapperExplorerView::MapperExplorerView
//
// Creates the Mapper Explorer debugger view.
//
// Parameters:
//   frame  - Initial view frame.
//   parent - Owning PretendoWindow.
//
// Returns:
//   Constructor; no return value.
// -----------------------------------------------------------------------------
MapperExplorerView::MapperExplorerView (BRect frame, PretendoWindow *parent)
	: BView(frame, "mapper_explorer_view", B_FOLLOW_ALL,
			B_WILL_DRAW | B_PULSE_NEEDED)
{
	fParent = parent;

	SetViewColor(240, 240, 240);
	SetLowColor(ViewColor());
}


// -----------------------------------------------------------------------------
// MapperExplorerView::~MapperExplorerView
//
// Destroys the Mapper Explorer debugger view.
//
// Parameters:
//   None.
//
// Returns:
//   Destructor; no return value.
// -----------------------------------------------------------------------------
MapperExplorerView::~MapperExplorerView()
{
}


void
MapperExplorerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	CaptureState();
	CaptureMMC1State();

	Invalidate();
}


// -----------------------------------------------------------------------------
// MapperExplorerView::Pulse
//
// Updates live mapper state, mapper-specific debugger state, and recent-change
// highlighting.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::Pulse()
{
	bool redraw = CaptureState();

	if (CaptureMMC1State()) {
		redraw = true;
	}

	for (int i = 0; i < 5; ++i) {
		if (fPRGChangeTicks[i] > 0) {
			--fPRGChangeTicks[i];
			redraw = true;
		}
	}

	for (int i = 0; i < 8; ++i) {
		if (fCHRChangeTicks[i] > 0) {
			--fCHRChangeTicks[i];
			redraw = true;
		}
	}

	if (redraw) {
		Invalidate();
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureState
//
// Captures the current generic mapper debug state.
//
// A change is reported when a mapper is loaded or unloaded, when the mapper
// object changes, or when its debugger-visible revision changes.
//
// Individual PRG and CHR rows are also compared against the previous state so
// recently changed mappings can be highlighted briefly.
//
// Parameters:
//   None.
//
// Returns:
//   true if the cached display state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureState()
{
	Mapper *mapper = nes::cart.mapper();

	if (!mapper) {
		if (!fMapper && !fHaveState) {
			return false;
		}

		fMapper = nullptr;
		fMapperName = "";
		fState = {};
		fHaveState = false;

		for (int i = 0; i < 5; ++i) {
			fPRGChangeTicks[i] = 0;
		}

		for (int i = 0; i < 8; ++i) {
			fCHRChangeTicks[i] = 0;
		}

		return true;
	}

	const mapper_debug_state_t state = mapper->debug_state();
	const std::string mapperName = mapper->name();

	const bool mapperChanged = mapper != fMapper;
	const bool revisionChanged =
		!fHaveState || state.revision != fState.revision;
	const bool nameChanged =
		fMapperName != mapperName.c_str();

	if (!mapperChanged && !revisionChanged && !nameChanged) {
		return false;
	}

	if (fHaveState && !mapperChanged) {
		for (int i = 0; i < 5; ++i) {
			if (BankStateChanged(fState.prg[i], state.prg[i])) {
				fPRGChangeTicks[i] = 12;
			}
		}

		for (int i = 0; i < 8; ++i) {
			if (BankStateChanged(fState.chr[i], state.chr[i])) {
				fCHRChangeTicks[i] = 12;
			}
		}
	} else {
		// A newly loaded mapper establishes the initial display state rather
		// than treating every row as a recent bank switch.
		for (int i = 0; i < 5; ++i) {
			fPRGChangeTicks[i] = 0;
		}

		for (int i = 0; i < 8; ++i) {
			fCHRChangeTicks[i] = 0;
		}
	}

	fMapper = mapper;
	fMapperName = mapperName.c_str();
	fState = state;
	fHaveState = true;

	return true;
}


void
MapperExplorerView::Draw (BRect updateRect)
{
	(void)updateRect;

	SetHighColor(0, 0, 0);

	if (!fHaveState) {
		DrawNoROMMessage();
		return;
	}

	DrawMapperSummary();
	DrawPRGTable();
	DrawCHRTable();

	DrawMMC1Panel();
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawNoROMMessage
//
// Draws the empty-state message shown when no cartridge mapper is loaded.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawNoROMMessage()
{
	SetFont(be_plain_font);
	SetHighColor(90, 90, 90);

	DrawString(
		"No ROM loaded.",
		BPoint(20.0f, 30.0f)
	);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMapperSummary
//
// Draws the mapper name, mirroring mode, and current debug-state revision.
//
// Revision is shown as secondary diagnostic information.  Its absolute value is
// not significant; it changes whenever the debugger-visible mapper state
// changes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMapperSummary()
{
	SetFont(be_bold_font);
	SetHighColor(0, 0, 0);

	DrawString("Mapper Explorer", BPoint(20.0f, 28.0f));

	SetFont(be_plain_font);

	BString text;

	text.SetToFormat(
		"Mapper: %s",
		fMapperName.String()
	);

	DrawString(text.String(), BPoint(20.0f, 56.0f));

	text.SetToFormat(
		"Mirroring: %s",
		MirroringName(fState.mirroring)
	);

	DrawString(text.String(), BPoint(20.0f, 78.0f));

	SetHighColor(110, 110, 110);

	text.SetToFormat(
		"Revision: %llu",
		static_cast<unsigned long long>(fState.revision)
	);

	DrawString(text.String(), BPoint(390.0f, 78.0f));
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawPRGTable
//
// Draws the five debugger-visible CPU PRG mapping slots.
//
// PRG bank numbers are expressed as resolved physical 8 KB units. Rows that
// changed recently are highlighted briefly.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawPRGTable()
{
	static const char *ranges[5] = {
		"$6000-$7FFF",
		"$8000-$9FFF",
		"$A000-$BFFF",
		"$C000-$DFFF",
		"$E000-$FFFF"
	};

	float y = 116.0f;

	SetFont(be_bold_font);
	SetHighColor(0, 0, 0);

	DrawString("CPU / PRG", BPoint(20.0f, y));

	y += 28.0f;

	DrawTableHeader(y, "Resolved 8K Bank");

	y += 27.0f;

	for (int i = 0; i < 5; ++i) {
		DrawBankRow(
			y,
			ranges[i],
			fState.prg[i],
			0x2000,
			fPRGChangeTicks[i] > 0
		);

		y += 22.0f;
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawCHRTable
//
// Draws the eight debugger-visible PPU CHR mapping slots.
//
// CHR bank numbers are expressed as resolved physical 1 KB units. Rows that
// changed recently are highlighted briefly.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawCHRTable()
{
	static const char *ranges[8] = {
		"$0000-$03FF",
		"$0400-$07FF",
		"$0800-$0BFF",
		"$0C00-$0FFF",
		"$1000-$13FF",
		"$1400-$17FF",
		"$1800-$1BFF",
		"$1C00-$1FFF"
	};

	float y = 302.0f;

	SetFont(be_bold_font);
	SetHighColor(0, 0, 0);

	DrawString("PPU / CHR", BPoint(20.0f, y));

	y += 28.0f;

	DrawTableHeader(y, "Resolved 1K Bank");

	y += 27.0f;

	for (int i = 0; i < 8; ++i) {
		DrawBankRow(
			y,
			ranges[i],
			fState.chr[i],
			0x0400,
			fCHRChangeTicks[i] > 0
		);

		y += 22.0f;
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawTableHeader
//
// Draws a mapping-table column header.
//
// Parameters:
//   y           - Baseline for the header text.
//   bankHeading - Label describing the resolved bank unit.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawTableHeader (float y, const char *bankHeading)
{
	const float left = 20.0f;
	const float right = Bounds().right - 20.0f;

	SetHighColor(218, 218, 218);
	FillRect(BRect(left, y - 15.0f, right, y + 5.0f));

	SetFont(be_bold_font);
	SetHighColor(45, 45, 45);

	DrawString("Range", BPoint(28.0f, y));
	DrawString("Type", BPoint(155.0f, y));
	DrawString(bankHeading, BPoint(250.0f, y));
	DrawString("Offset", BPoint(405.0f, y));
	DrawString("Access", BPoint(510.0f, y));

	SetHighColor(180, 180, 180);
	StrokeLine(
		BPoint(left, y + 6.0f),
		BPoint(right, y + 6.0f)
	);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawBankRow
//
// Draws one resolved mapper-memory row.
//
// The physical offset is derived from the resolved bank number and the bank-unit
// size used by the table. PRG rows use 8 KB units; CHR rows use 1 KB units.
//
// Recently changed rows receive a temporary highlight so live mapper activity
// is easier to see.
//
// Parameters:
//   y        - Baseline for the row text.
//   range    - CPU or PPU address range represented by the row.
//   bank     - Resolved debugger-visible mapper bank state.
//   bankSize - Size in bytes of one resolved bank unit.
//   changed  - true if this row should receive the recent-change highlight.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawBankRow (
	float y,
	const char *range,
	const mapper_debug_bank_t &bank,
	uint32_t bankSize,
	bool changed)
{
	const float left = 20.0f;
	const float right = Bounds().right - 20.0f;

	if (changed) {
		SetHighColor(255, 244, 190);
	} else {
		switch (bank.type) {
			case MapperDebugMemoryType::PRGRAM:
			case MapperDebugMemoryType::CHRRAM:
				SetHighColor(235, 246, 235);
				break;

			case MapperDebugMemoryType::Unmapped:
				SetHighColor(242, 242, 242);
				break;

			case MapperDebugMemoryType::PRGROM:
			case MapperDebugMemoryType::CHRROM:
			default:
				SetHighColor(248, 248, 248);
				break;
		}
	}

	FillRect(BRect(left, y - 14.0f, right, y + 6.0f));

	SetFont(be_fixed_font);

	if (bank.type == MapperDebugMemoryType::Unmapped) {
		SetHighColor(125, 125, 125);
	} else {
		SetHighColor(25, 25, 25);
	}

	DrawString(range, BPoint(28.0f, y));
	DrawString(MemoryTypeName(bank.type), BPoint(155.0f, y));

	BString bankText;
	BString offsetText;

	if (bank.type == MapperDebugMemoryType::Unmapped) {
		bankText = "-";
		offsetText = "-";
	} else {
		bankText.SetToFormat(
			"%u",
			static_cast<unsigned int>(bank.bank)
		);

		const uint32_t offset = bank.bank * bankSize;

		offsetText.SetToFormat(
			"$%06X",
			static_cast<unsigned int>(offset)
		);
	}

	DrawString(bankText.String(), BPoint(250.0f, y));
	DrawString(offsetText.String(), BPoint(405.0f, y));

	BString access;
	FormatAccess(bank, access);

	DrawString(access.String(), BPoint(510.0f, y));

	SetHighColor(225, 225, 225);
	StrokeLine(
		BPoint(left, y + 7.0f),
		BPoint(right, y + 7.0f)
	);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::BankStateChanged
//
// Determines whether one debugger-visible mapper bank entry changed.
//
// All fields that affect what the Mapper Explorer displays are compared.  This
// includes the resolved bank, memory type, access state, mapped address, and
// mapping size.
//
// Parameters:
//   oldBank - Previously captured mapper-bank state.
//   newBank - Newly captured mapper-bank state.
//
// Returns:
//   true if the visible bank state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::BankStateChanged (const mapper_debug_bank_t &oldBank,
									  const mapper_debug_bank_t &newBank) const
{
	return
		oldBank.address  != newBank.address ||
		oldBank.size     != newBank.size ||
		oldBank.bank     != newBank.bank ||
		oldBank.type     != newBank.type ||
		oldBank.readable != newBank.readable ||
		oldBank.writable != newBank.writable;
}



// -----------------------------------------------------------------------------
// MapperExplorerView::MemoryTypeName
//
// Converts a mapper debugger memory type into display text.
//
// Parameters:
//   type - Debugger-visible mapper memory type.
//
// Returns:
//   Static display string for the supplied type.
// -----------------------------------------------------------------------------
const char *
MapperExplorerView::MemoryTypeName (MapperDebugMemoryType type) const
{
	switch (type) {
		case MapperDebugMemoryType::PRGROM:
			return "PRG ROM";

		case MapperDebugMemoryType::PRGRAM:
			return "PRG RAM";

		case MapperDebugMemoryType::CHRROM:
			return "CHR ROM";

		case MapperDebugMemoryType::CHRRAM:
			return "CHR RAM";

		case MapperDebugMemoryType::Unmapped:
		default:
			return "Unmapped";
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MirroringName
//
// Converts the mapper debugger mirroring state into display text.
//
// Parameters:
//   mirroring - Debugger-visible mapper mirroring mode.
//
// Returns:
//   Static display string for the supplied mirroring mode.
// -----------------------------------------------------------------------------
const char *
MapperExplorerView::MirroringName (MapperDebugMirroring mirroring) const
{
	switch (mirroring) {
		case MapperDebugMirroring::SingleLow:
			return "Single Screen Low";

		case MapperDebugMirroring::SingleHigh:
			return "Single Screen High";

		case MapperDebugMirroring::Vertical:
			return "Vertical";

		case MapperDebugMirroring::Horizontal:
			return "Horizontal";

		case MapperDebugMirroring::FourScreen:
			return "Four Screen";

		case MapperDebugMirroring::Unknown:
		default:
			return "Unknown";
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::FormatAccess
//
// Formats debugger-visible read/write permissions for one mapped bank.
//
// Parameters:
//   bank - Mapper debug-bank entry.
//   text - Destination string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::FormatAccess (
	const mapper_debug_bank_t &bank,
	BString &text) const
{
	if (bank.readable && bank.writable) {
		text = "R/W";
		return;
	}

	if (bank.readable) {
		text = "R/O";
		return;
	}

	if (bank.writable) {
		text = "W/O";
		return;
	}

	text = "-";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureMMC1State
//
// Captures MMC1-specific internal debugger state when the current cartridge uses
// Mapper 1.
//
// MMC1 state is sampled independently from the generic mapper revision because
// partial serial writes can modify the shift register without changing the
// resolved PRG/CHR mappings.
//
// Parameters:
//   None.
//
// Returns:
//   true if the cached MMC1 state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureMMC1State()
{
	Mapper *mapper = nes::cart.mapper();

	Mapper1 *mmc1 = dynamic_cast<Mapper1 *>(mapper);

	if (!mmc1) {
		if (!fHaveMMC1State) {
			return false;
		}

		fMMC1State = {};
		fHaveMMC1State = false;

		return true;
	}

	const mapper1_debug_state_t state = mmc1->debug_state_mmc1();

	if (fHaveMMC1State && !MMC1StateChanged(fMMC1State, state)) {
		return false;
	}

	fMMC1State = state;
	fHaveMMC1State = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC1StateChanged
//
// Determines whether any MMC1-specific debugger state changed.
//
// This comparison includes persistent serial-transfer history so completed
// transfers remain visible even when they occur entirely between UI pulses.
//
// Parameters:
//   oldState - Previously captured MMC1 state.
//   newState - Newly captured MMC1 state.
//
// Returns:
//   true if any displayed MMC1 state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::MMC1StateChanged (
	const mapper1_debug_state_t &oldState,
	const mapper1_debug_state_t &newState) const
{
	if (
		oldState.shift_register        != newState.shift_register ||
		oldState.shift_count           != newState.shift_count ||
		oldState.control               != newState.control ||
		oldState.chr_bank_0            != newState.chr_bank_0 ||
		oldState.chr_bank_1            != newState.chr_bank_1 ||
		oldState.prg_bank              != newState.prg_bank ||
		oldState.prg_mode              != newState.prg_mode ||
		oldState.chr_mode              != newState.chr_mode ||
		oldState.prg_ram_enabled       != newState.prg_ram_enabled ||
		oldState.serial_write_count    != newState.serial_write_count ||
		oldState.register_commit_count != newState.register_commit_count ||
		oldState.reset_count           != newState.reset_count ||
		oldState.have_last_commit      != newState.have_last_commit ||
		oldState.last_register         != newState.last_register ||
		oldState.last_value            != newState.last_value ||
		oldState.have_last_transfer    != newState.have_last_transfer
	) {
		return true;
	}

	for (int i = 0; i < 5; ++i) {
		if (oldState.last_transfer[i] != newState.last_transfer[i]) {
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC1PRGModeName
//
// Returns a readable description of an MMC1 PRG banking mode.
//
// Parameters:
//   mode - Decoded MMC1 Control-register PRG mode.
//
// Returns:
//   Static display string describing the mode.
// -----------------------------------------------------------------------------
const char *
MapperExplorerView::MMC1PRGModeName (uint8_t mode) const
{
	switch (mode & 0x03) {
		case 0:
		case 1:
			return "32 KB";

		case 2:
			return "Fixed $8000 / switch $C000";

		case 3:
			return "Switch $8000 / fixed $C000";
	}

	return "Unknown";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC1CHRModeName
//
// Returns a readable description of the MMC1 CHR banking mode.
//
// Parameters:
//   mode - Decoded MMC1 Control-register CHR mode.
//
// Returns:
//   Static display string describing the mode.
// -----------------------------------------------------------------------------
const char *
MapperExplorerView::MMC1CHRModeName (uint8_t mode) const
{
	return (mode & 0x01) ? "4 KB" : "8 KB";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC1RegisterName
//
// Returns the display name of one MMC1 internal register.
//
// Parameters:
//   reg - MMC1 register index, 0 through 3.
//
// Returns:
//   Static display string for the supplied register.
// -----------------------------------------------------------------------------
const char *
MapperExplorerView::MMC1RegisterName (uint8_t reg) const
{
	switch (reg) {
		case 0:
			return "Control";

		case 1:
			return "CHR Bank 0";

		case 2:
			return "CHR Bank 1";

		case 3:
			return "PRG Bank";
	}

	return "Unknown";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMMC1Panel
//
// Draws MMC1-specific internal register and serial-interface state.
//
// The most recently completed five-write serial transfer is preserved and shown
// as a sequence of intermediate latch values. This makes MMC1 serial activity
// visible even though a complete transfer normally occurs much faster than the
// debugger UI can sample it.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMMC1Panel()
{
	if (!fHaveMMC1State) {
		return;
	}

	float y = 550.0f;

	SetFont(be_bold_font);
	SetHighColor(0, 0, 0);

	DrawString("MMC1 Internal State", BPoint(20.0f, y));

	SetFont(be_fixed_font);

	BString text;

	y += 26.0f;

	text.SetToFormat(
		"Control: $%02X     CHR0: $%02X     CHR1: $%02X     PRG: $%02X",
		static_cast<unsigned int>(fMMC1State.control),
		static_cast<unsigned int>(fMMC1State.chr_bank_0),
		static_cast<unsigned int>(fMMC1State.chr_bank_1),
		static_cast<unsigned int>(fMMC1State.prg_bank)
	);

	DrawString(text.String(), BPoint(28.0f, y));

	y += 22.0f;

	text.SetToFormat(
		"PRG Mode: %u - %s",
		static_cast<unsigned int>(fMMC1State.prg_mode),
		MMC1PRGModeName(fMMC1State.prg_mode)
	);

	DrawString(text.String(), BPoint(28.0f, y));

	y += 22.0f;

	text.SetToFormat(
		"CHR Mode: %u - %s",
		static_cast<unsigned int>(fMMC1State.chr_mode),
		MMC1CHRModeName(fMMC1State.chr_mode)
	);

	DrawString(text.String(), BPoint(28.0f, y));

	y += 22.0f;

	text.SetToFormat(
		"PRG RAM: %s",
		fMMC1State.prg_ram_enabled ? "Enabled" : "Disabled"
	);

	DrawString(text.String(), BPoint(28.0f, y));

	y += 30.0f;

	SetFont(be_bold_font);

	DrawString("Last Serial Transfer", BPoint(20.0f, y));

	SetFont(be_fixed_font);

	y += 24.0f;

	if (fMMC1State.have_last_transfer) {
		text.SetToFormat(
			"1/5: $%02X   2/5: $%02X   3/5: $%02X   4/5: $%02X   5/5: $%02X",
			static_cast<unsigned int>(fMMC1State.last_transfer[0]),
			static_cast<unsigned int>(fMMC1State.last_transfer[1]),
			static_cast<unsigned int>(fMMC1State.last_transfer[2]),
			static_cast<unsigned int>(fMMC1State.last_transfer[3]),
			static_cast<unsigned int>(fMMC1State.last_transfer[4])
		);
	} else {
		text = "No completed serial transfer captured.";
	}

	DrawString(text.String(), BPoint(28.0f, y));

	y += 24.0f;

	if (fMMC1State.have_last_commit) {
		text.SetToFormat(
			"Last Commit: %s = $%02X",
			MMC1RegisterName(fMMC1State.last_register),
			static_cast<unsigned int>(fMMC1State.last_value)
		);
	} else {
		text = "Last Commit: None";
	}

	DrawString(text.String(), BPoint(28.0f, y));

	y += 30.0f;

	SetFont(be_bold_font);

	DrawString("Serial Activity", BPoint(20.0f, y));

	SetFont(be_fixed_font);

	y += 24.0f;

	text.SetToFormat(
		"Writes: %llu     Commits: %llu     Resets: %llu",
		static_cast<unsigned long long>(
			fMMC1State.serial_write_count
		),
		static_cast<unsigned long long>(
			fMMC1State.register_commit_count
		),
		static_cast<unsigned long long>(
			fMMC1State.reset_count
		)
	);

	DrawString(text.String(), BPoint(28.0f, y));
}

