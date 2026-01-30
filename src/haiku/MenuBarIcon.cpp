
#include "MenuBarIcon.h"


MenuBarIcon::MenuBarIcon (BRect frame, BMenuBar *menuBar)
	: BView (frame, "menu_icon", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fMenuBar = menuBar;
	fIconBitmap = new BBitmap(frame, B_RGBA32);
	
	if (fIconBitmap->IsValid()) {
		app_info appInfo;
		
		if (be_app->GetAppInfo(&appInfo) == B_OK) {
			BFile file;
			BAppFileInfo appFileInfo;
			
			file.SetTo(&appInfo.ref, B_READ_ONLY);
			appFileInfo.SetTo(&file);
			appFileInfo.GetIcon(fIconBitmap, B_MINI_ICON);
		}
	}
}


MenuBarIcon::~MenuBarIcon()
{
	delete fIconBitmap;
}


void
MenuBarIcon::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
MenuBarIcon::Draw (BRect updateRect)
{
	SetDrawingMode(B_OP_OVER);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fIconBitmap);
	
	BView::Draw(updateRect);
}	

