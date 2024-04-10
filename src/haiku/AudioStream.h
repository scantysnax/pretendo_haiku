
#ifndef _AUDIO_STREAM_H_
#define _AUDIO_STREAM_H_

#include <SoundPlayer.h>
#include <boost/noncopyable.hpp>

#include "asm/copies.h"

// NOTICE: 
// this code is currently deprecated.  It is left in the tree just in
// case it is needed for some reason, at some point.  Sound is now
// handled by SoundPusher.cc, SoundPusher.h

class AudioStream : public boost::noncopyable 
{
	public:
			AudioStream (float sampleRate, int32 sampleBits, int32 	channels, int32 bufferSize);
	virtual ~AudioStream();
	
	public:
	void Stream (const void *stream, size_t numSamples);
	void Start (void);
	void Stop (void);
	uint8 *SoundBuffer (void) { return fSoundBuffer; };
	
	public:
	void InternalSync (void *buffer, size_t size);
	static void sync_hook (void *cookie, void *buffer, size_t size, 
						   const media_raw_audio_format &format);				   
	private:
	BSoundPlayer *fSoundPlayer;
	sem_id fSemaphore;
	uint32 fWritePosition;
	uint32 fPlayPosition;
	uint32 fBufferTotal;
	uint8 *fSoundBuffer;
	int32 fSampleBits;
	
	private:
	bool fStreaming;
};

#endif // _AUDIO_STREAM_H_
