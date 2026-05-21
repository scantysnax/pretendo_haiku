
#include "PaletteDebugView.h"


PaletteDebugView::PaletteDebugView (BRect frame)
	: BView(frame, "palette_debug_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_NAVIGABLE)
{
}


PaletteDebugView::~PaletteDebugView()
{
}


void
PaletteDebugView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BView::AttachedToWindow();
}


void 
PaletteDebugView::Draw (BRect updateRect)
{
	BView::Draw(updateRect);
}


void
PaletteDebugView::MessageReceived (BMessage *message)
{
	BView::MessageReceived(message);
}
