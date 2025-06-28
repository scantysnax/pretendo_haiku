
#include "AudioStream.h"


AudioStream::AudioStream (float sampleRate, size_t sampleBits, size_t channels, size_t bufferSize)
{	
	media_raw_audio_format format;
	memset(&format, 0, sizeof(format));
	
	format.frame_rate = sampleRate;
	format.channel_count = channels;
	format.format = media_raw_audio_format::B_AUDIO_UCHAR;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = bufferSize;

	fSoundPlayer = new BSoundPlayer (&format, "Pretendo output", &play_buffer, nullptr, this);
	fWritePosition = 0;
	fPlayPosition = 0;
	fBufferTotal = bufferSize * sampleBits / 8;
	fSoundBuffer = reinterpret_cast<uint8 *>(malloc(fBufferTotal));	
	fStreaming = false;
	fMutex = new SimpleMutex("pretendo_sound_mutex");
	
	memset(fSoundBuffer, 0x80, fBufferTotal);	
}


AudioStream::~AudioStream()
{
	if (fSoundPlayer) {
		fSoundPlayer->Stop();
		delete fSoundPlayer; 
		delete fMutex;
	}
	
	free(fSoundBuffer);
	fSoundBuffer = nullptr;
}


void
AudioStream::Start()
{
	if (fSoundPlayer->InitCheck() == B_OK) {
		fSoundPlayer->Start();
		fSoundPlayer->SetHasData (true);
		fStreaming = true;
	} else {
		fStreaming = false;
		fSoundPlayer->SetHasData(false);
	}
}


void
AudioStream::Stop()
{
	if (fStreaming) {
		fStreaming = false;
		fSoundPlayer->Stop();
		fSoundPlayer->SetHasData(false);
		fMutex->Unlock();
	}
}


void
AudioStream::Stream (void const *stream, size_t samples)
{
	if (! fSoundPlayer || fStreaming == false) {
		return;
	}
	
	if (fMutex->Lock() == B_OK) {
		const uint8 *out = reinterpret_cast<const uint8 *>(stream);
		size_t len = samples * sizeof(uint8);
		size_t pos = fWritePosition + len;
		size_t space = fBufferTotal - fWritePosition;
			
		if (pos > fBufferTotal) {
			mmx_copy(fSoundBuffer + fWritePosition, out, space);
			out += space;
			len -= space;
			mmx_copy (fSoundBuffer, out, len);
			fWritePosition = pos - fBufferTotal;
		} else {
			mmx_copy(fSoundBuffer + fWritePosition, out, len);
			fWritePosition = pos;
		}
	}
}


void
AudioStream::PlayBuffer (void *buffer, size_t size)
{
	uint8 *out = reinterpret_cast<uint8 *>(buffer);
	size_t len = size * sizeof(uint8);
	size_t pos = fPlayPosition + len;
	size_t space = fBufferTotal - fPlayPosition;
		
	if (pos > fBufferTotal) {
		mmx_copy(out, fSoundBuffer + fPlayPosition, space);
		out += space;
		len -= space;
		mmx_copy(out, fSoundBuffer, len);
		fPlayPosition = pos - fBufferTotal;
	} else {
		mmx_copy(out, fSoundBuffer + fPlayPosition, len);
		fPlayPosition = pos;
	}

	fMutex->Unlock();
}


void
AudioStream::play_buffer (void *cookie, void *buffer, size_t size, 
	const media_raw_audio_format &format)
{
	(void)format;
	
	AudioStream *_this = reinterpret_cast<AudioStream *>(cookie);
	_this->PlayBuffer(buffer, size);	
}
