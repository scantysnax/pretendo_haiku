
#ifndef _AUDIO_STREAM_H_
#define _AUDIO_STREAM_H_

#include <SoundPlayer.h>

#include <cstring>

#include "Mutex.h"

#include "asm/copies.h"


class AudioStream 
{
	public:
			AudioStream (float sampleRate, int32 sampleBits, int32 channels, 
						 size_t bufferSize);
	virtual ~AudioStream();
	
	public:
	void Start();
	void Stop();
	void Stream (void const *stream, size_t const samples);
	void SetMuted (bool muted);
	void ClearBuffer();
	void SuspendForDebugger();
	void ResumeFromDebugger();
	void ResetPacing();
	
	private:
	void PlayBuffer (void *buffer, size_t const size);
	static void play_buffer (void *cookie, void *buffer, size_t size, 
							 const media_raw_audio_format &format);				   
	private:
	BSoundPlayer *fSoundPlayer = nullptr;
	size_t fWritePosition = 0;
	size_t fPlayPosition = 0;
	size_t fBufferSize = 0;
	uint8 *fSoundBuffer = nullptr;
	Mutex *fMutex = nullptr;
	bool fDebugSuspended = false;
	
	private:
	bool fMuted = false;
	bool fStreaming = false;
};

#endif // _AUDIO_STREAM_H_
