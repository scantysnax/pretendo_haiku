
// nes stuff
#include "Apu.h"
#include "Cart.h"
#include "Input.h"
#include "Mapper.h"
#include "Nes.h"
#include "Palette.h"
#include "Reset.h"


// ui and other things
#include "AudioStream.h"
#include "Controller.h"
#include "Mutex.h"
#include "PaletteWindow.h"
#include "PretendoView.h"
#include "PretendoWindow.h"
#include "ROMFilePanel.h"
#include "ROMInfoWindow.h"
#include "VideoScreen.h"


// use mmx blitters and memcpy()
#include "asm/blitters.h"
#include "asm/copies.h"


PretendoWindow::PretendoWindow()
	: BDirectWindow (BRect (0, 0, 0, 0), "Pretendo", B_TITLED_WINDOW, B_NOT_RESIZABLE, 0)		
{
	// ui things
	AddMenu();
	ResizeTo(screen_size::WIDTH, screen_size::HEIGHT);
	CenterOnScreen();
	
	BRect bounds(Bounds());
	bounds.OffsetTo(B_ORIGIN);
	bounds.top = fMenuHeight;
	fView = new PretendoView(bounds, this);
	AddChild(fView);
	
	// setup video buffers
	void *bitsArea;
	void *dirtyArea;
	
	fBitsArea = create_area("pretendo_frame_buffer", &bitsArea, B_ANY_ADDRESS,
					((screen_size::WIDTH * 2) * (screen_size::HEIGHT * 2) * 4 + 
						B_PAGE_SIZE-1) & ((uint32)-1 ^ (B_PAGE_SIZE-1)), B_NO_LOCK,
						B_READ_AREA | B_WRITE_AREA);
					
	fDirtyArea = create_area("pretendo_dirty_buffer", &dirtyArea, B_ANY_ADDRESS,
					((screen_size::WIDTH * 2) * (screen_size::HEIGHT * 2) * 4 +
						B_PAGE_SIZE-1) & ((uint32)-1 ^ (B_PAGE_SIZE-1)), B_NO_LOCK, 
						B_READ_AREA | B_WRITE_AREA);
					
	if (fBitsArea < B_OK || fDirtyArea < B_OK) {
		(new BAlert("Error", "Can't allocate video buffers.  Quitting.",
			"Sorry", nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	} else {
		memset(bitsArea, 0x0, (screen_size::WIDTH*2) * (screen_size::HEIGHT*2) * 4);
		memset(dirtyArea, 0xff, (screen_size::WIDTH*2) * (screen_size::HEIGHT*2) * 4);
		
		fBackBuffer.bits = reinterpret_cast<uint8 *>(bitsArea);
		fDirtyBuffer.bits = reinterpret_cast<uint8 *>(dirtyArea);
	}
	
	// setup BBitmap.  keep it contiguous in memory
	fBitmap = new BBitmap(BRect(0, 0, screen_size::WIDTH-1, 
		screen_size::HEIGHT-1), B_CMAP8, false, true);
	
	if (! fBitmap || ! fBitmap->IsValid()) {
		(new BAlert("Error", "Can't create video bitmap.  Quitting.","Sorry", 
					nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	} else {
		fBitmapBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
		ClearBitmap(false);
	}

	// setup a BBitmap for overlay framework (checks for overlay support inherently)
	// this will always fail until we get hardware accelerated video
	bool overlayOK = false;

	bounds.Set(0, 0, screen_size::WIDTH-1, screen_size::HEIGHT-1);
	fOverlayBitmap = new BBitmap(bounds, B_BITMAP_WILL_OVERLAY, B_YCbCr422);
	overlayOK = fOverlayBitmap && fOverlayBitmap->IsValid();

	if (overlayOK) {
		fVideoMenu->ItemAt(2)->SetEnabled(true);
		fOverlayBits = reinterpret_cast<uint8 *>(fOverlayBitmap->Bits());
		ClearBitmap(true);
	} else {
		fOverlayBits = nullptr;
		if (fOverlayBitmap) {
			delete fOverlayBitmap;
		}
		
		fView->SetViewColor(0, 0, 0);
		ClearBitmap(false);
		fVideoMenu->ItemAt(2)->SetEnabled(false);
	}
	
	// start video
	fDirectConnected = 
	fFullScreen = 
	fFrameworkChanging = false;	
	fFramework = 
	fPrevFramework = video_framework::NONE;
	fDoubled = false;
	fClear = 0;
	
	if (overlayOK) {
		ChangeFramework(video_framework::OVERLAY);
	} else {
		// make sure we can use windowed mode.  if not default to bitmap framework
		if (BDirectWindow::SupportsWindowMode() == false) {
			fVideoMenu->ItemAt(video_framework::DIRECT)->SetEnabled(false);
			ChangeFramework(video_framework::BITMAP);
		} else {
			// there will be mouse "trails" on the BDirectWindow
			// until we get a hardware cursor. 
			// this is not ideal, so default to bitmap framework
			// but making it available for testing
			
			//ChangeFramework(video_framework::DIRECT);
			ChangeFramework(video_framework::BITMAP);
		}
	}
	
	// we can't change to full screen yet
	fVideoMenu->ItemAt(video_framework::FULLSCREEN)->SetEnabled(false);
	
	memset(&fKeyStates, 0, sizeof (key_info));
	fOpenPanel = new ROMFilePanel;
	
	// sound
	// we don't need to upscale the buffer size if using the MediaKit, so divide it out
	fAudioStream = new AudioStream(nes::apu::frequency, 8, 1, nes::apu::buffer_size / 4);

	// this is the emulator processing loop
	// thread gets a cheeky name, as per the Be Book
	char const *threadNames[] = {
		"pocket calculator",
		"keystroke logger", 
		"bitcoin miner", 
		"password finder",
		"nsa surveillance thread",
		"aes encryption cracker",
		"network traffic monitor",
		"prime finder",
		"mersenne twister",
		"fibonacci sequence generator"
	};

	int32 const index = (rand() % 10);
	fThread = spawn_thread(emulator_thread, threadNames[index], B_DISPLAY_PRIORITY, 
				reinterpret_cast<void *>(this));
	if (fThread < B_OK) {
		// we couldn't spawn the main thread, party over.  everyone go home
		(new BAlert("Error", "Couldn't spawn main thread.  Quitting.", "Sorry",
		 nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		
		be_app->PostMessage(B_QUIT_REQUESTED);
	} else {
		fRunning = false;
	}
	
	// we need a mutual exclusion to protect threaded code
	fMutex = new Mutex("pretendo_mutex");
	fMutex->Lock();
	resume_thread(fThread);
	
	SetDefaultPalette();
}
#include <iostream>

PretendoWindow::~PretendoWindow()
{	
	// break everything down and clean up
	fRunning = fDirectConnected = false;
	fThread = B_BAD_THREAD_ID;
	
	if (fOpenPanel->Window()) {
		fOpenPanel->Window()->Lock();
		fOpenPanel->Window()->Quit();
	}
	
	if (fBitmap->IsValid()) {
		delete fBitmap;
	}
	
	if (fOverlayBitmap->IsValid()) {
		delete fOverlayBitmap;
	}
	
	delete_area(fBitsArea);
	delete_area(fDirtyArea);

	fAudioStream->Stop();
	delete fAudioStream;
	
	
	if (fROMInfoWindow != nullptr) {
		if (fROMInfoWindow->Lock()) {
			fROMInfoWindow->Quit();
		}
	}
	
	
	if (fPaletteWindow != nullptr) {
		if (fPaletteWindow->Lock()) {
			fPaletteWindow->Quit();
		}
	}
	
	
	if (fPatternTable1Window != nullptr) {
		if (fPatternTable1Window->Lock()) {
			fPatternTable1Window->Quit();
		}
	}

	if (fPatternTable2Window != nullptr) {
		if (fPatternTable2Window->Lock()) {
			fPatternTable2Window->Quit();
		}
	}
	
	if (fNameTable1Window != nullptr) {
		if (fNameTable1Window->Lock()) {
			fNameTable1Window->Quit();
		}
	}
	
	if (fNameTable2Window != nullptr) {
		if (fNameTable2Window->Lock()) {
			fNameTable2Window->Quit();
		}
	}
	
	if (fNameTable3Window != nullptr) {
		if (fNameTable3Window->Lock()) {
			fNameTable3Window->Quit();
		}
	}
	
	if (fNameTable4Window != nullptr) {
		if (fNameTable4Window->Lock()) {
			fNameTable4Window->Quit();
		}
	}
	
	fMutex->Unlock();
	
	Hide();
	Sync();	
}


void
PretendoWindow::DirectConnected (direct_buffer_info *info)
{	
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
			fClear = 5;
			fClipInfo.bounds = info->window_bounds;
			fClipInfo.bounds.top += fMenuHeight + 1;
	
			if (fFramework == video_framework::DIRECT) {
				SetFrontBuffer(reinterpret_cast<uint8 *>(info->bits)
					+ (fClipInfo.bounds.top * info->bytes_per_row), info->pixel_format,
					info->bits_per_pixel / 8, info->bytes_per_row);
			}
		
			fClipInfo.clip_list = nullptr;
			fDirectConnected = true;	// ready to go
		
		// intentional fall through //	
		
		case B_DIRECT_MODIFY:
			// (re)calculate clipping rects
			fClear = 5;
			fClipInfo.clip_count = info->clip_list_count;
			fClipInfo.clip_list = 
				reinterpret_cast<clipping_rect *>(realloc(fClipInfo.clip_list, 
				fClipInfo.clip_count * sizeof(clipping_rect)));
	
			memcpy(fClipInfo.clip_list, info->clip_list,
				fClipInfo.clip_count * sizeof(clipping_rect));
					
			for (int32 i = 0; i < fClipInfo.clip_count; i++) {
				if (fClipInfo.clip_list[i].top <= 
					(info->window_bounds.top + fMenuHeight)) {
					fClipInfo.clip_list[i].top = info->window_bounds.top + fMenuHeight;
				}
			}
			break;
			
		case B_DIRECT_STOP:
			// we're done.  disconnect and free clip list
			fDirectConnected = false;
			free(fClipInfo.clip_list);
			break;
	}
	
	BDirectWindow::DirectConnected(info);
}


void
PretendoWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case messages::DRAW_BITMAP:
			// this has to go here, since the window is apparently guaranteed to be locked
			fView->DrawBitmap(fBitmap, fView->Bounds());
			break;
		
		case messages::CHANGE_RENDER:
			ChangeFramework(
				static_cast<video_framework>(fVideoMenu->IndexOf(fVideoMenu->FindMarked())));
			break;
		
		case messages::LEAVE_FULLSCREEN:
			ChangeFramework(fPrevFramework);
		 	break;
			
		case messages::ROM_LOADED:
			OnLoadROM(message);
			break;
			
		case messages::SHOW_OPEN:
			fOpenPanel->Show();
			break;
			
		case B_REFS_RECEIVED:
			be_app->PostMessage(message);
			break;

		case messages::FREE_ROM:
			OnFreeROM();
			break;
			
		case messages::ROM_INFO:
			OnROMInfo();
			break;
			
		case messages::SHOW_ABOUT:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
			
		case messages::QUIT:
			OnQuit();
			break;
			
		case messages::CPU_RUN:
			OnRun();
			break;
			
		case messages::CPU_STOP:
			OnStop();
			break;
			
		case messages::CPU_PAUSE:
			OnPause();
			break;
						
		case messages::RST_SOFT:
			OnSoftReset();
			break;
			
		case messages::RST_HARD:
			OnHardReset();
			break;
				
		case messages::CFG_INPUT:
			OnConfigureInput();
			break;
			
		case messages::ADJ_PALETTE:
			OnAdjustPalette();
			break;
			
		case messages::SHOW_PTNTBL1:			
			OnViewPatternTable1();
			break;
			
		case messages::SHOW_PTNTBL2:
			OnViewPatternTable2();
			break;
		
		case messages::SHOW_NTBL1:
			OnViewNameTable1();
			break;
			
		case messages::SHOW_NTBL2:
			OnViewNameTable2();
			break;
			
		case messages::SHOW_NTBL3:
			OnViewNameTable3();
			break;
		
		case messages::SHOW_NTBL4:
			OnViewNameTable4();
			break;
			
		case messages::ENABLE_SQ1:
			OnAudioSquare1();
			break;
			
		case messages::ENABLE_SQ2:
			OnAudioSquare2();
			break;
			
		case messages::ENABLE_TRI:
			OnAudioTriangle();
			break;
			
		case messages::ENABLE_NOISE:
			OnAudioNoise();
			break;
			
		case messages::ENABLE_DMC:
			OnAudioDMC();
			break;
		}
	
	BDirectWindow::MessageReceived (message);
}


void
PretendoWindow::WindowActivated (bool flag)
{
	BDirectWindow::WindowActivated (flag);	
}


void
PretendoWindow::MenusBeginning()
{	
	// set up recently opened ROM menu, we keep 5 most recent
	// item list seems to be off by 1
	int32 const recentItems = 5+1;
	BMenu *menu = BRecentFilesList::NewFileListMenu("Load ROM" B_UTF8_ELLIPSIS,
				  nullptr, nullptr, this->PreferredHandler(), recentItems, false, nullptr, 0, 
				  "application/x-vnd.scantysnax-Pretendo");
	
	fFileMenu->AddItem(new BMenuItem(menu, new BMessage(messages::SHOW_OPEN)), 0);
	
	BDirectWindow::MenusBeginning();
}


void
PretendoWindow::MenusEnded()
{	
	// remove the recent files list
	fFileMenu->RemoveItem(static_cast<int32>(0)); // keep this 32-bit friendly
	
	BDirectWindow::MenusEnded();
}


bool
PretendoWindow::QuitRequested()
{	
	// this code is *super* sensitive
	// kill the mutual exclusion, and wait for the thread to stop
	// do not touch this code.
	// do not look at it.
	// do not even breathe on it.		
	status_t ret;

	delete fMutex;
	wait_for_thread(fThread, &ret);
	
	fRunning = 
	fDirectConnected = false;
		
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
PretendoWindow::ResizeTo (float width, float height)
{
	height += fMenuHeight;//+1;	// account for menubar height
	
	BDirectWindow::ResizeTo (width, height);
}


void
PretendoWindow::Zoom (BPoint origin, float width, float height)
{
	(void)origin;
	(void)width;
	(void)height;
	
	float const w = Bounds().Width();
			
	if (w == screen_size::WIDTH) {
		ResizeTo((screen_size::WIDTH*2), (screen_size::HEIGHT*2));
		fDoubled = true;
	} else if (w == screen_size::WIDTH*2) {
		ResizeTo(screen_size::WIDTH, screen_size::HEIGHT);
		fDoubled = false;
	} 
	
	// do not call the default //
}


void
PretendoWindow::AddMenu()
{
	fMenu = new BMenuBar(BRect(0, 0, 0, 0), "pretendo_menu");
	fMenu->ResizeToPreferred();
	AddChild(fMenu);
	
	fFileMenu = new BMenu("File");
	fMenu->AddItem(fFileMenu);
	
	fEmuMenu = new BMenu("Emulator");
	fMenu->AddItem(fEmuMenu);
	
	fToolMenu = new BMenu("Tools");
	fMenu->AddItem(fToolMenu);

	fFileMenu->AddItem(new BMenuItem("Free ROM", new BMessage(messages::FREE_ROM)));
	fFileMenu->AddItem(new BMenuItem("ROM Info", new BMessage(messages::ROM_INFO)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem (new BMenuItem("About" B_UTF8_ELLIPSIS, new BMessage(messages::SHOW_ABOUT)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("Quit", new BMessage(messages::QUIT)));
	
	fEmuMenu->AddItem(new BMenuItem("Start", new BMessage(messages::CPU_RUN)));
	fEmuMenu->AddItem(new BMenuItem("Pause", new BMessage(messages::CPU_PAUSE)));
	fEmuMenu->AddItem(new BMenuItem("Stop", new BMessage(messages::CPU_STOP)));
	fEmuMenu->AddSeparatorItem();
	fEmuMenu->AddItem(new BMenuItem("Reset (soft)", new BMessage(messages::RST_SOFT)));
	fEmuMenu->AddItem(new BMenuItem("Reset (hard)", new BMessage(messages::RST_HARD)));
	fEmuMenu->AddSeparatorItem();
	
	fVideoMenu = new BMenu("Video");
	fEmuMenu->AddItem(fVideoMenu);
	fVideoMenu->AddItem(new BMenuItem("None", new BMessage (messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("Bitmap", new BMessage(messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("Overlay", new BMessage(messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("DirectWindow", new BMessage(messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("WindowScreen", new BMessage(messages::CHANGE_RENDER), 'F'));
	fVideoMenu->SetRadioMode(true);
	
	fAudioMenu = new BMenu("Audio");
	fEmuMenu->AddItem(fAudioMenu);
	fAudioMenu->AddItem(new BMenuItem("Square 1", new BMessage(messages::ENABLE_SQ1)));
	fAudioMenu->AddItem(new BMenuItem("Square 2", new BMessage(messages::ENABLE_SQ2)));
	fAudioMenu->AddItem(new BMenuItem("Triangle", new BMessage(messages::ENABLE_TRI)));
	fAudioMenu->AddItem(new BMenuItem("Noise", new BMessage(messages::ENABLE_NOISE)));
	fAudioMenu->AddItem(new BMenuItem("DMC/DPCM", new BMessage(messages::ENABLE_DMC)));
	
	(fAudioMenu->ItemAt(nes::apu::sound_channel::SQUARE1))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::SQUARE2))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::TRIANGLE))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::NOISE))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::DPCM))->SetMarked(true);
	
	fEmuMenu->AddItem(new BMenuItem("Input" B_UTF8_ELLIPSIS, new BMessage(messages::CFG_INPUT)));
	
	fToolMenu->AddItem(new BMenuItem("Adjust Palette" B_UTF8_ELLIPSIS, new BMessage(messages::ADJ_PALETTE)));
	fToolMenu->AddSeparatorItem();
	fPatternTableMenu = new BMenu("View Pattern Tables");
	fPatternTableMenu->AddItem(new BMenuItem("1 (0x0-0xfff)", new BMessage(messages::SHOW_PTNTBL1)));
	fPatternTableMenu->AddItem(new BMenuItem("2 (0x1000-0x1fff)", new BMessage(messages::SHOW_PTNTBL2)));
	fToolMenu->AddItem(fPatternTableMenu);
	fNameTableMenu = new BMenu("View Name Tables");
	fNameTableMenu->AddItem(new BMenuItem("1 (0x2000-0x23ff)", new BMessage(messages::SHOW_NTBL1)));
	fNameTableMenu->AddItem(new BMenuItem("2 (0x2400-0x27ff)", new BMessage(messages::SHOW_NTBL2)));
	fNameTableMenu->AddItem(new BMenuItem("3 (0x2800-0x2bff)", new BMessage(messages::SHOW_NTBL3)));
	fNameTableMenu->AddItem(new BMenuItem("4 (0x2c00-0x2fff)", new BMessage(messages::SHOW_NTBL4)));
	fToolMenu->AddItem(fNameTableMenu);

	fMenuHeight = fMenu->Bounds().IntegerHeight();
		
	SetKeyMenuBar(fMenu);
}


void
PretendoWindow::OnLoadROM (BMessage *message)
{
	BString path;
	
	if (message->FindString ("rom_path", &path) == B_OK) {
		OnFreeROM();
		if (nes::cart.load(path.String()) == false) {
			(new BAlert("Error", "Error.  Couldnt't load ROM Image.", "Okay", nullptr, nullptr,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			return;
		}
	}
}


void
PretendoWindow::OnFreeROM()
{	
	OnStop();
	nes::cart.unload();
}


void
PretendoWindow::OnROMInfo()
{
	if (fROMInfoWindow && fROMInfoWindow->Lock()) {
		fROMInfoWindow->Quit();
		fROMInfoWindow = nullptr;
	}
	
	if (nes::cart.mapper() != nullptr) { // && ! fROMInfoWindow) {
		fROMInfoWindow = new ROMInfoWindow();
		fROMInfoWindow->Show();
	}
}

void
PretendoWindow::OnQuit()
{	
	// seeya!
	be_app->PostMessage(B_QUIT_REQUESTED);
}


void
PretendoWindow::OnRun()
{	
	if (! fRunning) {
		// make sure we have a cart loaded
		if (nes::cart.mapper()) {
			reset(nes::Reset::Hard);
			fMutex->Unlock(); // unlock the mutual exclusion
			fRunning = true;  // signal the thread that we're running
			fAudioStream->Start();
			fVideoMenu->ItemAt(video_framework::FULLSCREEN)->SetEnabled(true); // we can now go fullscreen
		}
	} else if (fPaused) {
		OnPause();
	}
}


void
PretendoWindow::OnStop()
{		
	// if we're running, set fRunning to false to signal the thread we're not
	// running, lock the mutual exclusion and stop the sound stream
	if (fRunning) {
		fRunning = false;
		
		if (! fPaused) {
			fMutex->Lock();
			fAudioStream->Stop();
		}

		// clear the window contents
		if (fFramework == video_framework::OVERLAY) {
			ClearBitmap(true);
		} else {
			ClearBitmap(false);
			fView->SetViewColor(0, 0, 0);
			fView->Invalidate();
		}
		
		// make sure we can't go fullscreen
		fVideoMenu->ItemAt(video_framework::FULLSCREEN)->SetEnabled(false);
	}
	
	fPaused = false;
	fEmuMenu->ItemAt(1)->SetMarked(false);
}


void
PretendoWindow::OnPause()
{	
	if (fRunning) {
		if (fPaused) {
			// if we are paused, we want to unpause, so lock the mutual exclusion
			// update the recent roms menu and start the sound interface
			fMutex->Unlock();
			fEmuMenu->ItemAt(1)->SetMarked(false);
			fAudioStream->Start();
		} else {
			// otherwise, we want to pause, so lock the mutual exclusion
			// mark the menu accordingly and stop the sound interface
			fMutex->Lock();
			fEmuMenu->ItemAt(1)->SetMarked(true);
			fAudioStream->Stop();
		}
	
		fPaused = !fPaused;
	}
}


void
PretendoWindow::OnSoftReset()
{
	reset(nes::Reset::Soft);
}


void
PretendoWindow::OnHardReset()
{
	reset(nes::Reset::Hard);
}


void
PretendoWindow::OnConfigureInput()
{
	if (fInputWindow && fInputWindow->Lock()) {
		fInputWindow->Quit();
		fInputWindow = nullptr;
	}
		
	fInputWindow = new InputWindow(this);
	fInputWindow->Show();	
}


void
PretendoWindow::OnAdjustPalette()
{
	puts(__PRETTY_FUNCTION__);

	if (fPaletteWindow && fPaletteWindow->Lock()) {
		fPaletteWindow->Quit();
		fPaletteWindow = nullptr;
	} 
	
	fPaletteWindow = new PaletteWindow(this);
	fPaletteWindow->Show();

#if 0
	if (fPaletteWindow != nullptr) {
		fPaletteWindow->Activate();
	} else {
		fPaletteWindow = new PaletteWindow(this);
		fPaletteWindow->Show();
	}
	if (fPaletteWindow == nullptr) {
		fPaletteWindow = new PaletteWindow(this);
	}
	
	fPaletteWindow->Show();
#endif
}

void
PretendoWindow::OnViewPatternTable1()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}

	if (fPatternTable1Window && fPatternTable1Window->Lock()) {
		fPatternTable1Window->Quit();
		fPatternTable1Window = nullptr;
	} 
	
	fPatternTable1Window = new PatternTableWindow(this, 0);
	fPatternTable1Window->Show();

#if 0
	if (fPatternTable0Window != nullptr) {
		fPatternTable0Window->Activate();
	} else {
		fPatternTable0Window = new PatternTableWindow(this, 0);
		fPatternTable0Window->Show();
	}
	
	if (fPatternTable0Window == nullptr) {
		fPatternTable0Window = new PatternTableWindow(this, 0);
	}
	
	fPatternTable0Window->Show();
#endif
}

void
PretendoWindow::OnViewPatternTable2()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}

	if (fPatternTable2Window && fPatternTable2Window->Lock()) {
		fPatternTable2Window->Quit();
		fPatternTable2Window = nullptr;
	} 
	
	fPatternTable2Window = new PatternTableWindow(this, 1);
	fPatternTable2Window->Show();
}

void
PretendoWindow::OnViewNameTable1()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}
	
#if 0
		//if (fNameTable0Window && fNameTable0Window->Lock()) {
		//fNameTable0Window->Quit();
		//fNameTable0Window = nullptr;
	//}
	
	//if (nes::cart.mapper() != nullptr) { // && ! fROMInfoWindow) {
	//	fNameTable0Window = new NameTableWindow(this, 0);
	//	fNameTable0Window->Show();
	//}
#endif
}

void
PretendoWindow::OnViewNameTable2()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}
}

void
PretendoWindow::OnViewNameTable3()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}
}


