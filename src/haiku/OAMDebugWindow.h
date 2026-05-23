#ifndef OAM_DEBUG_WINDOW_H_
#define OAM_DEBUG_WINDOW_H_

#include <Message.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <Window.h>


class PretendoWindow;
class OAMDebugView;


class OAMDebugWindow : public BWindow
{
	public:
			OAMDebugWindow(PretendoWindow* parent);
	virtual ~OAMDebugWindow();

	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent = nullptr;
	OAMDebugView *fView = nullptr;
};


#endif
