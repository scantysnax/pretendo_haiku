
#ifndef _APU_SCOPE_VIEW_H_
#define _APU_SCOPE_VIEW_H_

#include <View.h>

#include "Apu.h"
#include "Cart.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"


class APUScopeView : public BView
{
	private:
	enum scope_channel : int32 {
		SCOPE_SQUARE1 = 0,
		SCOPE_SQUARE2,
		SCOPE_TRIANGLE,
		SCOPE_NOISE,
		SCOPE_DMC,
		SCOPE_MIXED
	};
	
	public:
			APUScopeView (BRect frame, PretendoWindow *parent);
	virtual ~APUScopeView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();
	virtual void KeyDown (const char *bytes, int32 numBytes);

	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage();
	void DrawHeaderPanel();

	private:
	void CaptureSamples();
	void DrawWaveformPanel (BRect panel, const char *title, scope_channel channel, float maximumValue);
	float SampleValue (const nes::apu::apu_scope_sample_t &sample, scope_channel channel) const;
	void ComputeVisibleSampleRange (uint32 &startIndex, uint32 &endIndex) const;
	void ComputeVisibleMinMax (scope_channel channel, float &minimumValue, float &maximumValue) const;
	
	private:
	void ClampCursor();
	bool FindTriggerSample (scope_channel channel, uint32 &triggerIndex) const;
	float TriggerThreshold (scope_channel channel) const;
	const char *TriggerChannelName() const;
	void CycleTriggerMode();
	
	private:
	void ToggleChannelVisible (scope_channel channel);
	void CycleSoloChannel();
	const char *SoloChannelName() const;
	
	private:
	nes::apu::apu_scope_sample_t fSamples[nes::apu::APU_SCOPE_SAMPLE_CAPACITY];
	uint32 fSampleCount = 0;
	bool fFreezeUpdates = false;
	int32 fCursorSample = -1;
	uint32 fVisibleSampleCount = nes::apu::APU_SCOPE_SAMPLE_CAPACITY;
	nes::apu::apu_debug_state_t fDebugState;
	bool fTriggerEnabled = false;
	scope_channel fTriggerChannel = SCOPE_SQUARE1;
	
	private:
	bool fChannelVisible[6] = { true, true, true, true, true, true };
	int32 fSoloChannel = -1;
};


#endif	// _APU_SCOPE_VIEW_