void
PretendoWindow::OnViewNameTable4()
{
	puts(__PRETTY_FUNCTION__);
	
	if (! nes::cart.mapper()) {
		return;
	}
}


void
PretendoWindow::OnAudioSquare1()
{
	bool marked = fAudioMenu->ItemAt(nes::apu::SQUARE1)->IsMarked();
	marked = ! marked;
	(fAudioMenu->ItemAt(nes::apu::SQUARE1))->SetMarked(marked);
	
	if (marked) {
		nes::apu::unmute_channel(nes::apu::SQUARE1);
	} else {
		nes::apu::mute_channel(nes::apu::SQUARE1);
	}
}


void
PretendoWindow::OnAudioSquare2()
{	
	bool marked = fAudioMenu->ItemAt(nes::apu::SQUARE2)->IsMarked();
	marked = ! marked;
	(fAudioMenu->ItemAt(nes::apu::SQUARE2))->SetMarked(marked);
	
	if (marked) {
		nes::apu::unmute_channel(nes::apu::SQUARE2);
	} else {
		nes::apu::mute_channel(nes::apu::SQUARE2);
	}
}


void
PretendoWindow::OnAudioTriangle()
{
	bool marked = fAudioMenu->ItemAt(nes::apu::TRIANGLE)->IsMarked();
	marked = ! marked;
	(fAudioMenu->ItemAt(nes::apu::TRIANGLE))->SetMarked(marked);
	
	if (marked) {
		nes::apu::unmute_channel(nes::apu::TRIANGLE);
	} else {
		nes::apu::mute_channel(nes::apu::TRIANGLE);
	}
}


