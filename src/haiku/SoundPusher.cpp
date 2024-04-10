
#include <cstdio>
#include <cstring>

#include "SoundPusher.h"
#include "asm/copies.h"


SoundPusher::SoundPusher()
	: fFrameRate(48000)
{
	
	fInBufferFrameCount = fFrameRate / 60;
	
	memset(&fAudioFormat, 0, sizeof(gs_audio_format));
	fAudioFormat.frame_rate = fFrameRate;
	fAudioFormat.channel_count = 1;
	fAudioFormat.format = gs_audio_format::B_GS_U8;
	fAudioFormat.buffer_size = fInBufferFrameCount * 4;
}


SoundPusher::~SoundPusher()
{
	
}


bool
SoundPusher::Init (void)
{
	fBufferSize = nes::apu::buffer_size;
	fSoundPusher = new BPushGameSound(fInBufferFrameCount, &fAudioFormat, 4, NULL);
	
	if (fSoundPusher->InitCheck() == B_OK) {
		puts("SoundPusher::InitCheck() -> OK");
		
		void *outBase;
        size_t outSize;
        
        fSoundPusher->LockForCyclic(&outBase, &outSize);
        memset(outBase, 0, outSize);
        fSoundPusher->UnlockCyclic();
		
		puts("SoundPusher::Init() -> buffer cleared");
		return true;
	}
	
	puts("SoundPusher::Init() -> false");
	return false;
}


bool
SoundPusher::Start (void)
{
	if (fSoundPusher->StartPlaying() != B_OK) {
		puts("SoundPusher::Start() -> fail");
		return false;
	}
	
	puts("SoundPusher::Start() -> OK");
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
	
	puts("SoundPusher::Stop() -> OK");
}


void 
SoundPusher::UnlockPage(void)
{
	if (fSoundPusher->UnlockPage(fSoundBuffer) != B_OK) {
		puts("SoundPusher UnlockPage() -> fail");
	}
}


void
SoundPusher::LockNextPage (void)
{
	BPushGameSound::lock_status lockStatus;	
	uint8 const *data = nes::apu::buffer();
	 
	lockStatus = 
		fSoundPusher->LockNextPage(reinterpret_cast<void **>(&fSoundBuffer), &fBufferSize);
	
	if (lockStatus != BPushGameSound::lock_ok) {
		if (lockStatus == BPushGameSound::lock_ok_frames_dropped) {
			printf("SoundPusher::LockNextPage() -> frames dropped");
		} else {
			printf("SoundPusher::LockNextPage() -> lock failed");
			return;
		}
	}

	mmx_copy(fSoundBuffer, data, fBufferSize);
}
