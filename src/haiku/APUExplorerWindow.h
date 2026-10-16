#ifndef _APU_EXPLORER_WINDOW_H_
#define _APU_EXPLORER_WINDOW_H_


#include <TabView.h>
#include <View.h>

#include "APUExplorerView.h"
#include "ExpansionAudioExplorerView.h"
#include "PretendoWindow.h"


class APUExplorerView;


// -----------------------------------------------------------------------------
// APUExplorerWindow
//
// Floating debugger window that owns the APU Explorer tab interface.
//
// The Native APU tab contains the existing APUExplorerView.  The Expansion tab
// is reserved for mapper-provided expansion-audio state such as VRC6.
//
// This keeps the debugger window compact while allowing mapper-specific audio
// information to be added without increasing the overall window size.
// -----------------------------------------------------------------------------
class APUExplorerWindow : public BWindow
{
	public:
			APUExplorerWindow (PretendoWindow *parent);
	virtual ~APUExplorerWindow();

	public:
	virtual bool QuitRequested();
	//virtual void Pulse();
	virtual void UpdateExpansionTab();

	private:
	PretendoWindow *fParent = nullptr;

	BTabView *fTabView = nullptr;

	APUExplorerView *fView = nullptr;
	ExpansionAudioExplorerView *fExpansionView = nullptr;
};


#endif // _APU_EXPLORER_WINDOW_H_
