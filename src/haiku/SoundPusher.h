
#ifndef _SOUND_PUSHER_H_
#define _SOUND_PUSHER_H_

#include <GameSoundDefs.h>
#include <PushGameSound.h>
#include <MediaDefs.h>

#include "Apu.h"
#include "Nes.h"


int32 const kBufferCount = 4;

class SoundPusher
{
	public:
	SoundPusher();
	virtual ~SoundPusher();
	
	public:
	bool Init (void);
	bool Start (void);
	void Stop (void);
	void LockNextPage(void);
	void UnlockPage (void);
	
	private: 
	size_t fFrameRate;
	size_t fInBufferFrameCount;
	gs_audio_format fAudioFormat;
	uint8 *fSoundBuffer;
	size_t fBufferSize;

	private:
	BPushGameSound *fSoundPusher;
};


#endif // _SOUND_PUSHER_H_
