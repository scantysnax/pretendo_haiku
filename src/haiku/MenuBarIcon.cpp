
#include "MenuBarIcon.h"


MenuBarIcon::MenuBarIcon (BRect frame, BMenuBar *menuBar)
	: BView (frame, "menu_icon", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fIconBitmap = new BBitmap(BRect(0, 0, icon_size::WIDTH-1, icon_size::HEIGHT-1), B_RGBA32);
	fMenuBar = menuBar;
	
	frame.PrintToStream();
}


MenuBarIcon::~MenuBarIcon()
{
	delete fIconBitmap;
}


void
MenuBarIcon::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	if (fIconBitmap->IsValid()) {
		app_info appInfo;
		
		if (be_app->GetAppInfo(&appInfo) == B_OK) {
			BFile file(&appInfo.ref, B_READ_ONLY);
			
			if (file.InitCheck() == B_OK) {
				BAppFileInfo appFileInfo(&file);
				
				if (appFileInfo.InitCheck() == B_OK) {
					appFileInfo.GetIcon(fIconBitmap, B_MINI_ICON);
				}
			}
		}
	}
	
	//MoveTo(0,0);
	
	BView::AttachedToWindow();
}


void
MenuBarIcon::Draw (BRect updateRect)
{
	SetDrawingMode(B_OP_OVER);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmap(fIconBitmap);
	
	BView::Draw(updateRect);
}	

