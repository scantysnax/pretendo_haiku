
#ifndef _ROM_INFO_WINDOW_H_
#define _ROM_INFO_WINDOW_H_

#include <Window.h>
#include <TabView.h>
#include <ListView.h>
#include <ScrollView.h>

#include <libxml2/libxml/parser.h>


#include "ROMInfoView.h"

class ROMInfoScrollView : public BScrollView
{
	public:
	ROMInfoScrollView();
	virtual ~ROMInfoScrollView();
	
	public:
	virtual void Draw (BRect updateRect);
	virtual void AttachedToWindow();
	
	private:
	
};


class ROMInfoWindow : public BWindow
{	
	public:
			ROMInfoWindow();
	virtual ~ROMInfoWindow();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual bool QuitRequested();
	
	private:
	void LoadSettings();
	void SaveSettings();
	BMessage *fSettingsMessage;
	
	private:
	ROMInfoView *fROMInfoView = nullptr;
};


#endif //_ROM_INFO_WINDOW_H_
