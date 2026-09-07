#ifndef _APU_EXPLORER_VIEW_H_
#define _APU_EXPLORER_VIEW_H_

#include <View.h>

#include "Apu.h"
#include "Cart.h"
#include "DebugHelpers.h"


class PretendoWindow;


// -----------------------------------------------------------------------------
// APUExplorerView
//
// Debugger view for inspecting the current programmed and effective state of the
// NES APU.
//
// The view combines two side-effect-free debugger snapshots:
//
//   explorer_state() - most recently programmed raw APU register values.
//   debug_state()    - current effective/internal APU channel state.
//
// This allows the debugger to show both what software last wrote and what the
// APU is currently doing as a result.
// -----------------------------------------------------------------------------
class APUExplorerView : public BView
{
	public:
			APUExplorerView (BRect frame, PretendoWindow *parent);
	virtual ~APUExplorerView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();

	private:
	void CaptureState();

	private:
	void DrawHeaderPanel();
	void DrawSquare1Panel (BRect panel);
	void DrawSquare2Panel (BRect panel);
	void DrawTrianglePanel (BRect panel);
	void DrawNoisePanel (BRect panel);
	void DrawDMCPanel (BRect panel);
	void DrawGlobalPanel (BRect panel);

	private:
	void DrawRegisterLine (const char *address, const char *name, uint8 value, float x, float y);
	void DrawTextLine (const char *label, const char *value, float x, float y);
	void DrawBoolLine (const char *label, bool value, float x, float y);
	void DrawFixedLine (const char *label, const char *value, float x, float y);
	void DrawStateLine (const char *label, bool value, bool goodWhenTrue, float x, float y);
	void DrawColoredFixedLine (const char *label, const char *value, rgb_color color, float x, float y);

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage(BRect panel);

	private:
	PretendoWindow *fParent = nullptr;

	private:
	nes::apu::apu_explorer_state_t fExplorerState;
	nes::apu::apu_debug_state_t fDebugState;
};


#endif // _APU_EXPLORER_VIEW_H_

