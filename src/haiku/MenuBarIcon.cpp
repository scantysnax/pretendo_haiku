
#include "MenuBarIcon.h"


#include <Application.h>
#include <AppFileInfo.h>
#include <File.h>
#include <Roster.h>

MenuBarIcon::MenuBarIcon (BRect frame, BMenuBar *menuBar)
	: BView (frame, "menu_icon", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fMenuBar = menuBar;
	
	app_info ai;
	BFile file;
	BAppFileInfo afi;
	
	if (be_app->GetAppInfo(&ai) == B_OK) {
		file.SetTo(&ai.ref, B_READ_ONLY);
		afi.SetTo(&file);

		fIconBitmap = new BBitmap(frame, B_RGBA32);

		if (afi.GetIcon(fIconBitmap, B_MINI_ICON) != B_OK) {
			delete fIconBitmap;
			fIconBitmap = nullptr;
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
	DrawBitmap(fIconBitmap, Bounds());
	
	BView::Draw (updateRect);
}	