void
PretendoWindow::OnAudioNoise()
{
	bool marked = fAudioMenu->ItemAt(nes::apu::NOISE)->IsMarked();
	marked = ! marked;
	(fAudioMenu->ItemAt(nes::apu::NOISE))->SetMarked(marked);
	
	if (marked) {
		nes::apu::unmute_channel(nes::apu::NOISE);
	} else {
		nes::apu::mute_channel(nes::apu::NOISE);
	}
}

	
void 
PretendoWindow::OnAudioDMC()
{
	bool marked = fAudioMenu->ItemAt(nes::apu::DPCM)->IsMarked();
	marked = ! marked;
	(fAudioMenu->ItemAt(nes::apu::DPCM))->SetMarked(marked);
	
	if (marked) {
		nes::apu::unmute_channel(nes::apu::DPCM);
	} else {
		nes::apu::mute_channel(nes::apu::DPCM);
	}
}


void
PretendoWindow::RenderLine8 (uint8 *dest, const uint32_t *source)
{
	// render to 8-bit buffer
	int32 intensity = 0;
	int32 width = screen_size::WIDTH / 4;
	uint8 *palette = reinterpret_cast<uint8 *>(fMappedPalette[intensity]);
	
	while (width--) {
		*(uint8 *)(dest+0) = palette[*source++ & 0x3f];
		*(uint8 *)(dest+1) = palette[*source++ & 0x3f];
		*(uint8 *)(dest+2) = palette[*source++ & 0x3f];
		*(uint8 *)(dest+3) = palette[*source++ & 0x3f];
		dest += 4 * sizeof(uint8);
	}	
}


