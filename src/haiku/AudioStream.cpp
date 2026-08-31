
#include "AudioStream.h"

#include <cstdio>


// -----------------------------------------------------------------------------
// AudioStream::AudioStream
//
// Creates the MediaKit sound player, allocates the circular host audio buffer,
// initializes the audio pacing mutex, and clears the output buffer to unsigned
// 8-bit silence.
//
// Parameters:
//   sampleRate - Output sample rate requested from BSoundPlayer.
//   sampleBits - Number of bits per audio sample.
//   channels   - Number of output channels.
//   bufferSize - MediaKit callback buffer size in bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
AudioStream::AudioStream(float sampleRate, int32 sampleBits, int32 channels, size_t bufferSize)
{
	media_raw_audio_format format;
	memset(&format, 0, sizeof(format));

	format.frame_rate = sampleRate;
	format.channel_count = channels;
	format.format = media_raw_audio_format::B_AUDIO_UCHAR;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = bufferSize;

	fSoundPlayer = new BSoundPlayer(&format, "Pretendo output", &play_buffer, nullptr, this);
	fBufferSize = bufferSize * (sampleBits / 8);
	fSoundBuffer = reinterpret_cast<uint8 *>(malloc(fBufferSize));
	
	fMutex = new Mutex("pretendo_audio_mutex");

	if (fSoundBuffer) {
		memset(fSoundBuffer, 0x80, fBufferSize);
	}
}


// -----------------------------------------------------------------------------
// AudioStream::~AudioStream
//
// Stops and destroys the MediaKit sound player, releases the audio pacing mutex,
// frees the host audio buffer, and clears owned pointers.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
AudioStream::~AudioStream()
{
	if (fSoundPlayer) {
		fSoundPlayer->Stop();

		delete fSoundPlayer;
		fSoundPlayer = nullptr;
	}

	delete fMutex;
	fMutex = nullptr;

	free(fSoundBuffer);
	fSoundBuffer = nullptr;
}


// -----------------------------------------------------------------------------
// AudioStream::Start
//
// Clears the host audio buffer, drains stale pacing permits, and starts the
// BSoundPlayer stream if the player is ready.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::Start()
{
	ClearBuffer();
	ResetPacing();

	if (fSoundPlayer->InitCheck() == B_OK) {
		fSoundPlayer->SetHasData(true);
		fSoundPlayer->Start();

		fStreaming = true;
	} else {
		fSoundPlayer->SetHasData(false);

		fStreaming = false;
	}
}

// -----------------------------------------------------------------------------
// AudioStream::Stop
//
// Stops MediaKit audio streaming and releases the audio pacing mutex so the
// emulator thread cannot remain blocked waiting for a callback that will no
// longer arrive.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// AudioStream::ClearBuffer
//
// Clears the internal unsigned 8-bit audio ring buffer to silence and resets
// both circular-buffer positions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::ClearBuffer()
{
	if (!fSoundBuffer) {
		return;
	}

	memset(fSoundBuffer, 0x80, fBufferSize);

	fWritePosition = 0;
	fPlayPosition = 0;
}


// -----------------------------------------------------------------------------
// AudioStream::SuspendForDebugger
//
// Stops host audio output while the debugger is single-stepping.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::SuspendForDebugger()
{
	fDebugSuspended = true;
	fMuted = true;

	ClearBuffer();

	if (fSoundPlayer && fStreaming) {
		fSoundPlayer->SetHasData(false);
		fSoundPlayer->Stop();
		fStreaming = false;
	}
}


// -----------------------------------------------------------------------------
// AudioStream::ResumeFromDebugger
//
// Restarts host audio output after debugger stepping.
//
// The host audio buffer and accumulated pacing permits are cleared before
// streaming resumes so stale debugger-era samples or catch-up permits cannot
// cause a burst of unpaced audio immediately after leaving debugger mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::ResumeFromDebugger()
{
	ClearBuffer();
	ResetPacing();

	fMuted = false;
	fDebugSuspended = false;

	if (fSoundPlayer && fSoundPlayer->InitCheck() == B_OK) {
		fSoundPlayer->SetHasData(true);
		fSoundPlayer->Start();
		fStreaming = true;
	}
}


