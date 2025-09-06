
#ifndef _PRETENDO_APP_H_
#define _PRETENDO_APP_H_

#include <Application.h>
#include <Path.h>

#include "PretendoWindow.h"
#include "AboutWindow.h"


class PretendoApp : public BApplication 
{
	public:
			PretendoApp();
	virtual ~PretendoApp();
	
	public:
	virtual void ReadyToRun();
	virtual void AboutRequested();
	virtual void RefsReceived (BMessage *message);
	virtual void ArgvReceived (int32 argc, char **argv);
	
	private:
	PretendoWindow *fWindow = nullptr;
};


#endif // _PRETENDO_APP_H_