void
PretendoWindow::RenderLine16 (uint8 *dest, const uint32_t *source)
{
	// render to 16-bit buffer
	int32 intensity = 0;
	int32 width = screen_size::WIDTH / 4;
	uint16 *palette = reinterpret_cast<uint16 *>(fMappedPalette[intensity]);

	while (width--) {
		*(uint16 *)(dest+0) = palette[*source++ & 0x3f];
		*(uint16 *)(dest+2) = palette[*source++ & 0x3f];
		*(uint16 *)(dest+4) = palette[*source++ & 0x3f];
		*(uint16 *)(dest+6) = palette[*source++ & 0x3f];
		dest += 4 * sizeof(uint16);
	}	
}


void
PretendoWindow::RenderLine32 (uint8 *dest, const uint32_t *source)
{
	// render to 32-bit buffer
	int32 intensity = 0;
	int32 width = screen_size::WIDTH / 4;
	uint32 const *palette = reinterpret_cast<uint32 *>(fMappedPalette[intensity]);
	
	while (width--) {
		*(uint32 *)(dest+0) = palette[*source++ & 0x3f];
		*(uint32 *)(dest+4) = palette[*source++ & 0x3f];
		*(uint32 *)(dest+8) = palette[*source++ & 0x3f];
		*(uint32 *)(dest+12) = palette[*source++ & 0x3f];
		dest += 4 * sizeof(uint32);
	}
}


