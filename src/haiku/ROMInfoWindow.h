
#ifndef _ROM_INFO_WINDOW_H_
#define _ROM_INFO_WINDOW_H_

#include <ListView.h>
#include <TabView.h>
#include <ScrollView.h>
#include <Window.h>

#include <libxml2/libxml/parser.h>

#include "ROMInfoView.h"


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
