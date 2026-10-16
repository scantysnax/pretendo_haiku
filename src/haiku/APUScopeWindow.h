#ifndef _APU_SCOPE_WINDOW_H_
#define _APU_SCOPE_WINDOW_H_

#include <Window.h>


class APUScopeView;
class BTabView;
class ExpansionAudioScopeView;
class PretendoWindow;


// -----------------------------------------------------------------------------
// APUScopeWindow
//
// Hosts the APU oscilloscope debugger.
//
// The window uses a tabbed layout so the native NES APU oscilloscope and
// mapper-provided expansion-audio oscilloscope can share the same debugger
// window.
//
// The Expansion tab is created only while the active mapper exposes expansion
// audio.
// -----------------------------------------------------------------------------
class APUScopeWindow : public BWindow
{
	public:
			APUScopeWindow (PretendoWindow *parent);
	virtual ~APUScopeWindow();

	public:
	virtual bool QuitRequested();

	public:
	void UpdateExpansionTab();

	private:
	PretendoWindow *fParent = nullptr;

	BTabView *fTabView = nullptr;

	APUScopeView *fView = nullptr;
	ExpansionAudioScopeView *fExpansionView = nullptr;
};


#endif	// _APU_SCOPE_WINDOW_H_
