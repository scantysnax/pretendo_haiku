
#ifndef _ROM_INFO_WINDOW_H_
#define _ROM_INFO_WINDOW_H_

#include <Window.h>
#include <TabView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <libxml2/libxml/parser.h>
#include <string>

#include "ROMInfoView.h"

using std::string;

class ROMInfoScrollView : public BScrollView
{
	public:
	ROMInfoScrollView();
	virtual ~ROMInfoScrollView();
	
	public:
	virtual void Draw(BRect updateRect);
	virtual void AttachedToWindow (void);
};


class ROMInfoWindow : public BWindow
{	
	public:
			ROMInfoWindow();
	virtual ~ROMInfoWindow();
	
	public:
	virtual void MessageReceived (BMessage *message);
	virtual bool QuitRequested (void);
	
	private:
	ROMInfoView *fROMInfoView;
};


#endif //_ROM_INFO_WINDOW_H_
