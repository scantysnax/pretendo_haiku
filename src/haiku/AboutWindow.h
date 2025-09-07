
#ifndef _ABOUT_WINDOW_H_
#define _ABOUT_WINDOW_H_

#include <Window.h>
#include <Bitmap.h>
#include <Button.h>
#include <Roster.h>
#include <String.h>
#include <TranslationUtils.h>
#include <TextView.h>

#include "LinkView.h"
#include "version.h"


class AboutView : public BView
{
	public:
			AboutView (BRect frame);
	virtual ~AboutView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	
	private:
	BBitmap *fIcon;
	BBitmap *fLogo;
};


class AboutWindow : public BWindow 
{
	public:
			AboutWindow();
	virtual ~AboutWindow();
	
	public:
	virtual bool QuitRequested();
	virtual void MessageReceived (BMessage *message);
	
	private:
	AboutView *fAboutView;
};


#endif // _ABOUT_WINDOW_H_
