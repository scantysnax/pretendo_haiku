
#ifndef _AUDIO_VIEW_H_
#define _AUDIO_VIEW_H_

#include <Window.h>
#include <View.h>
#include <string>

class AudioView : public BView
{
	public:
	AudioView();
	virtual ~AudioView();
	
	public:
	virtual void Draw (BRect updateRect);
	virtual void AttachedToWindow (void);
	
	private:
	uint8 *fAudioBuffer;
};
		

#endif // AUDIO_VIEW_H
