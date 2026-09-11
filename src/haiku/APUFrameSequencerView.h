
#ifndef APU_FRAME_SEQUENCER_VIEW_H
#define APU_FRAME_SEQUENCER_VIEW_H

#include <Font.h>
#include <String.h>
#include <View.h>

#include "Apu.h"
#include "PretendoWindow.h"


class APUFrameSequencerView : public BView
{
	public:
			APUFrameSequencerView (BRect frame, PretendoWindow *parent);
	virtual	~APUFrameSequencerView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void Pulse();

	private:
	void CaptureState();
	void DrawHeaderPanel();
	void DrawStatePanel();
	void DrawTimelinePanel();
	void DrawRecentEventsPanel();
	void DrawNoROMMessage();
	bool HasROMLoaded() const;

	private:
	nes::apu::apu_frame_sequencer_debug_state_t fState;
	nes::apu::apu_frame_event_t fEvents[nes::apu::APU_FRAME_EVENT_CAPACITY];
	uint32 fEventCount;
	bool fFreezeUpdates;
};


#endif
