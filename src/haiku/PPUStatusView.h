#ifndef _PPU_STATUS_VIEW_H_
#define _PPU_STATUS_VIEW_H_

#include <View.h>


class PretendoWindow;


// -----------------------------------------------------------------------------
// PPUStatusView
//
// Debugger view for inspecting live PPU render state.  The view summarizes the
// important PPU registers and decodes the bits that affect name table selection,
// pattern table selection, sprite size, rendering enable state, monochrome mode,
// color emphasis, and scroll address state.
// -----------------------------------------------------------------------------
class PPUStatusView : public BView
{
	public:
			PPUStatusView (BRect frame, PretendoWindow *parent);
	virtual ~PPUStatusView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();

	private:
	void DrawHeaderPanel();
	void DrawRegisterPanel();
	void DrawControlPanel();
	void DrawMaskPanel();
	void DrawStatusPanel();
	void DrawTimingPanel();

	private:
	PretendoWindow *fParent = nullptr;
};


#endif // _PPU_STATUS_VIEW_H_
