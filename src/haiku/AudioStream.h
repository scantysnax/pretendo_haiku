
#ifndef _AUDIO_STREAM_H_
#define _AUDIO_STREAM_H_

#include <SoundPlayer.h>

#include <cstring>

#include "Mutex.h"

#include "asm/copies.h"


class AudioStream 
{
	public:
			AudioStream (float sampleRate, size_t sampleBits, size_t channels, 
						 size_t bufferSize);
	virtual ~AudioStream();
	
	public:
	void Start();
	void Stop();
	void Stream (void const *stream, size_t const samples);
	
	private:
	void PlayBuffer (void *buffer, size_t const size);
	static void play_buffer (void *cookie, void *buffer, size_t size, 
							 const media_raw_audio_format &format);				   
	private:
	BSoundPlayer *fSoundPlayer;
	size_t fWritePosition;
	size_t fPlayPosition;
	size_t fBufferSize;
	uint8 *fSoundBuffer;
	Mutex *fMutex;
	
	private:
	bool fStreaming;
};

#endif // _AUDIO_STREAM_H_
