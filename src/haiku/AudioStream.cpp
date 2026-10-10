
#include "AudioStream.h"


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
// Prepares emulator audio playback.
//
// The MediaKit sound player is started only once. Subsequent emulator
// Stop/Start transitions switch between silence and generated audio without
// repeatedly stopping and restarting the host audio device.
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
	fMuted = true;
	fStreaming = false;

	ClearBuffer();
	ResetPacing();

	if (!fSoundPlayer || fSoundPlayer->InitCheck() != B_OK) {
		if (fSoundPlayer) {
			fSoundPlayer->SetHasData(false);
		}

		return;
	}

	if (!fPlayerStarted) {
		fSoundPlayer->SetHasData(true);
		fSoundPlayer->Start();

		fPlayerStarted = true;
	}

	fStreaming = true;
	fMuted = false;
}


// -----------------------------------------------------------------------------
// AudioStream::Stop
//
// Stops emulator audio production without stopping the MediaKit sound player.
//
// The host callback remains active and supplies unsigned 8-bit silence while
// emulation is stopped. This avoids repeatedly restarting the host audio device,
// which was found to contribute substantially to the startup pop.
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
	fMuted = true;
	fStreaming = false;

	ClearBuffer();

	if (fMutex) {
		fMutex->UnlockIfNeeded();
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
		// drain all currently available pacing permits.
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

		while (length != 0) {
			const size_t space = fBufferSize - fWritePosition;
			const size_t amount = (length < space) ? length : space;

			if (amount != 0) {
				if (fMuted || !output) {
					memset(fSoundBuffer + fWritePosition, 0x80, amount);
				} else {
					mmx_copy(fSoundBuffer + fWritePosition, output, amount);
					output += amount;
				}

				fWritePosition += amount;
				length -= amount;
			}

			if (fWritePosition == fBufferSize) {
				fWritePosition = 0;
			}
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
AudioStream::PlayBuffer (void *buffer, size_t const size)
{
	uint8 *output = reinterpret_cast<uint8 *>(buffer);
		
	if (!output || size == 0) {
		fMutex->Unlock();
		return;
	}

	if (fMuted) {
		memset(output, 0x80, size);
		fMutex->UnlockIfNeeded();
		return;
	}

	size_t length = size;

	while (length != 0) {
		const size_t available = fBufferSize - fPlayPosition;
		const size_t amount = (length < available) ? length : available;

		if (amount != 0) {
			mmx_copy(output, fSoundBuffer + fPlayPosition, amount);
			output += amount;
			fPlayPosition += amount;
			length -= amount;
		}

		if (fPlayPosition == fBufferSize) {
			fPlayPosition = 0;
		}
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

