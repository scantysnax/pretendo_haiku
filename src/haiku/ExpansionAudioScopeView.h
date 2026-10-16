#ifndef _EXPANSION_AUDIO_SCOPE_VIEW_H_
#define _EXPANSION_AUDIO_SCOPE_VIEW_H_

#include <View.h>

#include "Apu.h"
#include "Mapper.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// ExpansionAudioScopeView
//
// Displays mapper-provided expansion-audio channel waveforms captured by the
// APU oscilloscope history.
//
// The view supports live/frozen display, zoom, trigger tracking, mouse-adjustable
// trigger position, Min/Max statistics, Solo, per-panel visibility, and restore-
// all controls.
// -----------------------------------------------------------------------------
class ExpansionAudioScopeView : public BView
{
	private:
	enum trigger_channel : int32 {
		TRIGGER_CHANNEL_0 = 0,
		TRIGGER_CHANNEL_1,
		TRIGGER_CHANNEL_2,
		TRIGGER_TOTAL
	};

	public:
			ExpansionAudioScopeView (BRect frame);
	virtual ~ExpansionAudioScopeView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MouseDown (BPoint where);

	private:
	void CaptureSamples();
	void DrawHeaderPanel();
	void DrawWaveformPanel (BRect panel, const char *title, size_t channel, float maximumValue);
	void DrawTotalWaveformPanel (BRect panel);
	void ClampCursor();
	void ComputeVisibleSampleRange (uint32 &startIndex, uint32 &endIndex) const;
	void ComputeVisibleMinMax (size_t channel, float &minimumValue, float &maximumValue) const;
	void ComputeVisibleTotalMinMax (float &minimumValue, float &maximumValue) const;

	private:
	void CycleTriggerMode();
	const char *TriggerChannelName() const;
	bool FindTriggerSample (trigger_channel channel, uint32 latestIndex, uint32 &triggerIndex) const;
	float TriggerThreshold (trigger_channel channel) const;

	private:
	void CycleSoloMode();

	private:
	nes::apu::scope_sample_t fSamples[nes::apu::APU_SCOPE_SAMPLE_CAPACITY];
	uint32 fSampleCount = 0;
	mapper_audio_debug_state_t fDebugState;
	size_t fChannelCount = 0;
	bool fFreezeUpdates = false;
	int32 fCursorSample = -1;
	uint32 fVisibleSampleCount = nes::apu::APU_SCOPE_SAMPLE_CAPACITY;
	bool fTriggerEnabled = false;
	trigger_channel fTriggerChannel = TRIGGER_CHANNEL_0;
	float fTriggerPosition = 0.25f;
	int32 fSoloChannel = -1;
	bool fChannelVisible[4] = {
		true,
		true,
		true,
		true
	};
};


#endif	// _EXPANSION_AUDIO_SCOPE_VIEW_H_
