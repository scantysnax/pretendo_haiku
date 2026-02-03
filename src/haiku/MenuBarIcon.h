
#ifndef _MENU_BAR_ICON_H_
#define _MENU_BAR_ICON_H_

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <MenuBar.h>
#include <Roster.h>
#include <View.h>


class MenuBarIcon : public BView
{
	public:
	typedef enum {
		WIDTH = 18,
		HEIGHT = 18,
		PADDING = 2
	} icon_size;
	
	
	public:
			MenuBarIcon(BMenuBar *menuBar);
	virtual ~MenuBarIcon();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	
	private:
	BBitmap *fIconBitmap = nullptr;	
	BMenuBar *fMenuBar = nullptr;
};


#endif // _MENU_BAR_ICON_H_
