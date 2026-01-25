
#ifndef _MENU_BAR_ICON_H_
#define _MENU_BAR_ICON_H_

#include <View.h>
#include <MenuBar.h>
#include <Bitmap.h>
#include <TranslationUtils.h>

class MenuBarIcon : public BView
{
	public:
			MenuBarIcon (BRect frame, BMenuBar *menuBar);
	virtual ~MenuBarIcon();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	
	private:
	BMenuBar *fMenuBar = nullptr;
	BBitmap *fIconBitmap = nullptr;	
};


#endif // _MENU_BAR_ICON_H_
