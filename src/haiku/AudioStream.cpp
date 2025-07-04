
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

	fSoundPlayer = new BSoundPlayer(&format, "Pretendo output", &play_buffer, nullptr, this);
	fWritePosition = 0;
	fPlayPosition = 0;
	fBufferSize = bufferSize * (sampleBits / 8);
	fSoundBuffer = reinterpret_cast<uint8 *>(malloc(fBufferSize));	
	fStreaming = false;
	fMutex = new SimpleMutex("pretendo_sound_mutex");
	
	memset(fSoundBuffer, 0x80, fBufferSize);	
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
		fSoundPlayer->SetHasData (true);
		fSoundPlayer->Start();
		fStreaming = true;
	} else {
		fSoundPlayer->SetHasData(false);
		fStreaming = false;
	}
}


void
AudioStream::Stop()
{
	if (fStreaming) {
		fSoundPlayer->SetHasData(false);
		fStreaming = false;
		fSoundPlayer->Stop();
		fMutex->Unlock();
	}
}


void
AudioStream::Stream (void const *stream, size_t const samples)
{
	if (! fSoundPlayer || fStreaming == false) {
		return;
	}
	
	if (fMutex->Lock()) {
		uint8 const *output = reinterpret_cast<uint8 const *>(stream);
		size_t length = samples * sizeof(uint8);
		size_t position = fWritePosition + length;
		size_t const space = fBufferSize - fWritePosition;
			
		if (position > fBufferSize) {
			mmx_copy(fSoundBuffer + fWritePosition, output, space);
			output += space;
			length -= space;
			mmx_copy(fSoundBuffer, output, length);
			fWritePosition = position - fBufferSize;
		} else {
			mmx_copy(fSoundBuffer + fWritePosition, output, length);
			fWritePosition = position;
		}
	}
}


void
AudioStream::PlayBuffer (void *buffer, size_t const size)
{
	uint8 *output = reinterpret_cast<uint8 *>(buffer);
	size_t length = size * sizeof(uint8);
	size_t position = fPlayPosition + length;
	size_t const space = fBufferSize - fPlayPosition;
		
	if (position > fBufferSize) {
		mmx_copy(output, fSoundBuffer + fPlayPosition, space);
		output += space;
		length -= space;
		mmx_copy(output, fSoundBuffer, length);
		fPlayPosition = position - fBufferSize;
	} else {
		mmx_copy(output, fSoundBuffer + fPlayPosition, length);
		fPlayPosition = position;
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
