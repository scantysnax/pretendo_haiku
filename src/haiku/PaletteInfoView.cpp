
#include "PaletteInfoView.h"


PaletteInfoView::PaletteInfoView(BRect frame)
	: BView(frame, "palette_info_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_NAVIGABLE)
{
}


PaletteInfoView::~PaletteInfoView()
{
}


void
PaletteInfoView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BView::AttachedToWindow();
}


void 
PaletteInfoView::Draw (BRect updateRect)
{
	BView::Draw(updateRect);
}


void
PaletteInfoView::MessageReceived (BMessage *message)
{
	BView::MessageReceived(message);
}