void
PretendoWindow::ClearDirty()
{
	// clear dirty buffer
	uint32 *start = reinterpret_cast<uint32 *>(fDirtyBuffer.bits);
	uint32 *end = reinterpret_cast<uint32 *>(fDirtyBuffer.bits) + 
		(screen_size::WIDTH*2) * (screen_size::HEIGHT*2);
	
	if (fClear > 0) {
		while (start < end) {
			*start++ ^= 0xffffffff;
		}
		
		fClear--;
	}
}


void 
PretendoWindow::ClearBitmap (bool overlay)
{
	// clear any bitmaps used for rendering
	if (overlay) {
		uint8 *bits = fOverlayBits;
		for (int32 y = 0; y < PretendoWindow::screen_size::HEIGHT; y++) {
			for (int32 row = 0; row < fOverlayBitmap->BytesPerRow(); row += 2) {
				*(uint16 *)(bits+row) = (128 << 8) | (16 >> 0);
			}
		
			bits += fOverlayBitmap->BytesPerRow();
		}
	} else {
		memset(fBitmapBits, 0x0, fBitmap->BitsLength());
	}
}


void 
PretendoWindow::SetRenderer (color_space cs)
{	
	// choose a renderer based on incoming color_space
	switch (cs) {
		default:
		case B_CMAP8:
			for (int32 i = 0; i < 8; i++) {
				fMappedPalette[i] = reinterpret_cast<uint8 *>(&fPalette8[i]);
			}

			LineRenderer = &PretendoWindow::RenderLine8;
			break;
			
		case B_RGB16:
			for (int32 i = 0; i < 8; i++) {
				fMappedPalette[i] = reinterpret_cast<uint8 *>(&fPalette16[i]);
			}
			
			LineRenderer = &PretendoWindow::RenderLine16;
			break;
			
		case B_RGB32:
			for (int32 i = 0; i < 8; i++) {
				fMappedPalette[i] = reinterpret_cast<uint8 *>(&fPalette32[i]);
			}
			
			LineRenderer = &PretendoWindow::RenderLine32;
			break;
	}
}


