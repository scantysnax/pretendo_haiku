
#ifndef APU_FRAME_SEQUENCER_WINDOW_H
#define APU_FRAME_SEQUENCER_WINDOW_H

#include <Window.h>

class PretendoWindow;
class APUFrameSequencerView;


class APUFrameSequencerWindow : public BWindow {
	public:
			APUFrameSequencerWindow (PretendoWindow *parent);
	virtual ~APUFrameSequencerWindow();
	
	public:
	virtual bool QuitRequested();

	private:
	PretendoWindow *fParent;
	APUFrameSequencerView *fView;
};


#endif
