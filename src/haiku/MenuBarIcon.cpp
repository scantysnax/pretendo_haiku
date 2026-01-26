
#include "MenuBarIcon.h"


MenuBarIcon::MenuBarIcon (BRect frame, BMenuBar *menuBar)
	: BView (frame, "menu_icon", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fMenuBar = menuBar;
}


MenuBarIcon::~MenuBarIcon()
{
	delete fIconBitmap;
}


void
MenuBarIcon::AttachedToWindow()
{
	fIconBitmap = BTranslationUtils::GetBitmap('bits', "Icon");
		
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
MenuBarIcon::Draw (BRect updateRect)
{	
	BRect r(Bounds());
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(r);
	
	SetDrawingMode(B_OP_OVER);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fIconBitmap, r);
	
	BView::Draw (updateRect);
}	