void
PretendoWindow::SetFrontBuffer (uint8 *bits, color_space cs, int32 pixel_width, int32 row_bytes)
{
	// setup the front buffer
	fFrontBuffer.bits = bits;
	fFrontBuffer.pixel_format = cs;
	fFrontBuffer.pixel_width = pixel_width;
	fFrontBuffer.row_bytes = row_bytes;
	
	// prepare WindowScreen if necessary
	if (fFramework == video_framework::FULLSCREEN) {
		memset(fFrontBuffer.bits, 0x0, 480 * fFrontBuffer.row_bytes);
		memset(fDirtyBuffer.bits, 0xff, 480 * fFrontBuffer.row_bytes);
		fFrontBuffer.bits += (640 - screen_size::WIDTH*2) / 2;
	} else {
		// setup windowed buffer
		for (int32 y = 0; y < screen_size::HEIGHT; y++) {
			fLineOffsets[y] = fBackBuffer.bits + y * screen_size::WIDTH * pixel_width;
		}
	}
	
	// choose appropriate line renderer
	SetRenderer(cs);
}


void
PretendoWindow::ChangeFramework (video_framework fw)
{	
	// change the video framework being used
	if (fFramework == fw) {
		return;	
	}
	
	fFrameworkChanging = true;
	fPrevFramework = fFramework;
	fFramework = fw;
	
	fVideoMenu->ItemAt(fFramework)->SetMarked(true);
			
	// break down previous framework
	switch (fPrevFramework) {
		case video_framework::NONE:
		case video_framework::DIRECT:
			// nothing to do here
			break;
			
		case video_framework::BITMAP:
			ClearBitmap(false);
			break;
			
		case video_framework::OVERLAY:
			ClearBitmap(true);
			fView->ClearViewOverlay();
			fView->SetViewColor(0, 0, 0);
			fView->Invalidate();
			break;
			
		case video_framework::FULLSCREEN:			
			if (fVideoScreen->Lock()) {
				fVideoScreen->Quit();
			}
			break;
	}
	
	// build new framework
	switch (fFramework) {
		case video_framework::NONE:	
			fView->Invalidate();
			break;
			
		case video_framework::BITMAP:
			SetFrontBuffer(fBitmapBits, B_CMAP8, 4, fBitmap->BytesPerRow());
			
			// force a screen update in case we are coming from WindowScreen
			ClearBitmap(false);
			ClearDirty();
			break;
			
		case video_framework::OVERLAY:
			rgb_color key;
			SetFrontBuffer(fOverlayBits, B_RGB16, 2, fOverlayBitmap->BytesPerRow());
			ClearBitmap(true);
			fView->SetViewOverlay (fOverlayBitmap, fOverlayBitmap->Bounds(), 
				fView->Bounds(), &key, B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL 
				| B_OVERLAY_FILTER_VERTICAL);
			fView->SetViewColor(key);
			fView->Invalidate();
			break;
			
		case video_framework::DIRECT:
			// front buffer *must* be set in DirectConnected(), not here.
			Hide();
			Show();
			SetRenderer(BScreen().ColorSpace());
			break;
			
		case video_framework::FULLSCREEN:
			fVideoScreen = new VideoScreen (this);
			fVideoScreen->Show();
			snooze (1000000); 	// wait a little while for the screen to connect
			SetFrontBuffer(fVideoScreen->Bits(), B_CMAP8, 
							fVideoScreen->PixelWidth() / 2, fVideoScreen->RowBytes());
			fFullScreen = true;
			break;
	}
	
	fFrameworkChanging = false;
}


