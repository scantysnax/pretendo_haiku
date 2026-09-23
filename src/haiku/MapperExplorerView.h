
#ifndef _MAPPER_EXPLORER_VIEW_H_
#define _MAPPER_EXPLORER_VIEW_H_

#include <Font.h>
#include <String.h>
#include <View.h>

#include <algorithm>

#include "Cart.h"
#include "Mapper.h"
#include "Mapper000.h"
#include "Mapper001.h"
#include "Mapper002.h"
#include "Mapper003.h"
#include "Mapper004.h"
#include "Mapper005.h"
#include "Mapper007.h"
#include "Mapper009.h"

class PretendoWindow;


// -----------------------------------------------------------------------------
// MapperExplorerView
//
// Debugger view for inspecting the mapper-visible CPU PRG and PPU CHR memory
// mappings.
//
// The view presents the generic resolved mapping state supplied by Mapper rather
// than interpreting mapper-specific registers itself. Mapper-specific debugger
// state may also be displayed when the active mapper exposes additional useful
// internal information.
// -----------------------------------------------------------------------------
class MapperExplorerView : public BView
{
	public:
			MapperExplorerView (BRect frame, PretendoWindow *parent);
	virtual ~MapperExplorerView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	
	private:
	float DrawText (float x, float y, const char *text, const BFont *font);
	void DrawNoROMMessage();

	private:
	bool CaptureState();
	bool BankStateChanged (const mapper_debug_bank_t &oldBank,
						   const mapper_debug_bank_t &newBank) const;
	private:
	bool CaptureMMC1State();
	bool MMC1StateChanged (const mmc1_debug_state_t &oldState, 
						   const mmc1_debug_state_t &newState) const;
	private:
	bool CaptureUxROMState();
	bool UxROMStateChanged (const uxrom_debug_state_t &oldState,
							const uxrom_debug_state_t &newState) const;	
	private:
	bool CaptureCNROMState();
	bool CNROMStateChanged (const cnrom_debug_state_t &oldState,
							const cnrom_debug_state_t &newState) const;	
	private:
	bool CaptureMMC3State();
	bool MMC3StateChanged (const mmc3_debug_state_t &oldState, 
						   const mmc3_debug_state_t &newState) const;
	private:
	bool CaptureMMC5State();
	bool MMC5StateChanged (const mmc5_debug_state_t &oldState,
						   const mmc5_debug_state_t &newState) const;
	private:
	bool CaptureAxROMState();
	bool AxROMStateChanged (const axrom_debug_state_t &oldState,
							const axrom_debug_state_t &newState) const;			   
	private:
	bool CaptureMMC2State();
	bool MMC2StateChanged (const mmc2_debug_state_t &oldState,
						   const mmc2_debug_state_t &newState) const;					   									   
	private:
	void DrawMapperSummary();
	void DrawPRGTable();
	void DrawCHRTable();

	private:
	void DrawTableHeader (float y, const char *bankHeading);
	void DrawBankRow (float y, const char *range, const mapper_debug_bank_t &bank,
					  uint32 bankSize, bool changed);
	private:
	bool HaveMapperSpecificPanel() const;
	bool MapperSpecificPanelExpected() const;
	void DrawUnsupportedMapperPanel();
	void DrawMapperSpecificSeparator();
	
	private:
	void DrawNROMPanel();
	void DrawMMC1Panel();
	void DrawUxROMPanel();
	void DrawCNROMPanel();
	void DrawMMC3Panel();
	void DrawMMC5Panel();
	void DrawAxROMPanel();
	void DrawMMC2Panel();
	
	private:
	const char *MemoryTypeName (MapperDebugMemoryType type) const;
	const char *MirroringName (MapperDebugMirroring mirroring) const;
	void FormatAccess (const mapper_debug_bank_t &bank, BString &text) const;

	private:
	const char *MMC1PRGModeName (uint8_t mode) const;
	const char *MMC1CHRModeName (uint8_t mode) const;
	const char *MMC1RegisterName (uint8_t reg) const;
	const char *MMC3HardwareModeName (uint8_t mode) const;
	const char *MMC2LatchName (bool latch) const;

	private:
	PretendoWindow *fParent = nullptr;

	private:
	Mapper *fMapper = nullptr;
	mapper_debug_state_t fState = {};
	BString fMapperName;
	bool fHaveState = false;
	uint8 fPRGChangeTicks[5] = {};
	uint8 fCHRChangeTicks[8] = {};

	private:
	mmc1_debug_state_t fMMC1State = {};
	bool fHaveMMC1State = false;
	
	private:
	uxrom_debug_state_t fUxROMState = {};
	bool fHaveUxROMState = false;

	private:
	cnrom_debug_state_t fCNROMState = {};
	bool fHaveCNROMState = false;
	
	private:
	mmc3_debug_state_t fMMC3State = {};
	bool fHaveMMC3State = false;
	
	private:
	mmc5_debug_state_t fMMC5State = {};
	bool fHaveMMC5State = false;
	
	private:
	axrom_debug_state_t fAxROMState = {};
	bool fHaveAxROMState = false;
	
	private:
	mmc2_debug_state_t fMMC2State = {};
	bool fHaveMMC2State = false;	
};


#endif // _MAPPER_EXPLORER_VIEW_H_
