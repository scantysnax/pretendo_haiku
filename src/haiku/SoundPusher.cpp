
#include <cstring>
#include <iostream>

#include "SoundPusher.h"


SoundPusher::SoundPusher()
{
	fFrameRate = nes::apu::frequency;
	fInBufferFrameCount = fFrameRate / 60;
	
	memset(&fAudioFormat, 0, sizeof(gs_audio_format));
	fAudioFormat.frame_rate = fFrameRate;
	fAudioFormat.channel_count = 1;
	fAudioFormat.format = gs_audio_format::B_GS_U8;
	fAudioFormat.byte_order = B_MEDIA_LITTLE_ENDIAN; // doesn't matter, just here for completeness
	fAudioFormat.buffer_size = fInBufferFrameCount * kBufferCount;
}


SoundPusher::~SoundPusher()
{
	delete fSoundPusher;
}


bool
SoundPusher::Init()
{
	fBufferSize = nes::apu::buffer_size;
	fSoundPusher = new BPushGameSound(fInBufferFrameCount, &fAudioFormat, kBufferCount, nullptr);
	
	std::cout << __PRETTY_FUNCTION__ << " buffer size: " << fBufferSize << std::endl;
	
	if (fSoundPusher->InitCheck() == B_OK) {
		std::cout << __PRETTY_FUNCTION__  << " OK" << std::endl;
		
		void *outBase;
        size_t outSize;
        
        fSoundPusher->LockForCyclic(&outBase, &outSize);
        memset(outBase, 0, outSize);
        fSoundPusher->UnlockCyclic();
        
		std::cout << __PRETTY_FUNCTION__ << " buffer cleared" << std::endl;
		return true;
	}
	
	std::cout << __PRETTY_FUNCTION__ << " fail" << std::endl;;
	return false;
}


bool
SoundPusher::Start()
{
	if (fSoundPusher->StartPlaying() != B_OK) {
		std::cout << __PRETTY_FUNCTION__ << " fail" << std::endl;
		return false;
	}
	
	std::cout << __PRETTY_FUNCTION__ << " OK" << std::endl;
	return true;
}

void
SoundPusher::Stop()
{
	void *outBase;
	size_t outSize;
	
	fSoundPusher->LockForCyclic(&outBase, &outSize);
	fSoundPusher->StopPlaying();
	fSoundPusher->UnlockCyclic();
	
	std::cout << __PRETTY_FUNCTION__ << " OK" << std::endl;
}


void 
SoundPusher::UnlockPage()
{
	if (fSoundPusher->UnlockPage(fSoundBuffer) != B_OK) {
		std::cout << __PRETTY_FUNCTION__ << " fail" << std::endl;
	}
}


void
SoundPusher::LockNextPage()
{
	BPushGameSound::lock_status lockStatus;	
	lockStatus = fSoundPusher->LockNextPage(reinterpret_cast<void **>(&fSoundBuffer), &fBufferSize);
	
	if (lockStatus != BPushGameSound::lock_ok) {
		if (lockStatus == BPushGameSound::lock_ok_frames_dropped) {
			std::cout << __PRETTY_FUNCTION__ << " frames dropped" << std::endl;
			return;
		} else {
			std::cout << __PRETTY_FUNCTION__ << " lock failed" << std::endl;
			return;
		}
	}
	
	nes::apu::read_samples(fSoundBuffer, fBufferSize);
}