void
PretendoWindow::DrawDirect()
{
	// drawing code for the DirectWindow
	
	clipping_rect *clip = fClipInfo.clip_list;
	uint8 *dest;
	uint8 *source;
	uint8 *dirty;
	size_t size;
	
	if (! fDoubled) {
		// 1:1
		for (int32 i = 0; i < fClipInfo.clip_count; i++, clip++) {
			int32 const x = (clip->left - fClipInfo.bounds.left) * fPixelWidth;
			int32 const y = (clip->top - fClipInfo.bounds.top) + 1;
			int32 const w = clip->right - clip->left + 1;
			int32 h = clip->bottom - clip->top + 1;
			
			dest = fFrontBuffer.bits + y * fFrontBuffer.row_bytes + clip->left * fPixelWidth;
			source = fBackBuffer.bits + y * fBackBuffer.row_bytes + x;
			dirty = fDirtyBuffer.bits + y * fBackBuffer.row_bytes + x;
			size = w * fPixelWidth;
			
			while (h--) {
				blit_windowed_dirty_mmx(source, dirty, dest, size, fPixelWidth);
				
				dest += fFrontBuffer.row_bytes;
				source += fBackBuffer.row_bytes;
				dirty += fBackBuffer.row_bytes;
			}
		}
	} else {
		// 2:1
		int32 const h = fClipInfo.bounds.bottom - fClipInfo.bounds.top + 1;
		
		for (int32 i = 0; i < fClipInfo.clip_count; i++, clip++) {
			int32 const x = ((clip->left - fClipInfo.bounds.left) / 2) * fPixelWidth;
			int32 const w = clip->right - clip->left + 1;
		
			for (int32 y = 0; y < h; y += 2) {
				if (clip->top - fClipInfo.bounds.top <= y && clip->bottom - fClipInfo.bounds.top >= y) {
					dest = fFrontBuffer.bits + y * fFrontBuffer.row_bytes + clip->left * fPixelWidth;
					source = fBackBuffer.bits + (y / 2) * fBackBuffer.row_bytes + x;
					dirty = fDirtyBuffer.bits + (y / 2) * fBackBuffer.row_bytes + x;
					size = w * fPixelWidth;						

					blit_2x_windowed_dirty_mmx(source, dirty, dest, size, 
						fPixelWidth, fFrontBuffer.row_bytes);									
				}
			}
		}
	}
}


void
PretendoWindow::DrawBitmap()
{
	// drawing code for BBitmap
	
	uint8 *dest = fBitmapBits;
	uint8 *source = fBackBuffer.bits;
	uint8 *dirty = fDirtyBuffer.bits;
	
	size_t const size = screen_size::WIDTH;
	size_t height = screen_size::HEIGHT;
	
	while (height--) {
		blit_windowed_dirty_mmx(source, dirty, dest, size, fPixelWidth);

		dest += fFrontBuffer.row_bytes;
		source += fBackBuffer.row_bytes;
		dirty += fBackBuffer.row_bytes;
	}

	// FIXME: what is the right way to do this?	
	
	// this crashes/hangs sometimes on exit
	//Lock();
	//fView->DrawBitmap(fBitmap, fView->Bounds());
	//Unlock();
	
	// oddly, this method seems to work well
	PostMessage (messages::DRAW_BITMAP);	
}


void
PretendoWindow::DrawOverlay()
{
	// drawing code for overlay
	// no point to compile this code since we don't have overlay, and it's 32-bit only

#if 0
			source = reinterpret_cast<uint8 *>(fBackBuffer.bits);
			dest = reinterpret_cast<uint8 *>(fOverlayBits);
			size = PretendoWindow::screen_size::WIDTH / 2;
			
			blit_overlay(dest, source, size, fPaletteY, fPaletteYCbCr);
		
			
			for (int32 y = 0; y < PretendoWindow::screen_size::HEIGHT; y++) {
				asm volatile  ("pushl %%edi\n"
					  		  "pushl %%esi\n"
				  			  "pushl %%ebx\n"
				  			  //"pushl %%ebp\n"
				  	
				  			  "movl %0, %%edi\n"	//dest
				 	 		  "movl %1, %%esi\n"	//src
				  			  "movl %2, %%ecx\n"	//size
				  			  "movl %3, %%eax\n"	//Y
					  		  "movl %4, %%edx\n"	//YCbCr
					  
				  		  	  "1:\n"
				  		  	  "movl (%%esi), %%ebx\n"
				  		  	  "shrl $16, %%ebx\n"
				  		  	  "movl (%%eax,%%ebx,4), %%ebx\n"
				  		  	  "shll $16, %%ebx\n"
				  
				  		  	  "pushl %%ecx\n"
				  		  	  
				  		  	  "movl (%%esi), %%ecx\n"
				  		  	  "andl $0xffff, %%ecx\n"
				  		  	  "movl (%%edx,%%ecx,4), %%ecx\n"
		  
		  				  	  "orl %%ebx, %%ecx\n"

				  		  	  "movl %%ecx, (%%edi)\n"
				  		  	  "addl $4, %%edi\n" 
				  		  	  "addl $4, %%esi\n"
				  			  
				  			  "popl %%ecx\n"
				  			  	
				  		  	  "subl $1, %%ecx\n"
				  		  	  "jnz 1b\n"
				  
				  	  		  //"popl %%ebp\n"
				  		  	  "popl %%ebx\n"
				  		  	  "popl %%esi\n"
				  		  	  "popl %%edi\n"
				  		  	  : 
				  		  	  : "D"(dest), "S"(source), "c"(size), 
				  		  	  "a"((uint32 *)fPaletteY), "d" ((uint32 *)fPaletteYCbCr)
				  		  	  :
				);
				
				source += fBackBuffer.row_bytes;
				dest += fOverlayBitmap->BytesPerRow();
			}
#endif
}

void
PretendoWindow::DrawFullScreen()
{
	if (fFullScreen) {
		uint8 *dest = fFrontBuffer.bits;
		uint8 *source = fBackBuffer.bits;
		uint8 *dirty = fDirtyBuffer.bits;
		
		int32 const dx = fFrontBuffer.row_bytes;
		int32 const sx = fBackBuffer.row_bytes;
				
		for (int32 y = 0; y < screen_size::HEIGHT; y++) {
			blit_2x_dirty_mmx(dest, source, dirty, dx, screen_size::WIDTH);
			dest += (dx * 2);
			source += sx;
			dirty += sx;
		}
	}
}


