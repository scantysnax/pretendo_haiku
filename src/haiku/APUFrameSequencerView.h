
#ifndef _APU_FRAME_SEQUENCER_VIEW_H_
#define _APU_FRAME_SEQUENCER_VIEW_H_

#include <Font.h>
#include <OS.h>
#include <String.h>
#include <View.h>

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
	nes::apu::frame_sequencer_debug_state_t fState = {};
	nes::apu::frame_event_t fEvents[nes::apu::APU_FRAME_EVENT_CAPACITY] = {};
	uint32 fEventCount = 0;
	bool fFreezeUpdates = false;

	private:
	// Display-only timeline sweep position.  This is intentionally independent
	// of the exact sampled APU frame-sequencer position.
	float fDisplayedSequenceCycle = 0.0f;

	// Indicates whether the display sweep has been initialized from a real
	// captured sequencer position.
	bool fHaveDisplayedSequenceCycle = false;

	// Host timestamp of the previous timeline redraw, used to advance the
	// display-only sweep smoothly.
	bigtime_t fLastDisplayTime = 0;

	// Last captured APU cycle, used to determine whether emulation is actually
	// advancing.  The visual sweep remains stationary while the APU is stopped.
	uint64 fLastDisplayedAPUCycle = 0;
	
	public:
	void ResetView();
};

#endif	// _APU_FRAME_SEQUENCER_VIEW_H_
