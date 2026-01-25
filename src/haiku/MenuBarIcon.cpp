
#include "MenuBarIcon.h"

#include <iostream>


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
	
	if (! fIconBitmap->IsValid() || fIconBitmap == nullptr) {
		std::cout << __PRETTY_FUNCTION__ << " " << "failed to load icon" << std::endl;
	}
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
MenuBarIcon::Draw (BRect updateRect)
{	
	BRect r = Bounds();
	//r.left = 
	r.PrintToStream();
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(r);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBitmap, r);
	
	//BView::Draw (updateRect);
}	