void
PretendoWindow::BlitScreen()
{	
	// decide which blitter to use based on the current video framework
	
	if (fFrameworkChanging) {
		return;
	}

	switch (fFramework) {
		case video_framework::NONE:
			break;
			
		case video_framework::BITMAP:
			DrawBitmap();
			break;
			
		case video_framework::OVERLAY:
			DrawOverlay();
			break;
			
		case video_framework::DIRECT:
			if (fDirectConnected) {				
				ClearDirty();
				DrawDirect();
			}
			break;
			
		case video_framework::FULLSCREEN:
			DrawFullScreen();
			break;
	}
}


void
PretendoWindow::submit_scanline(int scanline, const uint32_t *source)
{
	(this->*LineRenderer)(fLineOffsets[scanline], source);
}

void 
PretendoWindow::set_palette(const color_emphasis_t *intensity, const rgb_color_t *pal)
{
	int32 i, j;
	rgb_color c;
	
	// rgb palettes
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 64; j++) {
			c.red   = (pal[j].r * intensity[i].r);
			c.green = (pal[j].g * intensity[i].g);
			c.blue  = (pal[j].b * intensity[i].b);
			
			fPalette8[i][j] = BScreen().IndexForColor(c.red, c.green, c.blue);
						
			fPalette16[i][j] = ((c.red & 0xf8) << 8);
			fPalette16[i][j] |= ((c.green & 0xfc) << 3);  
			fPalette16[i][j] |= ((c.blue & 0xf8) >> 3);
			
			fPalette32[i][j] = (c.red << 16) | (c.green << 8) | (c.blue & 0xff);
		}
	}
	
	// luma/chroma palette (for overlay)
	uint8 y;
	uint8 cb[65536];
	uint8 cr[65536];
	
	for (i = 0; i < 65536; i++) {
		// separate r,g,b components
		c.red = ((i & 0xf800) >> 11) << 3;
		c.green = ((i & 0x07e0) >> 5) << 2;
		c.blue = (i & 0x001f) << 3;
		
		// convert to y,cb,cr components
		y = fPaletteY[i] = (uint8)((int16)((double)
			(77.0 / 256.0) * (219 * (c.red / 256.0) + 16) +
			(150.0 / 256.0) * (219 * (c.green / 256.0) + 16) +
			(29.0 / 256.0) * (219 * (c.blue / 256.0) + 16)));
			
		cb[i] = (uint8)((int16)((double)
			(-44.0 / 256.0) * (219 * (c.red / 256.0) + 16) - 
			(87.0 / 256.0) * (219 * (c.green / 256.0) + 16) +
			(131.0 / 256.0) * (219 * (c.blue / 256.0) + 16) + 128.0));
			
		cr[i] = (uint8)((int16)((double)
			(131.0 / 256.0) * (219 * (c.red / 256.0) + 16) -
			(110.0 / 256.0) * (219 * (c.green / 256.0) + 16) -
			(21.0 / 256.0) * (219 * (c.blue / 256.0) + 16) + 128.0));
			
		fPaletteYCbCr[i] = (cr[i] << 24) | (0 << 16) | (cb[i] << 8) | (y & 0xff);
	}		
}


void  
PretendoWindow::start_frame()
{	
	// setup DirectWindow if we need
	//if (fDirectConnected) {
		fPixelWidth = fFrontBuffer.pixel_width;
		fBackBuffer.row_bytes = screen_size::WIDTH * fPixelWidth;
	//}
}


void
PretendoWindow::end_frame()
{
	size_t const bufferSize = nes::apu::frequency / nes::apu::frame_rate;
	uint8 sampleBuffer[bufferSize];
	size_t const bufferCount = nes::apu::read_samples(sampleBuffer, sizeof(sampleBuffer));
	
	BlitScreen();
	fAudioStream->Stream(sampleBuffer, bufferCount);
}


status_t
PretendoWindow::emulator_thread (void *data)
{
	// start the show!
	PretendoWindow *window = reinterpret_cast<PretendoWindow *>(data);	
	
	while (1) {
		// lock mutex
		if (window->LockMutex() == false) { 
			break;
		}
		
		// do frame events
		window->start_frame();
		nes::run_frame (window);
		window->end_frame();
		window->ReadKeyStates();	
		
		// unlock mutex
		window->UnlockMutex();
	}	
	
	return B_OK;
}


inline void
PretendoWindow::CheckKey (int32 index, int32 key) const
{
	// read keystates as explained in the BeBook
	// note the window does not need to have focus for this to work
	
	nes::input::controller1.keystate_[index] = 
		fKeyStates.key_states[key >> 3] & (1 << (7 - (key % 8)));
}


inline void
PretendoWindow::ReadKeyStates()
{
	get_key_info(&fKeyStates);
	
	CheckKey(Controller::INDEX_UP, 		default_keys::UP);
	CheckKey(Controller::INDEX_DOWN, 	default_keys::DOWN);
	CheckKey(Controller::INDEX_LEFT, 	default_keys::LEFT);
	CheckKey(Controller::INDEX_RIGHT, 	default_keys::RIGHT);
	CheckKey(Controller::INDEX_SELECT,	default_keys::SELECT);
	CheckKey(Controller::INDEX_START, 	default_keys::START);
	CheckKey(Controller::INDEX_B, 		default_keys::B);
	CheckKey(Controller::INDEX_A, 		default_keys::A);
}


void
PretendoWindow::SetDefaultPalette()
{
	// if we couldn't load a palette from settings, use the defaults
	set_palette(Palette::intensity, 
				Palette::NTSC (Palette::default_saturation,
				Palette::default_hue,
				Palette::default_contrast,
				Palette::default_brightness,
				Palette::default_gamma));
}
