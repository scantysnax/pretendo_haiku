

#include "MapperExplorerView.h"
#include "PretendoWindow.h"


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
	: BView(frame, "mapper_explorer_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fParent = parent;

	SetViewColor(216, 216, 216);
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


// -----------------------------------------------------------------------------
// MapperExplorerView::AttachedToWindow
//
// Initializes the Mapper Explorer after the view has been attached to its
// window.
//
// The initial generic mapper state and any supported mapper-specific debugger
// state are captured before the first redraw.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::AttachedToWindow()
{
	BView::AttachedToWindow();

	CaptureState();
	CaptureMMC1State();
	CaptureUxROMState();
	CaptureCNROMState();
	CaptureMMC3State();
	CaptureMMC5State();
	CaptureAxROMState();
	CaptureMMC2State();
	
	
	Invalidate();
}


// -----------------------------------------------------------------------------
// MapperExplorerView::Draw
//
// Draws the complete Mapper Explorer view.
//
// The generic mapper summary and resolved CPU/PRG and PPU/CHR bank maps are
// always drawn when a ROM is loaded. Mapper-specific diagnostic panels are then
// drawn when the active mapper exposes additional internal debugger state.
//
// Parameters:
//   updateRect - Region of the view that needs to be redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

	if (dynamic_cast<Mapper0 *>(fMapper) != nullptr) {
		DrawMapperSpecificSeparator();
		DrawNROMPanel();
	} else if (HaveMapperSpecificPanel()) {
		DrawMapperSpecificSeparator();
		
		DrawMMC1Panel();
		DrawUxROMPanel();
		DrawCNROMPanel();
		DrawMMC3Panel();
		DrawMMC5Panel();
		DrawAxROMPanel();
		DrawMMC2Panel();
	} else if (MapperSpecificPanelExpected()) {
		DrawMapperSpecificSeparator();
		DrawUnsupportedMapperPanel();
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::Pulse
//
// Updates the live Mapper Explorer state.
//
// The generic mapper snapshot and any supported mapper-specific debugger state
// are refreshed. Recent PRG/CHR mapping-change highlight timers are also
// advanced.
//
// The view is invalidated only when displayed state changes or a temporary
// change highlight needs to be updated.
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
	
	if (CaptureUxROMState()) {
		redraw = true;
	}
	
	if (CaptureCNROMState()) {
		redraw = true;
	}

	if (CaptureMMC3State()) {
		redraw = true;
	}
	
	if (CaptureMMC5State()) {
		redraw = true;
	}
	
	if (CaptureAxROMState()) {
		redraw = true;
	}

	if (CaptureMMC2State()) {
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
// MapperExplorerView::DrawText
//
// Draws text using the supplied font and returns the horizontal position
// immediately following the rendered string.
//
// This helper makes it easier to combine plain labels with fixed-width
// hexadecimal or binary values on the same line.
//
// Parameters:
//   x    - Horizontal drawing position.
//   y    - Text baseline.
//   text - String to draw.
//   font - Font used to render the string.
//
// Returns:
//   Horizontal position immediately following the rendered string.
// -----------------------------------------------------------------------------
float
MapperExplorerView::DrawText (float x, float y, const char *text, const BFont *font)
{
	SetFont(font);
	DrawString(text, BPoint(x, y));

	return x + StringWidth(text);
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

	DrawString("No ROM loaded.", BPoint(20.0f, 30.0f));
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
	const bool revisionChanged = !fHaveState || state.revision != fState.revision;
	const bool nameChanged = fMapperName != mapperName.c_str();

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
MapperExplorerView::BankStateChanged (const mapper_debug_bank_t &oldBank, const mapper_debug_bank_t &newBank) const
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

	const mmc1_debug_state_t state = mmc1->debug_state();

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
MapperExplorerView::MMC1StateChanged (const mmc1_debug_state_t &oldState, 
									  const mmc1_debug_state_t &newState) const
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
		oldState.have_last_transfer    != newState.have_last_transfer) {
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
// MapperExplorerView::CaptureUxROMState
//
// Captures UxROM-specific debugger state from the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   true if the UxROM state changed or UxROM availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureUxROMState()
{
	Mapper *mapper = nes::cart.mapper();
	Mapper2 *uxrom = dynamic_cast<Mapper2 *>(mapper);

	if (!uxrom) {
		if (!fHaveUxROMState) {
			return false;
		}

		fUxROMState = {};
		fHaveUxROMState = false;

		return true;
	}

	const uxrom_debug_state_t state = uxrom->debug_state();

	if (fHaveUxROMState && !UxROMStateChanged(fUxROMState, state)) {
		return false;
	}

	fUxROMState = state;
	fHaveUxROMState = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::UxROMStateChanged
//
// Determines whether any UxROM-specific debugger state changed.
//
// Parameters:
//   oldState - Previously captured UxROM state.
//   newState - Newly captured UxROM state.
//
// Returns:
//   true if any displayed UxROM state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::UxROMStateChanged (const uxrom_debug_state_t &oldState,
									   const uxrom_debug_state_t &newState) const
{
	return
		oldState.bank_select        != newState.bank_select ||
		oldState.prg_bank           != newState.prg_bank ||
		oldState.write_count        != newState.write_count ||
		oldState.have_last_write    != newState.have_last_write ||
		oldState.last_write_address != newState.last_write_address ||
		oldState.last_write_value   != newState.last_write_value;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureCNROMState
//
// Captures CNROM-specific debugger state from the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   true if the CNROM state changed or CNROM availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureCNROMState()
{
	Mapper *mapper = nes::cart.mapper();
	Mapper3 *cnrom = dynamic_cast<Mapper3 *>(mapper);

	if (!cnrom) {
		if (!fHaveCNROMState) {
			return false;
		}

		fCNROMState = {};
		fHaveCNROMState = false;

		return true;
	}

	const cnrom_debug_state_t state = cnrom->debug_state();

	if (fHaveCNROMState && !CNROMStateChanged(fCNROMState, state)) {
		return false;
	}

	fCNROMState = state;
	fHaveCNROMState = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CNROMStateChanged
//
// Determines whether any CNROM-specific debugger state changed.
//
// Parameters:
//   oldState - Previously captured CNROM state.
//   newState - Newly captured CNROM state.
//
// Returns:
//   true if any displayed CNROM state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CNROMStateChanged (const cnrom_debug_state_t &oldState,
									   const cnrom_debug_state_t &newState) const
{
	return
		oldState.bank_select        != newState.bank_select ||
		oldState.resolved_chr_bank  != newState.resolved_chr_bank ||
		oldState.write_count        != newState.write_count ||
		oldState.have_last_write    != newState.have_last_write ||
		oldState.last_write_address != newState.last_write_address ||
		oldState.last_write_value   != newState.last_write_value;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureMMC3State
//
// Captures MMC3-specific debugger state from the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   true if the MMC3 state changed or MMC3 availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureMMC3State()
{
	Mapper *mapper = nes::cart.mapper();
	MMC3 *mmc3 = dynamic_cast<MMC3 *>(mapper);

	if (!mmc3) {
		if (!fHaveMMC3State) {
			return false;
		}

		fMMC3State = {};
		fHaveMMC3State = false;

		return true;
	}

	const mmc3_debug_state_t state = mmc3->debug_state();

	if (fHaveMMC3State && !MMC3StateChanged(fMMC3State, state)) {
		return false;
	}

	fMMC3State = state;
	fHaveMMC3State = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC3StateChanged
//
// Determines whether any MMC3-specific debugger state changed.
//
// Parameters:
//   oldState - Previously captured MMC3 state.
//   newState - Newly captured MMC3 state.
//
// Returns:
//   true if any displayed MMC3 state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::MMC3StateChanged (const mmc3_debug_state_t &oldState, const mmc3_debug_state_t &newState) const
{
	if (
		oldState.command                  != newState.command ||
		oldState.selected_register        != newState.selected_register ||
		oldState.prg_mode                 != newState.prg_mode ||
		oldState.chr_mode                 != newState.chr_mode ||
		oldState.prg_ram_enabled          != newState.prg_ram_enabled ||
		oldState.prg_ram_writable         != newState.prg_ram_writable ||
		oldState.irq_latch                != newState.irq_latch ||
		oldState.irq_counter              != newState.irq_counter ||
		oldState.irq_reload               != newState.irq_reload ||
		oldState.irq_enabled              != newState.irq_enabled ||
		oldState.hardware_mode            != newState.hardware_mode ||
		oldState.a12_rising_edge_count    != newState.a12_rising_edge_count ||
		oldState.a12_qualified_edge_count != newState.a12_qualified_edge_count ||
		oldState.a12_rejected_edge_count  != newState.a12_rejected_edge_count ||
		oldState.irq_clock_count          != newState.irq_clock_count ||
		oldState.irq_assert_count         != newState.irq_assert_count ||
		oldState.last_a12_spacing         != newState.last_a12_spacing) {
		return true;
	}

	for (int i = 0; i < 2; ++i) {
		if (oldState.prg_bank[i] != newState.prg_bank[i]) {
			return true;
		}
	}

	for (int i = 0; i < 8; ++i) {
		if (oldState.chr_bank[i] != newState.chr_bank[i]) {
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureMMC5State
//
// Captures the current MMC5-specific debugger state.
//
// If the active mapper is not MMC5, any previously cached MMC5 state is
// discarded.
//
// Parameters:
//   None.
//
// Returns:
//   true if the cached MMC5 state or its availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureMMC5State()
{
	Mapper5 *mapper = dynamic_cast<Mapper5 *>(fMapper);

	if (mapper == nullptr) {
		if (!fHaveMMC5State) {
			return false;
		}

		fMMC5State = {};
		fHaveMMC5State = false;

		return true;
	}

	const mmc5_debug_state_t newState = mapper->debug_state();

	if (!fHaveMMC5State) {
		fMMC5State = newState;
		fHaveMMC5State = true;

		return true;
	}

	if (!MMC5StateChanged(fMMC5State, newState)) {
		return false;
	}

	fMMC5State = newState;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC5StateChanged
//
// Compares two MMC5 debugger-state snapshots.
//
// Parameters:
//   oldState - Previously captured MMC5 state.
//   newState - Newly captured MMC5 state.
//
// Returns:
//   true if any debugger-visible MMC5 state has changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::MMC5StateChanged (const mmc5_debug_state_t &oldState,
									 const mmc5_debug_state_t &newState) const
{
	if (
		oldState.prg_mode != newState.prg_mode ||
		oldState.chr_mode != newState.chr_mode ||
		oldState.bg_char_upper != newState.bg_char_upper ||
		oldState.last_chr_write_bg != newState.last_chr_write_bg ||
		oldState.prg_ram_protect1 != newState.prg_ram_protect1 ||
		oldState.prg_ram_protect2 != newState.prg_ram_protect2 ||
		oldState.mirroring_mode != newState.mirroring_mode ||
		oldState.exram_mode != newState.exram_mode ||
		oldState.fill_mode_tile != newState.fill_mode_tile ||
		oldState.fill_mode_attr != newState.fill_mode_attr ||
		oldState.vertical_split_mode != newState.vertical_split_mode ||
		oldState.vertical_split_scroll != newState.vertical_split_scroll ||
		oldState.vertical_split_bank != newState.vertical_split_bank ||
		oldState.large_sprites != newState.large_sprites ||
		oldState.fetch_count != newState.fetch_count ||
		oldState.irq_enabled != newState.irq_enabled ||
		oldState.irq_counter != newState.irq_counter ||
		oldState.irq_target != newState.irq_target ||
		oldState.irq_in_frame != newState.irq_in_frame ||
		oldState.irq_pending != newState.irq_pending ||
		oldState.multiplier_1 != newState.multiplier_1 ||
		oldState.multiplier_2 != newState.multiplier_2 ||
		oldState.multiplier_result != newState.multiplier_result
	) {
		return true;
	}

	for (int i = 0; i < 5; ++i) {
		if (oldState.prg_bank[i] != newState.prg_bank[i]) {
			return true;
		}
	}

	for (int i = 0; i < 8; ++i) {
		if (
			oldState.bg_chr_bank[i] != newState.bg_chr_bank[i] ||
			oldState.sp_chr_bank[i] != newState.sp_chr_bank[i]
		) {
			return true;
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureAxROMState
//
// Captures AxROM-specific debugger state from the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   true if the AxROM state changed or AxROM availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureAxROMState()
{
	Mapper *mapper = nes::cart.mapper();
	Mapper7 *axrom = dynamic_cast<Mapper7 *>(mapper);

	if (!axrom) {
		if (!fHaveAxROMState) {
			return false;
		}

		fAxROMState = {};
		fHaveAxROMState = false;

		return true;
	}

	const axrom_debug_state_t state = axrom->debug_state();

	if (fHaveAxROMState && !AxROMStateChanged(fAxROMState, state)) {
		return false;
	}

	fAxROMState = state;
	fHaveAxROMState = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::AxROMStateChanged
//
// Determines whether any AxROM-specific debugger state changed.
//
// Parameters:
//   oldState - Previously captured AxROM state.
//   newState - Newly captured AxROM state.
//
// Returns:
//   true if any displayed AxROM state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::AxROMStateChanged (const axrom_debug_state_t &oldState, 
									   const axrom_debug_state_t &newState) const
{
	return
		oldState.control                 != newState.control ||
		oldState.prg_bank                != newState.prg_bank ||
		oldState.single_screen_high      != newState.single_screen_high ||
		oldState.write_count             != newState.write_count ||
		oldState.have_last_write         != newState.have_last_write ||
		oldState.last_write_address      != newState.last_write_address ||
		oldState.last_write_value        != newState.last_write_value;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::CaptureMMC2State
//
// Captures MMC2-specific debugger state from the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   true if the MMC2 state changed or MMC2 availability changed.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::CaptureMMC2State()
{
	Mapper *mapper = nes::cart.mapper();
	Mapper9 *mmc2 = dynamic_cast<Mapper9 *>(mapper);

	if (!mmc2) {
		if (!fHaveMMC2State) {
			return false;
		}

		fMMC2State = {};
		fHaveMMC2State = false;

		return true;
	}

	const mmc2_debug_state_t state = mmc2->debug_state();

	if (fHaveMMC2State && !MMC2StateChanged(fMMC2State, state)) {
		return false;
	}

	fMMC2State = state;
	fHaveMMC2State = true;

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC2StateChanged
//
// Determines whether any MMC2-specific debugger state changed.
//
// Parameters:
//   oldState - Previously captured MMC2 state.
//   newState - Newly captured MMC2 state.
//
// Returns:
//   true if any displayed MMC2 state changed; false otherwise.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::MMC2StateChanged (const mmc2_debug_state_t &oldState, const mmc2_debug_state_t &newState) const
{
	return
		oldState.prg_bank             != newState.prg_bank ||
		oldState.latch0_lo            != newState.latch0_lo ||
		oldState.latch0_hi            != newState.latch0_hi ||
		oldState.latch1_lo            != newState.latch1_lo ||
		oldState.latch1_hi            != newState.latch1_hi ||
		oldState.latch0               != newState.latch0 ||
		oldState.latch1               != newState.latch1 ||
		oldState.active_chr0_bank     != newState.active_chr0_bank ||
		oldState.active_chr1_bank     != newState.active_chr1_bank ||
		oldState.latch0_low_count     != newState.latch0_low_count ||
		oldState.latch0_high_count    != newState.latch0_high_count ||
		oldState.latch1_low_count     != newState.latch1_low_count ||
		oldState.latch1_high_count    != newState.latch1_high_count ||
		oldState.have_last_trigger    != newState.have_last_trigger ||
		oldState.last_trigger_address != newState.last_trigger_address;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMapperSummary
//
// Draws the Mapper Explorer title and high-level mapper information.
//
// The mapper name and mirroring mode are treated as primary state, while the
// mapper revision counter is shown as secondary diagnostic information.
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
	BString text;

	// Main view title.
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Mapper Explorer", BPoint(20.0f, 28.0f));

	// Mapper name.
	SetHighColor(25, 25, 25);
	float x = 20.0f;
	x = DrawText(x, 56.0f, "Mapper: ", be_plain_font);
	DrawText(x, 56.0f, fMapperName.String(), be_plain_font);

	// Mirroring mode.
	x = 20.0f;
	x = DrawText(x, 78.0f, "Mirroring: ", be_plain_font);
	DrawText(x, 78.0f, MirroringName(fState.mirroring), be_plain_font);


	// Revision is useful for debugging but is intentionally de-emphasized.
	SetHighColor(100, 100, 100);
	x = 390.0f;
	x = DrawText(x, 78.0f, "Revision: ", be_plain_font);
	text.SetToFormat("%llu", static_cast<unsigned long long>(fState.revision));
	DrawText(x, 78.0f, text.String(), be_plain_font);
	
	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawPRGTable
//
// Draws the resolved CPU-side PRG mapping table.
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

	float y = 108.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("CPU / PRG", BPoint(20.0f, y));
	y += 24.0f;

	DrawTableHeader(y, "Resolved 8K Bank");
	y += 24.0f;

	for (int i = 0; i < 5; ++i) {
		DrawBankRow(y, ranges[i], fState.prg[i], 0x2000,fPRGChangeTicks[i] > 0);
		y += 19.0f;
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawCHRTable
//
// Draws the resolved PPU-side CHR mapping table.
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

	float y = 260.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("PPU / CHR", BPoint(20.0f, y));
	y += 24.0f;

	DrawTableHeader(y, "Resolved 1K Bank");
	y += 24.0f;

	for (int i = 0; i < 8; ++i) {
		DrawBankRow(y, ranges[i], fState.chr[i], 0x0400, fCHRChangeTicks[i] > 0);
		y += 19.0f;
	}
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawTableHeader
//
// Draws the column headings for a mapper bank table.
//
// Parameters:
//   y           - Baseline of the header text.
//   bankHeading - Heading used for the resolved-bank column.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawTableHeader (float y, const char *bankHeading)
{
	const float left = 20.0f;
	const float right = Bounds().right - 20.0f;

	SetHighColor(194, 194, 194);
	FillRect(BRect(left, y - 14.0f, right, y + 5.0f));

	SetFont(be_bold_font);
	SetHighColor(35, 35, 35);

	DrawString("Range", BPoint(28.0f, y));
	DrawString("Type", BPoint(155.0f, y));
	DrawString(bankHeading, BPoint(250.0f, y));
	DrawString("Offset", BPoint(405.0f, y));
	DrawString("Access", BPoint(510.0f, y));

	SetHighColor(175, 175, 175);
	StrokeLine(BPoint(left, y + 6.0f), BPoint(right, y + 6.0f));
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawBankRow
//
// Draws one resolved PRG or CHR mapping row.
//
// Row background colors identify the mapped memory type. Recently changed rows
// temporarily use a yellow highlight that takes precedence over the normal
// memory-type color.
//
// Hexadecimal address ranges and offsets use the fixed-width font. Descriptive
// text and decimal bank numbers use the plain font.
//
// Parameters:
//   y        - Baseline of the row text.
//   range    - CPU or PPU address range displayed for this row.
//   bank     - Resolved mapper bank state.
//   bankSize - Size of one displayed bank unit, in bytes.
//   changed  - true while the recent-change highlight is active.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawBankRow (float y, const char *range, const mapper_debug_bank_t &bank,
								 uint32 bankSize, bool changed)
{
	const float left = 20.0f;
	const float right = Bounds().right - 20.0f;

	if (changed) {
		SetHighColor(255, 238, 170);
	} else {
		switch (bank.type) {
			case MapperDebugMemoryType::PRGRAM:
			case MapperDebugMemoryType::CHRRAM:
				SetHighColor(226, 239, 226);
				break;

			case MapperDebugMemoryType::Unmapped:
				SetHighColor(204, 204, 204);
				break;

			case MapperDebugMemoryType::PRGROM:
			case MapperDebugMemoryType::CHRROM:
			default:
				SetHighColor(238, 238, 238);
				break;
		}
	}

	FillRect(BRect(left, y - 12.0f, right, y + 5.0f));

	if (bank.type == MapperDebugMemoryType::Unmapped) {
		SetHighColor(105, 105, 105);
	} else {
		SetHighColor(25, 25, 25);
	}

	// Hexadecimal address range.
	SetFont(be_fixed_font);
	DrawString(range, BPoint(28.0f, y));

	// Memory type is descriptive text.
	SetFont(be_plain_font);
	DrawString(MemoryTypeName(bank.type), BPoint(155.0f, y));

	BString bankText;
	BString offsetText;

	if (bank.type == MapperDebugMemoryType::Unmapped) {
		bankText = "-";
		offsetText = "-";
	} else {
		bankText.SetToFormat("%u", static_cast<unsigned int>(bank.bank));

		const uint32_t offset = bank.bank * bankSize;
		offsetText.SetToFormat("$%06X", static_cast<unsigned int>(offset));
	}

	// Resolved bank number is decimal.
	SetFont(be_plain_font);
	DrawString(bankText.String(), BPoint(250.0f, y));

	// Physical offset is hexadecimal.
	SetFont(be_fixed_font);
	DrawString(offsetText.String(), BPoint(405.0f, y));

	BString access;
	FormatAccess(bank, access);

	// Access description is ordinary text.
	SetFont(be_plain_font);
	DrawString(access.String(), BPoint(510.0f, y));

	SetHighColor(188, 188, 188);

	StrokeLine(BPoint(left, y + 6.0f), BPoint(right, y + 6.0f));
}


// -----------------------------------------------------------------------------
// MapperExplorerView::HaveMapperSpecificPanel
//
// Returns whether the active mapper exposes a mapper-specific diagnostics
// panel.
//
// Parameters:
//   None.
//
// Returns:
//   true if any mapper-specific state is currently available.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::HaveMapperSpecificPanel() const
{
	return
		fHaveMMC1State ||
		fHaveUxROMState ||
		fHaveCNROMState ||
		fHaveMMC3State ||
		fHaveMMC5State ||
		fHaveAxROMState ||
		fHaveMMC2State;		
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MapperSpecificPanelExpected
//
// Returns whether the active mapper would normally require additional
// mapper-specific diagnostics beyond the generic PRG/CHR mapping display.
//
// NROM is intentionally excluded because its useful mapper state is already
// completely represented by the generic Mapper Explorer tables.
//
// Parameters:
//   None.
//
// Returns:
//   true if the active mapper is not NROM and therefore may have additional
//   mapper-specific state to display.
// -----------------------------------------------------------------------------
bool
MapperExplorerView::MapperSpecificPanelExpected() const
{
	if (fMapper == nullptr) {
		return false;
	}

	if (dynamic_cast<Mapper0 *>(fMapper) != nullptr) {
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawUnsupportedMapperPanel
//
// Draws an informational message when the active mapper does not yet have a
// dedicated Mapper Explorer diagnostics panel.
//
// The generic PRG and CHR mapping tables remain valid and available above.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawUnsupportedMapperPanel()
{
	const float y = 490.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Mapper-Specific Diagnostics", BPoint(20.0f, y));

	SetFont(be_plain_font);
	SetHighColor(100, 100, 100);
	DrawString("Additional diagnostics are not available for this mapper yet.", BPoint(28.0f, y + 28.0f));
	DrawString("The generic PRG and CHR mappings above remain available.", BPoint(28.0f, y + 48.0f));

	SetHighColor(25, 25, 25);
}




// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMapperSpecificSeparator
//
// Draws a subtle horizontal separator between the generic mapper bank tables
// and the mapper-specific diagnostic area.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMapperSpecificSeparator()
{
	SetHighColor(180, 180, 180);
	StrokeLine(BPoint(20.0f, 456.0f), BPoint(Bounds().right - 20.0f, 456.0f));
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
const char*
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
const char*
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
MapperExplorerView::FormatAccess (const mapper_debug_bank_t &bank, BString &text) const
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
const char*
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
const char*
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
const char*
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
// MapperExplorerView::DrawNROMPanel
//
// Draws a short informational note for NROM cartridges.
//
// NROM has no mapper-specific banking hardware or runtime control state beyond
// the fixed PRG/CHR mappings already shown by the generic Mapper Explorer
// tables.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawNROMPanel()
{
	const float y = 490.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("NROM", BPoint(20.0f, y));

	SetFont(be_plain_font);
	SetHighColor(100, 100, 100);
	DrawString("No mapper-specific hardware state needs to be tracked.", BPoint(28.0f, y + 28.0f));
	DrawString("The fixed PRG and CHR mappings are shown above.", BPoint(28.0f, y + 48.0f));

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMMC1Panel
//
// Draws MMC1-specific register, banking-mode, serial-transfer, and mapper
// activity state.
//
// Section headings use a muted blue, primary state uses near-black, activity
// counters use secondary gray, enabled/disabled state is color coded, and
// hexadecimal register, address, and transfer values use the fixed-width font.
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

	float y = 478.0f;

	BString text;


	// -------------------------------------------------------------------------
	// Current MMC1 register state.
	// -------------------------------------------------------------------------

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("MMC1 Internal State", BPoint(20.0f, y));
	y += 26.0f;

	SetHighColor(25, 25, 25);
	float x = 28.0f;
	
	x = DrawText(x, y, "Control: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC1State.control));

	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "CHR0: ", be_plain_font);
	
	text.SetToFormat("$%02X",static_cast<unsigned int>(fMMC1State.chr_bank_0));
	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "CHR1: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC1State.chr_bank_1));
	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "PRG: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC1State.prg_bank));
	DrawText(x, y, text.String(), be_fixed_font);


	// PRG banking mode.

	y += 20.0f;
	x = 28.0f;

	x = DrawText(x, y, "PRG Mode: ", be_plain_font);
	text.SetToFormat("%u", static_cast<unsigned int>(fMMC1State.prg_mode));

	x = DrawText(x, y, text.String(), be_plain_font);
	x = DrawText(x, y, " - ", be_plain_font);

	switch (fMMC1State.prg_mode & 0x03) {
		case 0:
		case 1:
			DrawText(x, y, "32 KB", be_plain_font);
			break;

		case 2:
			x = DrawText(x, y, "Fixed ", be_plain_font);
			x = DrawText(x, y, "$8000", be_fixed_font);
			x = DrawText(x, y, " / switch ", be_plain_font);
			DrawText(x, y, "$C000", be_fixed_font);
			break;

		case 3:
			x = DrawText(x, y, "Switch ", be_plain_font);
			x = DrawText(x, y, "$8000", be_fixed_font);
			x = DrawText(x, y, " / fixed ", be_plain_font);
			DrawText(x, y, "$C000", be_fixed_font);
			break;
	}


	// CHR banking mode and PRG-RAM state.

	y += 20.0f;
	x = 28.0f;
	x = DrawText(x, y, "CHR Mode: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fMMC1State.chr_mode));
	x = DrawText(x, y, text.String(), be_plain_font);
	x = DrawText(x, y, " - ", be_plain_font);
	x = DrawText(x, y, MMC1CHRModeName(fMMC1State.chr_mode), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "PRG RAM: ", be_plain_font);

	if (fMMC1State.prg_ram_enabled) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	DrawText(x, y, fMMC1State.prg_ram_enabled ? "Enabled" : "Disabled", be_plain_font);

	SetHighColor(25, 25, 25);


	// -------------------------------------------------------------------------
	// Last completed serial transfer.
	// -------------------------------------------------------------------------

	y += 30.0f;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Last Serial Transfer", BPoint(20.0f, y));
	y += 24.0f;

	SetHighColor(25, 25, 25);

	if (fMMC1State.have_last_transfer) {
		x = 28.0f;

		for (int i = 0; i < 5; ++i) {
			text.SetToFormat("%d/5: ", i + 1);
			x = DrawText(x, y, text.String(), be_plain_font);

			text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC1State.last_transfer[i]));
			x = DrawText(x, y, text.String(), be_fixed_font);

			if (i < 4) {
				x += 14.0f;
			}
		}
	} else {
		SetHighColor(100, 100, 100);
		DrawText(28.0f, y, "No completed serial transfer captured.", be_plain_font);
		SetHighColor(25, 25, 25);
	}


	y += 20.0f;
	x = 28.0f;
	x = DrawText(x, y, "Last Commit: ", be_plain_font);

	if (fMMC1State.have_last_commit) {
		x = DrawText(x, y, MMC1RegisterName(fMMC1State.last_register), be_plain_font);
		x = DrawText(x, y, " = ", be_plain_font);
		text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC1State.last_value));
		DrawText(x, y, text.String(), be_fixed_font);
	} else {
		SetHighColor(100, 100, 100);
		DrawText(x, y, "None", be_plain_font);
		SetHighColor(25, 25, 25);
	}


	// -------------------------------------------------------------------------
	// Persistent serial-interface activity.
	// -------------------------------------------------------------------------

	y += 30.0f;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Serial Activity", BPoint(20.0f, y));
	y += 24.0f;
	SetHighColor(100, 100, 100);
	x = 28.0f;
	x = DrawText(x, y, "Writes: ", be_plain_font);
	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC1State.serial_write_count));

	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Commits: ", be_plain_font);
	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC1State.register_commit_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Resets: ", be_plain_font);
	text.SetToFormat("%llu",static_cast<unsigned long long>(fMMC1State.reset_count));
	DrawText(x, y, text.String(), be_plain_font);

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC3HardwareModeName
//
// Returns the display name of the active MMC3 hardware revision.
//
// Parameters:
//   mode - Debugger hardware-mode value.
//
// Returns:
//   Static display string describing the MMC3 revision.
// -----------------------------------------------------------------------------
const char*
MapperExplorerView::MMC3HardwareModeName (uint8_t mode) const
{
	switch (mode) {
		case 0:
			return "MMC3A";

		case 1:
			return "MMC3B";

		case 2:
			return "MMC6";
	}

	return "Unknown";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMMC3Panel
//
// Draws MMC3-specific bank-control, PRG-RAM, IRQ, and PPU A12 state.
//
// Mapper state is divided into two columns. Section headings use muted blue,
// primary state uses near-black, activity counters use secondary gray, and
// enabled/disabled states receive restrained status coloring.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMMC3Panel()
{
	if (!fHaveMMC3State) {
		return;
	}

	const float left = 20.0f;
	const float leftText = 28.0f;

	const float right = 325.0f;
	const float rightText = 333.0f;

	float leftY = 478.0f;
	float rightY = 478.0f;

	BString text;
	float x;


	// -------------------------------------------------------------------------
	// Left column: mapper and bank-register state.
	// -------------------------------------------------------------------------

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("MMC3 Internal State", BPoint(left, leftY));
	leftY += 26.0f;
	SetHighColor(25, 25, 25);
	
	x = leftText;
	x = DrawText(x, leftY, "Hardware: ", be_plain_font);
	DrawText(x, leftY, MMC3HardwareModeName(fMMC3State.hardware_mode), be_plain_font);
	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "Command: ", be_plain_font);
	
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.command));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, leftY, "Selected: ", be_plain_font);

	text.SetToFormat("R%u", static_cast<unsigned int>(fMMC3State.selected_register));
	DrawText(x, leftY, text.String(), be_plain_font);

	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "PRG Mode: ", be_plain_font);

	text.SetToFormat("%u", fMMC3State.prg_mode ? 1 : 0);
	x = DrawText(x, leftY, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, leftY, "CHR Mode: ", be_plain_font);
	
	text.SetToFormat("%u", fMMC3State.chr_mode ? 1 : 0);
	DrawText(x, leftY, text.String(), be_plain_font);
	leftY += 28.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Bank Registers", BPoint(left, leftY));
	leftY += 23.0f;

	SetHighColor(25, 25, 25);
	x = leftText;
	x = DrawText(x, leftY, "PRG R6: ", be_plain_font);
	
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.prg_bank[0]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, leftY, "R7: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.prg_bank[1]));
	DrawText(x, leftY, text.String(), be_fixed_font);
	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "CHR R0: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[0]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, leftY, "R1: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[2]));
	DrawText(x, leftY, text.String(), be_fixed_font);
	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "CHR R2: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[4]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, leftY, "R3: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[5]));
	DrawText(x, leftY, text.String(), be_fixed_font);
	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "CHR R4: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[6]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, leftY, "R5: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.chr_bank[7]));
	DrawText(x, leftY, text.String(), be_fixed_font);
	leftY += 28.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("PRG RAM", BPoint(left, leftY));
	leftY += 23.0f;

	SetHighColor(25, 25, 25);
	x = leftText;
	x = DrawText(x, leftY, "Enabled: ", be_plain_font);

	if (fMMC3State.prg_ram_enabled) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	x = DrawText(x, leftY, fMMC3State.prg_ram_enabled ? "Yes" : "No", be_plain_font);
	SetHighColor(25, 25, 25);
	x += 16.0f;
	x = DrawText(x, leftY, "Writable: ", be_plain_font);

	if (fMMC3State.prg_ram_enabled && fMMC3State.prg_ram_writable) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	DrawText(x, leftY, fMMC3State.prg_ram_writable ? "Yes" : "No", be_plain_font);
	SetHighColor(25, 25, 25);


	// -------------------------------------------------------------------------
	// Right column: IRQ and A12 activity.
	// -------------------------------------------------------------------------

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("IRQ", BPoint(right, rightY));
	rightY += 26.0f;

	SetHighColor(25, 25, 25);
	x = rightText;
	x = DrawText(x, rightY, "Latch: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.irq_latch));

	x = DrawText(x, rightY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, rightY, "Counter: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC3State.irq_counter));
	DrawText(x, rightY, text.String(), be_fixed_font);
	rightY += 20.0f;
	x = rightText;

	x = DrawText(x, rightY, "Reload: ", be_plain_font);
	DrawText(x, rightY, fMMC3State.irq_reload ? "Yes" : "No", be_plain_font);
	x += StringWidth(fMMC3State.irq_reload ? "Yes" : "No") + 16.0f;
	
	x = DrawText(x, rightY, "Enabled: ", be_plain_font);

	if (fMMC3State.irq_enabled) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	DrawText(x, rightY, fMMC3State.irq_enabled ? "Yes" : "No", be_plain_font);
	SetHighColor(25, 25, 25);
	
	rightY += 20.0f;
	SetHighColor(100, 100, 100);
	x = rightText;
	x = DrawText(x, rightY, "Clocks: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.irq_clock_count));
	DrawText(x, rightY, text.String(), be_plain_font);
	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Assertions: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.irq_assert_count));
	DrawText(x, rightY, text.String(), be_plain_font);
	rightY += 28.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("PPU A12 Activity", BPoint(right, rightY));
	rightY += 23.0f;
	SetHighColor(100, 100, 100);
	x = rightText;
	x = DrawText(x, rightY, "Rising: ", be_plain_font);
	
	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.a12_rising_edge_count));
	DrawText(x, rightY, text.String(), be_plain_font);
	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Qualified: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.a12_qualified_edge_count));
	DrawText(x, rightY, text.String(), be_plain_font);
	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Rejected: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.a12_rejected_edge_count));
	DrawText(x, rightY, text.String(), be_plain_font);
	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Last Spacing: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC3State.last_a12_spacing));
	x = DrawText(x, rightY, text.String(), be_plain_font);
	DrawText(x, rightY, " PPU cycles", be_plain_font);

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::MMC2LatchName
//
// Returns the display name of an MMC2 CHR latch state.
//
// Parameters:
//   latch - Current latch state.
//
// Returns:
//   "High" when the high CHR bank is selected, otherwise "Low".
// -----------------------------------------------------------------------------
const char*
MapperExplorerView::MMC2LatchName (bool latch) const
{
	return latch ? "High" : "Low";
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMMC2Panel
//
// Draws MMC2-specific PRG-bank, CHR-latch, and latch-trigger state.
//
// Section headings use muted blue, primary latch state uses near-black, and
// persistent latch-activity diagnostics use secondary gray.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMMC2Panel()
{
	if (!fHaveMMC2State) {
		return;
	}

	const float left = 20.0f;
	const float leftText = 28.0f;

	const float right = 325.0f;
	const float rightText = 333.0f;

	float y = 478.0f;

	BString text;


	// General MMC2 state.

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("MMC2 Internal State", BPoint(left, y));
	y += 26.0f;

	SetHighColor(25, 25, 25);
	float x = leftText;
	x = DrawText(x, y, "PRG Bank: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.prg_bank));
	DrawText(x, y, text.String(), be_fixed_font);
	y += 30.0f;


	// Left column: latch 0.

	float leftY = y;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("CHR $0000-$0FFF", BPoint(left, leftY));

	leftY += 24.0f;
	SetHighColor(25, 25, 25);
	x = leftText;
	x = DrawText(x, leftY, "Latch: ", be_plain_font);
	DrawText(x, leftY, MMC2LatchName(fMMC2State.latch0), be_plain_font);

	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "Low Bank: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.latch0_lo));
	DrawText(x, leftY, text.String(), be_fixed_font);

	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "High Bank: ", be_plain_font);
	
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.latch0_hi));
	DrawText(x, leftY, text.String(), be_fixed_font);
	
	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "Active: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.active_chr0_bank));
	DrawText(x, leftY, text.String(), be_fixed_font);


	// Right column: latch 1.

	float rightY = y;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("CHR $1000-$1FFF", BPoint(right, rightY));
	rightY += 24.0f;

	SetHighColor(25, 25, 25);

	x = rightText;
	x = DrawText(x, rightY, "Latch: ", be_plain_font);
	DrawText(x, rightY, MMC2LatchName(fMMC2State.latch1), be_plain_font);
	rightY += 20.0f;
	x = rightText;

	x = DrawText(x, rightY, "Low Bank: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.latch1_lo));
	DrawText(x, rightY, text.String(), be_fixed_font);
	rightY += 20.0f;
	x = rightText;

	x = DrawText(x, rightY, "High Bank: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.latch1_hi));
	DrawText(x, rightY, text.String(), be_fixed_font);
	rightY += 20.0f;
	x = rightText;

	x = DrawText(x, rightY, "Active: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC2State.active_chr1_bank));
	DrawText(x, rightY, text.String(), be_fixed_font);


	// Persistent latch activity.

	y = std::max(leftY, rightY) + 30.0f;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Latch Activity", BPoint(left, y));
	y += 24.0f;

	SetHighColor(100, 100, 100);
	x = leftText;
	x = DrawText(x, y, "Latch 0 Low: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC2State.latch0_low_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "High: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC2State.latch0_high_count));
	DrawText(x, y, text.String(), be_plain_font);
	y += 20.0f;
	x = leftText;
	x = DrawText(x, y, "Latch 1 Low: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC2State.latch1_low_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;

	x = DrawText(x, y, "High: ", be_plain_font);
	text.SetToFormat("%llu", static_cast<unsigned long long>(fMMC2State.latch1_high_count));
	DrawText(x, y, text.String(), be_plain_font);
	y += 20.0f;
	x = leftText;
	x = DrawText(x, y, "Last Trigger: ", be_plain_font);

	if (fMMC2State.have_last_trigger) {
		text.SetToFormat("$%04X", static_cast<unsigned int>(fMMC2State.last_trigger_address));
		DrawText(x, y, text.String(), be_fixed_font);
	} else {
		DrawText(x, y, "None", be_plain_font);
	}

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawAxROMPanel
//
// Draws AxROM-specific mapper-control and bank-selection state.
//
// Section headings use muted blue, primary mapper state uses near-black,
// enabled state uses restrained green, and activity diagnostics use secondary
// gray.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawAxROMPanel()
{
	if (!fHaveAxROMState) {
		return;
	}

	float y = 478.0f;

	BString text;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("AxROM Internal State", BPoint(20.0f, y));
	y += 26.0f;

	SetHighColor(25, 25, 25);
	float x = 28.0f;
	x = DrawText(x, y, "Control: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fAxROMState.control));
	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "PRG Bank: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fAxROMState.prg_bank));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Single-Screen: ", be_plain_font);
	DrawText(x, y, fAxROMState.single_screen_high ? "High" : "Low", be_plain_font);
	y += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Mapper Activity", BPoint(20.0f, y));
	y += 24.0f;

	SetHighColor(100, 100, 100);
	x = 28.0f;
	x = DrawText(x, y, "Writes: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fAxROMState.write_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Last Write: ", be_plain_font);

	if (fAxROMState.have_last_write) {
		text.SetToFormat("$%04X", static_cast<unsigned int>(fAxROMState.last_write_address));
		x = DrawText(x, y, text.String(), be_fixed_font);
		x = DrawText(x, y, " = ", be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fAxROMState.last_write_value));
		DrawText(x, y, text.String(), be_fixed_font);
	} else {
		DrawText(x, y, "None", be_plain_font);
	}

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawUxROMPanel
//
// Draws UxROM-specific PRG-bank selection and mapper-write state.
//
// Section headings use muted blue, primary mapper state uses near-black, and
// persistent write diagnostics use secondary gray.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawUxROMPanel()
{
	if (!fHaveUxROMState) {
		return;
	}

	float y = 478.0f;

	BString text;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("UxROM Internal State", BPoint(20.0f, y));
	y += 26.0f;

	SetHighColor(25, 25, 25);
	float x = 28.0f;
	x = DrawText(x, y, "Bank Select: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fUxROMState.bank_select));
	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "PRG Bank: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fUxROMState.prg_bank));
	DrawText(x, y, text.String(), be_plain_font);
	y += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Mapper Activity", BPoint(20.0f, y));
	y += 24.0f;

	SetHighColor(100, 100, 100);
	x = 28.0f;
	x = DrawText(x, y, "Writes: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fUxROMState.write_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Last Write: ", be_plain_font);

	if (fUxROMState.have_last_write) {
		text.SetToFormat("$%04X", static_cast<unsigned int>(fUxROMState.last_write_address));
		x = DrawText(x, y, text.String(), be_fixed_font);
		x = DrawText(x, y, " = ", be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fUxROMState.last_write_value));
		DrawText(x, y, text.String(), be_fixed_font);
	} else {
		DrawText(x, y, "None", be_plain_font);
	}

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawCNROMPanel
//
// Draws CNROM-specific CHR-bank selection and mapper-write state.
//
// Section headings use muted blue, primary mapper state uses near-black, and
// persistent write diagnostics use secondary gray.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawCNROMPanel()
{
	if (!fHaveCNROMState) {
		return;
	}

	float y = 478.0f;
	BString text;
	
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("CNROM Internal State", BPoint(20.0f, y));
	y += 26.0f;

	SetHighColor(25, 25, 25);
	float x = 28.0f;
	x = DrawText(x, y, "Bank Select: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fCNROMState.bank_select));
	x = DrawText(x, y, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, y, "Resolved CHR Bank: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fCNROMState.resolved_chr_bank));
	DrawText(x, y, text.String(), be_plain_font);
	y += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Mapper Activity", BPoint(20.0f, y));
	y += 24.0f;

	SetHighColor(100, 100, 100);
	x = 28.0f;
	x = DrawText(x, y, "Writes: ", be_plain_font);

	text.SetToFormat("%llu", static_cast<unsigned long long>(fCNROMState.write_count));
	x = DrawText(x, y, text.String(), be_plain_font);
	x += 16.0f;
	x = DrawText(x, y, "Last Write: ", be_plain_font);

	if (fCNROMState.have_last_write) {
		text.SetToFormat("$%04X", static_cast<unsigned int>(fCNROMState.last_write_address));
		x = DrawText(x, y, text.String(), be_fixed_font);
		x = DrawText(x, y, " = ", be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fCNROMState.last_write_value));
		DrawText(x, y, text.String(), be_fixed_font);
	} else {
		DrawText(x, y, "None", be_plain_font);
	}

	SetHighColor(25, 25, 25);
}


// -----------------------------------------------------------------------------
// MapperExplorerView::DrawMMC5Panel
//
// Draws the MMC5-specific diagnostics panel.
//
// MMC5 exposes substantially more internal mapper state than the other
// currently supported mappers, so its diagnostics are divided into three
// functional columns: PRG/CHR banking, nametable/ExRAM state, and IRQ/
// miscellaneous hardware state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
MapperExplorerView::DrawMMC5Panel()
{
	if (!fHaveMMC5State) {
		return;
	}

	const float left = 20.0f;
	const float leftText = 28.0f;

	const float middle = 300.0f;
	const float middleText = 308.0f;

	const float right = 580.0f;
	const float rightText = 588.0f;

	float y = 478.0f;

	BString text;
	float x;


	// -------------------------------------------------------------------------
	// Main MMC5 heading.
	// -------------------------------------------------------------------------

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);

	DrawString("MMC5 Internal State", BPoint(left, y));
	y += 28.0f;


	// -------------------------------------------------------------------------
	// Column headings.
	// -------------------------------------------------------------------------

	DrawString("PRG / CHR", BPoint(left, y));
	DrawString("Nametable / ExRAM", BPoint(middle, y));
	DrawString("IRQ / Misc", BPoint(right, y));


	// =========================================================================
	// Left column: PRG / CHR.
	// =========================================================================

	float leftY = y + 24.0f;

	SetHighColor(25, 25, 25);

	x = leftText;
	x = DrawText(x, leftY, "PRG Mode: ", be_plain_font);
	
	text.SetToFormat("%u", static_cast<unsigned int>(fMMC5State.prg_mode));
	x = DrawText(x, leftY, text.String(), be_plain_font);
	x += 16.0f;

	x = DrawText(x, leftY, "CHR Mode: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fMMC5State.chr_mode));
	DrawText(x, leftY, text.String(), be_plain_font);


	// Raw PRG registers $5113-$5117.

	leftY += 24.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("PRG Registers", BPoint(left, leftY));

	leftY += 20.0f;

	SetHighColor(25, 25, 25);

	x = leftText;
	x = DrawText(x, leftY, "$5113: ", be_fixed_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_bank[0]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 12.0f;
	x = DrawText(x, leftY, "$5114: ", be_fixed_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_bank[1]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 12.0f;
	x = DrawText(x, leftY, "$5115: ", be_fixed_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_bank[2]));
	DrawText(x, leftY, text.String(), be_fixed_font);

	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "$5116: ", be_fixed_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_bank[3]));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 12.0f;
	x = DrawText(x, leftY, "$5117: ", be_fixed_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_bank[4]));
	DrawText(x, leftY, text.String(), be_fixed_font);


	// Sprite CHR registers.

	leftY += 26.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Sprite CHR Banks", BPoint(left, leftY));

	leftY += 20.0f;
	SetHighColor(25, 25, 25);
	x = leftText;

	for (int i = 0; i < 4; ++i) {
		text.SetToFormat("%d: ", i);
		x = DrawText(x, leftY, text.String(), be_plain_font);
		text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.sp_chr_bank[i]));
		x = DrawText(x, leftY, text.String(), be_fixed_font);

		if (i < 3) {
			x += 10.0f;
		}
	}


	leftY += 18.0f;
	x = leftText;

	for (int i = 4; i < 8; ++i) {
		text.SetToFormat("%d: ", i);
		x = DrawText(x, leftY, text.String(), be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.sp_chr_bank[i]));
		x = DrawText(x, leftY, text.String(), be_fixed_font);

		if (i < 7) {
			x += 10.0f;
		}
	}


	// Background CHR registers.

	leftY += 26.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Background CHR Banks", BPoint(left, leftY));

	leftY += 20.0f;
	SetHighColor(25, 25, 25);
	x = leftText;

	for (int i = 0; i < 4; ++i) {
		text.SetToFormat("%d: ", i);
		x = DrawText(x, leftY, text.String(), be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.bg_chr_bank[i]));
		x = DrawText(x, leftY, text.String(), be_fixed_font);

		if (i < 3) {
			x += 10.0f;
		}
	}


	leftY += 18.0f;
	x = leftText;

	for (int i = 4; i < 8; ++i) {
		text.SetToFormat("%d: ", i);
		x = DrawText(x, leftY, text.String(), be_plain_font);

		text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.bg_chr_bank[i]));
		x = DrawText(x, leftY, text.String(), be_fixed_font);

		if (i < 7) {
			x += 10.0f;
		}
	}


	// Extended CHR selection state.

	leftY += 20.0f;
	x = leftText;
	x = DrawText(x, leftY, "Upper: ", be_plain_font);

	text.SetToFormat("$%04X", static_cast<unsigned int>(fMMC5State.bg_char_upper));
	x = DrawText(x, leftY, text.String(), be_fixed_font);
	x += 16.0f;

	x = DrawText(x, leftY, "Last Set: ", be_plain_font);
	DrawText(x, leftY, fMMC5State.last_chr_write_bg ? "Background" : "Sprite", be_plain_font);


	// =========================================================================
	// Middle column: Nametable / ExRAM.
	// =========================================================================

	float middleY = y + 24.0f;

	SetHighColor(25, 25, 25);

	x = middleText;
	x = DrawText(x, middleY, "NT Mapping: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.mirroring_mode));
	DrawText(x, middleY, text.String(), be_fixed_font);


	middleY += 20.0f;
	x = middleText;
	x = DrawText(x, middleY, "ExRAM Mode: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fMMC5State.exram_mode));
	DrawText(x, middleY, text.String(), be_plain_font);


	// Fill mode.

	middleY += 28.0f;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Fill Mode", BPoint(middle, middleY));

	middleY += 22.0f;
	SetHighColor(25, 25, 25);
	x = middleText;
	x = DrawText(x, middleY, "Tile: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.fill_mode_tile));
	x = DrawText(x, middleY, text.String(), be_fixed_font);
	x += 16.0f;

	x = DrawText(x, middleY, "Attribute: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.fill_mode_attr));
	DrawText(x, middleY, text.String(), be_fixed_font);


	// Vertical split state.

	middleY += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Vertical Split", BPoint(middle, middleY));

	middleY += 22.0f;
	SetHighColor(25, 25, 25);
	x = middleText;
	x = DrawText(x, middleY, "Mode: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.vertical_split_mode));
	DrawText(x, middleY, text.String(), be_fixed_font);
	
	middleY += 20.0f;
	x = middleText;
	x = DrawText(x, middleY, "Scroll: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.vertical_split_scroll));
	x = DrawText(x, middleY, text.String(), be_fixed_font);
	
	x += 16.0f;
	x = DrawText(x, middleY, "Bank: ", be_plain_font);
	
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.vertical_split_bank));
	DrawText(x, middleY, text.String(), be_fixed_font);


	// PRG-RAM write protection.

	middleY += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("PRG RAM Protect", BPoint(middle, middleY));
	middleY += 22.0f;

	SetHighColor(25, 25, 25);
	x = middleText;
	x = DrawText(x, middleY, "P1: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_ram_protect1));
	x = DrawText(x, middleY, text.String(), be_fixed_font);
	x += 20.0f;

	x = DrawText(x, middleY, "P2: ", be_plain_font);
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.prg_ram_protect2));
	DrawText(x, middleY, text.String(), be_fixed_font);


	// =========================================================================
	// Right column: IRQ / miscellaneous hardware state.
	// =========================================================================

	float rightY = y + 24.0f;

	SetHighColor(25, 25, 25);

	x = rightText;
	x = DrawText(x, rightY, "Target: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.irq_target));
	x = DrawText(x, rightY, text.String(), be_fixed_font);
	x += 16.0f;
	x = DrawText(x, rightY, "Counter: ", be_plain_font);
	
	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.irq_counter));
	DrawText(x,rightY, text.String(), be_fixed_font);

	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Enabled: ", be_plain_font);

	if (fMMC5State.irq_enabled) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	DrawText(x, rightY, fMMC5State.irq_enabled ? "Yes" : "No", be_plain_font);

	SetHighColor(25, 25, 25);
	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "In Frame: ", be_plain_font);

	DrawText(x, rightY, fMMC5State.irq_in_frame ? "Yes" : "No", be_plain_font);
	rightY += 20.0f;
	x = rightText;

	x = DrawText(x, rightY, "Pending: ", be_plain_font);

	if (fMMC5State.irq_pending) {
		SetHighColor(45, 105, 55);
	} else {
		SetHighColor(115, 115, 115);
	}

	DrawText(x, rightY, fMMC5State.irq_pending ? "Yes" : "No", be_plain_font);

	SetHighColor(25, 25, 25);


	// Rendering state.

	rightY += 30.0f;

	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Rendering", BPoint(right, rightY));

	rightY += 22.0f;

	SetHighColor(25, 25, 25);
	x = rightText;
	x = DrawText(x, rightY, "Large Sprites: ", be_plain_font);
	DrawText(x, rightY, fMMC5State.large_sprites ? "Yes" : "No", be_plain_font);

	rightY += 20.0f;
	SetHighColor(100, 100, 100);
	x = rightText;
	x = DrawText(x, rightY, "Fetch Count: ", be_plain_font);

	text.SetToFormat("%u", static_cast<unsigned int>(fMMC5State.fetch_count));
	DrawText(x, rightY, text.String(), be_plain_font);


	// Multiplier.

	rightY += 30.0f;
	SetFont(be_bold_font);
	SetHighColor(45, 75, 115);
	DrawString("Multiplier", BPoint(right, rightY));

	rightY += 22.0f;
	SetHighColor(25, 25, 25);
	x = rightText;
	x = DrawText(x, rightY, "A: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.multiplier_1));
	x = DrawText(x, rightY, text.String(), be_fixed_font);
	x += 16.0f;

	x = DrawText(x, rightY, "B: ", be_plain_font);

	text.SetToFormat("$%02X", static_cast<unsigned int>(fMMC5State.multiplier_2));
	DrawText(x, rightY, text.String(), be_fixed_font);

	rightY += 20.0f;
	x = rightText;
	x = DrawText(x, rightY, "Result: ", be_plain_font);

	text.SetToFormat("$%04X", static_cast<unsigned int>(fMMC5State.multiplier_result));
	DrawText(x, rightY, text.String(), be_fixed_font);

	SetHighColor(25, 25, 25);
}

