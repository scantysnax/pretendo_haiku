
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

	fSoundPlayer = new BSoundPlayer(
		&format,
		"Pretendo output",
		&play_buffer,
		nullptr,
		this
	);

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
// Clears the internal unsigned 8-bit audio ring buffer to silence.  This does
// not lock the audio pacing mutex.
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
// Drains any accumulated pacing permits from the audio semaphore.  This prevents
// the emulator from running a burst of catch-up frames after debugger pause or
// single-step mode, where the host audio callback may have continued releasing
// the pacing semaphore while normal emulation was not consuming it.
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
// Enables or disables host audio mute.  When mute is enabled, the ring buffer is
// cleared to unsigned 8-bit silence so stale samples from a running ROM cannot
// continue to circulate while entering debugger step mode.
//
// Parameters:
//   muted - true to mute host audio output.
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
// Writes emulator-generated samples into the host audio ring buffer.  The audio
// mutex is intentionally unlocked by PlayBuffer(); this preserves the original
// audio-paced emulator timing behavior.
//
// When muted, this still waits on the pacing mutex, but writes unsigned 8-bit
// silence instead of emulator audio so stale music cannot be queued during
// debugger stepping.
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
// Supplies audio to the MediaKit sound player.  When muted, unsigned 8-bit
// silence is written directly to the MediaKit output buffer.  The audio mutex is
// unlocked here to preserve the original audio-paced emulator timing behavior.
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
		fMutex->Unlock();
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