// -----------------------------------------------------------------------------
// AudioStream::ResetPacing
//
// Drains any accumulated pacing permits from the audio semaphore.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::ResetPacing()
{
	sem_id const locker = fMutex->Locker();

	while (acquire_sem_etc(locker, 1, B_RELATIVE_TIMEOUT, 0) == B_OK) {
		// Drain all currently available pacing permits.
	}
}


// -----------------------------------------------------------------------------
// AudioStream::SetMuted
//
// Enables or disables host audio mute.
//
// When mute is enabled, the host ring buffer is cleared to unsigned 8-bit
// silence so stale emulator audio cannot continue to circulate.
//
// Parameters:
//   muted - true to mute host audio.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::SetMuted (bool muted)
{
	fMuted = muted;

	if (muted) {
		ClearBuffer();
	}
}


// -----------------------------------------------------------------------------
// AudioStream::Stream
//
// Writes emulator-generated samples into the host audio ring buffer.
//
// The audio pacing mutex is acquired here and released by PlayBuffer(), which
// preserves the existing audio-paced emulator timing behavior.
//
// Parameters:
//   stream  - Source audio sample buffer.
//   samples - Number of unsigned 8-bit samples to write.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::Stream (void const *stream, size_t const samples)
{
	if (!fSoundPlayer || fStreaming == false || samples == 0) {
		return;
	}

	if (fMutex->Lock()) {
		uint8 const *output = reinterpret_cast<uint8 const *>(stream);
		size_t length = samples * sizeof(uint8);
		size_t const position = fWritePosition + length;
		size_t const space = fBufferSize - fWritePosition;

		if (position > fBufferSize) {
			if (fMuted || !output) {
				memset(fSoundBuffer + fWritePosition, 0x80, space);
				memset(fSoundBuffer, 0x80, length - space);
			} else {
				mmx_copy(fSoundBuffer + fWritePosition, output, space);
				output += space;
				mmx_copy(fSoundBuffer, output, length - space);
			}

			fWritePosition = position - fBufferSize;
		} else {
			if (fMuted || !output) {
				memset(fSoundBuffer + fWritePosition, 0x80, length);
			} else {
				mmx_copy(fSoundBuffer + fWritePosition, output, length);
			}

			fWritePosition = position;
		}
	}
}


// -----------------------------------------------------------------------------
// AudioStream::PlayBuffer
//
// Supplies audio to the MediaKit sound callback.
//
// When muted, unsigned 8-bit silence is returned directly.  While muted, at most
// one pacing permit is retained so callbacks cannot build a large semaphore
// backlog.
//
// During normal playback, one host ring-buffer block is copied to MediaKit and
// the producer pacing semaphore is released.
//
// Parameters:
//   buffer - Destination MediaKit audio buffer.
//   size   - Size of the destination buffer in bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::PlayBuffer(void *buffer, size_t const size)
{
	uint8 *output = reinterpret_cast<uint8 *>(buffer);

	if (!output || size == 0) {
		fMutex->Unlock();
		return;
	}

	if (fMuted) {
		memset(output, 0x80, size);

		int32 semCount = 0;

		if (get_sem_count(fMutex->Locker(), &semCount) == B_OK) {
			if (semCount <= 0) {
				fMutex->Unlock();
			}
		}

		return;
	}

	size_t length = size;
	size_t const position = fPlayPosition + length;
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


// -----------------------------------------------------------------------------
// AudioStream::play_buffer
//
// Static BSoundPlayer callback used to request audio samples from the
// AudioStream instance.  The callback cookie is the AudioStream object passed to
// BSoundPlayer during construction.
//
// Parameters:
//   cookie - AudioStream instance pointer supplied to BSoundPlayer.
//   buffer - Destination buffer that should be filled with audio samples.
//   size   - Number of bytes requested by BSoundPlayer.
//   format - Active raw audio format, unused because AudioStream already owns
//            the stream configuration.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
AudioStream::play_buffer (void *cookie, void *buffer, size_t size,
	const media_raw_audio_format &format)
{
	(void)format;
	
	AudioStream *_this = reinterpret_cast<AudioStream *>(cookie);
	_this->PlayBuffer(buffer, size);
}

