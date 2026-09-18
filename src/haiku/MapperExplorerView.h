#ifndef _MAPPER_EXPLORER_VIEW_H_
#define _MAPPER_EXPLORER_VIEW_H_

#include <String.h>
#include <View.h>

#include "Mapper.h"
#include "../mappers/Mapper001.h"


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
	bool CaptureState();
	bool BankStateChanged (
		const mapper_debug_bank_t &oldBank,
		const mapper_debug_bank_t &newBank) const;

	private:
	bool CaptureMMC1State();
	bool MMC1StateChanged (
		const mapper1_debug_state_t &oldState,
		const mapper1_debug_state_t &newState) const;

	private:
	void DrawNoROMMessage();
	void DrawMapperSummary();
	void DrawPRGTable();
	void DrawCHRTable();

	void DrawTableHeader (float y, const char *bankHeading);
	void DrawBankRow (
		float y,
		const char *range,
		const mapper_debug_bank_t &bank,
		uint32 bankSize,
		bool changed);

	private:
	void DrawMMC1Panel();

	private:
	const char *MemoryTypeName (MapperDebugMemoryType type) const;
	const char *MirroringName (MapperDebugMirroring mirroring) const;
	void FormatAccess (const mapper_debug_bank_t &bank, BString &text) const;

	private:
	const char *MMC1PRGModeName (uint8_t mode) const;
	const char *MMC1CHRModeName (uint8_t mode) const;
	const char *MMC1RegisterName (uint8_t reg) const;

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
	mapper1_debug_state_t fMMC1State = {};
	bool fHaveMMC1State = false;
};


#endif // _MAPPER_EXPLORER_VIEW_H_
