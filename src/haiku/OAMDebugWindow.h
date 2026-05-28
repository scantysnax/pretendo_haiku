#ifndef _OAM_DEBUG_WINDOW_H_
#define _OAM_DEBUG_WINDOW_H_

#include <Message.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <Window.h>


class PretendoWindow;
class OAMDebugView;
class PatternTableWindow;
class CHRExplorerView;


class OAMDebugWindow : public BWindow
{
	public:
			OAMDebugWindow(PretendoWindow *parent);
	virtual ~OAMDebugWindow();

	virtual bool QuitRequested();

	public:
	void SetPatternTables(PatternTableWindow* pt0, PatternTableWindow* pt1);

	private:
	PretendoWindow* fParent = nullptr;
	OAMDebugView* fView = nullptr;
	CHRExplorerView* fExplorer = nullptr;
};


#endif // _OAM_DEBUG_WINDOW_H_
