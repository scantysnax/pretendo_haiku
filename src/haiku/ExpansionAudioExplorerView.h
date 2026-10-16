#ifndef _EXPANSION_AUDIO_EXPLORER_VIEW_H_
#define _EXPANSION_AUDIO_EXPLORER_VIEW_H_

#include <View.h>

#include "Mapper.h"

class PretendoWindow;


// -----------------------------------------------------------------------------
// ExpansionAudioExplorerView
//
// Debugger view for inspecting mapper-provided expansion-audio state.
//
// The view consumes the generic mapper_audio_debug_state_t snapshot exposed by
// Mapper::debug_audio_state().  This keeps the GUI independent of individual
// mapper implementations while still allowing mapper-specific audio hardware
// to expose its current channel state.
// -----------------------------------------------------------------------------
class ExpansionAudioExplorerView : public BView
{
	public:
			ExpansionAudioExplorerView (BRect frame, PretendoWindow *parent);
	virtual ~ExpansionAudioExplorerView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void Pulse();

	private:
	void CaptureState();

	private:
	void DrawHeaderPanel();
	void DrawChannelPanel (BRect panel, const mapper_audio_channel_debug_t &channel);
	void DrawLevelMeter (BRect rect, uint32_t value, uint32_t maximum);

	private:
	void DrawFixedLine (const char *label, const char *value, float x, float y);
	void DrawStateLine (const char *label, bool value, float x, float y);
	void DrawColoredFixedLine (const char *label, const char *value,
							   rgb_color color, float x, float y);
	void DrawGlobalPanel (BRect panel);

	private:
	const char *ExpansionTypeName() const;

	private:
	PretendoWindow *fParent = nullptr;
	mapper_audio_debug_state_t fState;
};


#endif // _EXPANSION_AUDIO_EXPLORER_VIEW_H_
