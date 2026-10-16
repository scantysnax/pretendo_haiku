#ifndef _EXPANSION_AUDIO_STATUS_VIEW_H_
#define _EXPANSION_AUDIO_STATUS_VIEW_H_

#include <View.h>

#include "Mapper.h"


class ExpansionAudioStatusView : public BView
{
	public:
			ExpansionAudioStatusView (BRect frame);
	virtual ~ExpansionAudioStatusView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();

	private:
	void CaptureState();

	private:
	void DrawHeader();
	void DrawGlobalPanel (BRect panel);
	void DrawChannelPanel (BRect panel, const mapper_audio_channel_debug_t &channel);

	private:
	void DrawPanelBackground (BRect rect, const char *title);
	void DrawTextLine (const char *label, const char *value, float x, float y);
	void DrawBooleanLine (const char *label, bool value, float x, float y);
	void DrawColoredTextLine (const char *label, const char *value, rgb_color color, float x, float y);

	void DrawLevelMeter (BRect rect, uint32_t value, uint32_t maximum);

	void DrawDebugField (const mapper_audio_debug_field_t &field, float x, float y);

	private:
	mapper_audio_debug_state_t fState;
};


#endif // _EXPANSION_AUDIO_STATUS_VIEW_H_
