#ifndef _PPU_STATUS_VIEW_H_
#define _PPU_STATUS_VIEW_H_

#include <View.h>

#include <cmath>

#include "Cart.h"
#include "DebugHelpers.h"
#include "Ppu.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// PPUStatusView
//
// Debugger view for inspecting live PPU render state.  The view summarizes the
// important PPU registers and decodes the bits that affect name table selection,
// pattern table selection, sprite size, rendering enable state, monochrome mode,
// color emphasis, and scroll address state.
//
// The PPU state is captured once at the beginning of each redraw so every panel
// represents one coherent debugger sample.
// -----------------------------------------------------------------------------
class PPUStatusView : public BView
{
	public:
			PPUStatusView(BRect frame, PretendoWindow *parent);
	virtual ~PPUStatusView();

	public:
	virtual void Draw(BRect updateRect);
	virtual void Pulse();

	private:
	struct ppu_status_snapshot_t {
		uint8 ctrl = 0x00;
		uint8 mask = 0x00;
		uint8 status = 0x00;
		uint8 oamAddr = 0x00;

		nes::ppu::scroll_state_t scroll = {};

		uint16 dot = 0;
		uint16 scanline = 0;
	};

	private:
	void CaptureSnapshot();
	ppu_status_snapshot_t fSnapshot = {};

	void DrawHeaderPanel();
	void DrawRegisterPanel();
	void DrawControlPanel();
	void DrawMaskPanel();
	void DrawStatusPanel();
	void DrawTimingPanel();

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage(BRect panel);

};


#endif // _PPU_STATUS_VIEW_H_

