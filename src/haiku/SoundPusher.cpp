
#include <cstring>
#include <iostream>

#include "SoundPusher.h"



SoundPusher::SoundPusher()
{
	fFrameRate = nes::apu::frequency;
	fInBufferFrameCount = fFrameRate / nes::apu::fps;
	
	memset(&fAudioFormat, 0, sizeof(gs_audio_format));
	fAudioFormat.frame_rate = fFrameRate;
	fAudioFormat.channel_count = 1;
	fAudioFormat.format = gs_audio_format::B_GS_U8;
	fAudioFormat.byte_order = B_MEDIA_LITTLE_ENDIAN; // doesnt' really matter, just here for completeness
	fAudioFormat.buffer_size = fInBufferFrameCount * kBufferCount;
}


SoundPusher::~SoundPusher()
{
	delete fSoundPusher;
}

bool
SoundPusher::Init (void)
{
	fBufferSize = nes::apu::buffer_size;
	fSoundPusher = new BPushGameSound(fInBufferFrameCount, &fAudioFormat, kBufferCount, NULL);
	
	if (fSoundPusher->InitCheck() == B_OK) {
		std::cout << "SoundPusher::InitCheck() -> OK" << std::endl;
		
		void *outBase;
        size_t outSize;
        
        fSoundPusher->LockForCyclic(&outBase, &outSize);
        memset(outBase, 0, outSize);
        fSoundPusher->UnlockCyclic();
        
		std::cout << "SoundPusher::Init() -> buffer cleared" << std::endl;
		return true;
	}
	
	std::cout << "SoundPusher::Init() -> false" << std::endl;;
	return false;
}


bool
SoundPusher::Start (void)
{
	if (fSoundPusher->StartPlaying() != B_OK) {
		std::cout << "SoundPusher::Start() -> fail" << std::endl;
		return false;
	}
	
	std::cout << "SoundPusher::Start() -> OK" << std::endl;
	return true;
}

void
SoundPusher::Stop (void)
{
	void *outBase;
	size_t outSize;
	
	fSoundPusher->LockForCyclic(&outBase, &outSize);
	fSoundPusher->StopPlaying();
	fSoundPusher->UnlockCyclic();
	
	std::cout << "SoundPusher::Stop() -> OK" << std::endl;
}


void 
SoundPusher::UnlockPage(void)
{
	if (fSoundPusher->UnlockPage(fSoundBuffer) != B_OK) {
		std::cout << "SoundPusher UnlockPage() -> fail" << std::endl;
	}
}


void
SoundPusher::LockNextPage (void)
{
	BPushGameSound::lock_status lockStatus;	
	uint8 const *data = nes::apu::sample_buffer_.buffer();
	 
	lockStatus = 
		fSoundPusher->LockNextPage(reinterpret_cast<void **>(&fSoundBuffer), &fBufferSize);
	
	if (lockStatus != BPushGameSound::lock_ok) {
		if (lockStatus == BPushGameSound::lock_ok_frames_dropped) {
			std::cout << "SoundPusher::LockNextPage() -> frames dropped" << std::endl;
		} else {
			std::cout << "SoundPusher::LockNextPage() -> lock failed" << std::endl;
			return;
		}
	}
	
	// maybe this can be optimised?
	size_t pos = 0;
	while (pos < fBufferSize) {
		fSoundBuffer[pos] = data[pos];
		pos++;
	}
}
