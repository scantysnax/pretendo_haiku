
#include <iostream>

#include "Mapper.h"
#include "Nes.h"
#include "Apu.h"
#include "Cart.h"
#include "Input.h"
#include "PretendoWindow.h"
#include "CartInfoWindow.h"
#include "PaletteWindow.h"
#include "VideoScreen.h"
#include "ROMFilePanel.h"
#include "PaletteWindow.h"
#include "SimpleMutex.h"
#include "PretendoView.h"
#include "Controller.h"
#include "SoundPusher.h"
#include "Reset.h"
#include "Palette.h"
#include "asm/blitters.h"
#include "asm/copies.h"

class VideoScreen;
class PretendoView;


PretendoWindow::PretendoWindow()
	: BDirectWindow (BRect (0, 0, 0, 0), "Pretendo", B_TITLED_WINDOW, B_NOT_RESIZABLE, 0),
		fView(NULL),
		fMenu(NULL),
		fFileMenu(NULL),
		fEmuMenu(NULL),
		fVideoMenu(NULL),
		fOpenPanel(NULL),
		fCartInfoWindow(NULL),
		fBitmap(NULL),
		fOverlayBitmap(NULL),
		fBitmapBits(NULL),
		fOverlayBits(NULL),
		fBitsArea(B_ERROR),
		fDirtyArea(B_ERROR),
		fVideoScreen(NULL),
		fSoundPusher(NULL),
		//fPaletteWindow(NULL),
		fPaused(false),
		fRunning(false),
		fClockSpeed(0),
		fShowFPS(true)
		
{
	BRect bounds (Bounds());
	bounds.OffsetTo (B_ORIGIN);
	AddMenu();
	bounds.top = fMenuHeight;
	
	fView = new PretendoView(bounds, this);
	AddChild (fView);
	
	ResizeTo (SCREEN_WIDTH, SCREEN_HEIGHT);
	CenterOnScreen();
	
	// if we can't create even this simplest video interface there is no point
	// to keep the app running.
	fBitmap = new BBitmap (BRect (0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1), B_CMAP8);
	if (! fBitmap || ! fBitmap->IsValid()) {
		(new BAlert ("Error", "Not enough memory for video bitmap.  Quitting.",
			"Bummer", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		OnQuit();
	} else {
		// set up the BBitmap based framework
		fBitmapBits = reinterpret_cast<uint8 *>(fBitmap->Bits());
		ClearBitmap (false);
	}

	// use these in case we need to share buffers across threads
	void *areaBits = NULL;
	void *dirtyBits = NULL;
	
	fBitsArea = create_area ("frame buffer", &areaBits, B_ANY_ADDRESS,
					((SCREEN_WIDTH * 2) * (SCREEN_HEIGHT * 2) * 4 + B_PAGE_SIZE-1) & 
					((uint32)-1 ^ (B_PAGE_SIZE-1)), B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	
	fDirtyArea = create_area ("dirty buffer", &dirtyBits, B_ANY_ADDRESS,
					((SCREEN_WIDTH * 2) * (SCREEN_HEIGHT * 2) * 4 + B_PAGE_SIZE-1) & 
					((uint32)-1 ^ (B_PAGE_SIZE-1)), B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
					
	if (fBitsArea < B_OK || fDirtyArea < B_OK) {
		(new BAlert ("Error", "Not enough memory for video buffers.  Quitting.",
			"Bummer", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		OnQuit();
	} else {
		memset (areaBits, 0x00, (SCREEN_WIDTH*2) * (SCREEN_HEIGHT*2) * 4);
		memset (dirtyBits, 0xff, (SCREEN_WIDTH*2) * (SCREEN_HEIGHT*2) * 4);
		
		fBackBuffer.bits = reinterpret_cast<uint8 *>(areaBits);
		fDirtyBuffer.bits = reinterpret_cast<uint8 *>(dirtyBits);
	}					
	
	// create a BBitmap for overlay framework (checks for overlay support inherently)
	// this will always fail until we get hardware accelerated video
	bool overlayOK = false;

	bounds.Set (0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
	fOverlayBitmap = new BBitmap (bounds, B_BITMAP_WILL_OVERLAY, B_YCbCr422);
	overlayOK = fOverlayBitmap && fOverlayBitmap->IsValid();

	if (overlayOK) {
		fVideoMenu->ItemAt(2)->SetEnabled(true);
		fOverlayBits = reinterpret_cast<uint8 *>(fOverlayBitmap->Bits());
		ClearBitmap (true);
	} else {
		fOverlayBits = NULL;
		if (fOverlayBitmap) {
			delete fOverlayBitmap;
		}
		
		fView->SetViewColor(0, 0, 0);
		ClearBitmap(false);
		fVideoMenu->ItemAt(2)->SetEnabled(false);
	}
	
	fDirectConnected = 
	fFullScreen = 
	fFrameworkChanging = false;	
	fFramework = 
	fPrevFramework = NO_FRAMEWORK;
	
	if (overlayOK) {
		ChangeFramework (OVERLAY_FRAMEWORK);
	} else {
		// there will be mouse "trails" on the BDirectWindow until we get
		// a hardware cursor.  Again, no accelerated video yet
		ChangeFramework(DIRECTWINDOW_FRAMEWORK);
	}

	// other things we need
	fOpenPanel = new ROMFilePanel();	
	fSoundPusher = new SoundPusher();
	
	fDoubled = false;
	fClear = 0;
	
	fThread = spawn_thread(emulation_thread, "pretendo_thread", B_DISPLAY_PRIORITY, 
		reinterpret_cast<void *>(this));
	
	if (fThread < B_OK) {
			throw std::runtime_error("spawn_thread failed");
			// for some reason or another we couldn't spawn the main thread
			// we can't run without a thread
			OnQuit();
	} else {
		fRunning = false;
	}
	
	// we need a mutual exclusion to protect threaded code
	fMutex = new SimpleMutex("pretendo_mutex");
	fMutex->Lock();
	resume_thread(fThread);
	
	SetDefaultPalette();
	
	//fFileMenu->ItemAt(1)->SetEnabled(false);
	
	// initialise the sound code
	fSoundPusher->Init();
	
	// measure clock speeed
	bigtime_t start, finish;
	start = real_time_clock_usecs(); //ReadTSC();
	sleep(1);
	finish = real_time_clock_usecs(); //ReadTSC();
	fClockSpeed = finish - start;
	//printf("cpu clock speed: %lu Hz\n", fClockSpeed);
}


PretendoWindow::~PretendoWindow()
{	
	// tear everything down and clean up
	fRunning = fDirectConnected = false;
	fThread = B_BAD_THREAD_ID;

	if (fOpenPanel->Window()) {
		fOpenPanel->Window()->Lock();
		fOpenPanel->Window()->Quit();
		fOpenPanel = NULL;
	}
	
	if (fBitmap->IsValid()) {
		delete fBitmap;
		fBitmap = NULL;
	}
	
	if (fOverlayBitmap->IsValid()) {
		delete fOverlayBitmap;
		fOverlayBitmap = NULL;
	}
	
	delete_area (fBitsArea);
	delete_area (fDirtyArea);

	fSoundPusher->Stop();
	delete fSoundPusher;
	
	if (fCartInfoWindow != NULL) {
		fCartInfoWindow->Lock();
		fCartInfoWindow->Quit();
	}
	
	//if (fPaletteWindow != NULL) {
	//	fPaletteWindow->Lock();
	//	fPaletteWindow->Quit();
	//}

	
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
			
			if (fFramework == DIRECTWINDOW_FRAMEWORK) {
				SetFrontBuffer (reinterpret_cast<uint8 *>(info->bits)
					+ (fClipInfo.bounds.top * info->bytes_per_row), info->pixel_format,
					info->bits_per_pixel / 8, info->bytes_per_row);
			}
		
			fClipInfo.clip_list = NULL;
			fDirectConnected = true;
		// intentional fall through //	
		case B_DIRECT_MODIFY:
			fClear = 5;
			fClipInfo.clip_count = info->clip_list_count;
			fClipInfo.clip_list = 
				reinterpret_cast<clipping_rect *>(realloc(fClipInfo.clip_list, 
				fClipInfo.clip_count * sizeof(clipping_rect)));
			
			// mmx_copy doesn't work here, and i don't know why
			// there seems to be an edge case when the window is completely
			// obscured by another window, it causes a segfault
			
			//mmx_copy (fClipInfo.clip_list, info->clip_list,
			//	fClipInfo.clip_count * sizeof(clipping_rect));
				
			memcpy (fClipInfo.clip_list, info->clip_list,
				fClipInfo.clip_count * sizeof(clipping_rect));
			
			for (int32 i = 0; i < fClipInfo.clip_count; i++) {
				if (fClipInfo.clip_list[i].top <= 
					static_cast<int32>(info->window_bounds.top + fMenuHeight)) {
					fClipInfo.clip_list[i].top = info->window_bounds.top + fMenuHeight;
				}
			}
			break;
			
		case B_DIRECT_STOP:
			fDirectConnected = false;
			free (fClipInfo.clip_list);
			break;
	}
	
	BDirectWindow::DirectConnected(info);
}


void
PretendoWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case MSG_CHANGE_RENDER:
			ChangeFramework(
				static_cast<VIDEO_FRAMEWORK>(fVideoMenu->IndexOf(fVideoMenu->FindMarked())));
			break;
		
		case MSG_LEAVE_FULLSCREEN:
			ChangeFramework (fPrevFramework);
		 	break;
			
		case MSG_ROM_LOADED:
			OnLoadCart(message);
			break;
			
		case MSG_SHOW_OPEN:
			fOpenPanel->Show();
			break;
			
		case B_REFS_RECEIVED:
			be_app->PostMessage(message);
			break;

		case MSG_FREE_ROM:
			OnFreeCart();
			break;
			
		case MSG_CART_INFO:
			OnCartInfo();
			break;
			
		case MSG_ABOUT:
			be_app->PostMessage (B_ABOUT_REQUESTED);
			break;
			
		case MSG_QUIT:
			OnQuit();
			break;
			
		case MSG_CPU_RUN:
			OnRun();
			break;
			
		case MSG_CPU_STOP:
			OnStop();
			break;
			
		case MSG_CPU_PAUSE:
			OnPause();
			break;
			
		case MSG_CPU_DEBUG:
			OnDebug();
			break;
			
		case MSG_RST_SOFT:
			OnSoftReset();
			break;
			
		case MSG_RST_HARD:
			OnHardReset();
			break;
			
		case MSG_DRAW_BITMAP:
			// this has to go here, since the window is guaranteed to be locked
			fView->DrawBitmap(fBitmap, fView->Bounds());
			break;
			
		case MSG_ADJ_PALETTE:
		//	if (fPaletteWindow == NULL) {
		//		fPaletteWindow = new PaletteWindow(this);
		//	}
			
		//	fPaletteWindow->Show();
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
PretendoWindow::MenusBeginning (void)
{	
	BMenu *menu = BRecentFilesList::NewFileListMenu ("Load ROM" B_UTF8_ELLIPSIS,
		NULL, NULL, this->PreferredHandler(), 5, false, NULL, 0, 
		"application/x-vnd.scantysnax-Pretendo");
	
	fFileMenu->AddItem(new BMenuItem(menu, new BMessage(MSG_SHOW_OPEN)), 0);
	
	BDirectWindow::MenusBeginning();
}


void
PretendoWindow::MenusEnded (void)
{	
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
	height += fMenuHeight;
	
	BDirectWindow::ResizeTo (width, height);
}


void
PretendoWindow::Zoom (BPoint origin, float width, float height)
{
	(void)origin;
	(void)width;
	(void)height;
	
	float w = Bounds().right - Bounds().left;	
		
	if (w == SCREEN_WIDTH) {
		ResizeTo ((SCREEN_WIDTH*2), (SCREEN_HEIGHT*2));
		fDoubled = true;
	} else {
		if (w == SCREEN_WIDTH*2) {
			ResizeTo (SCREEN_WIDTH, SCREEN_HEIGHT);
		} 
		
		fDoubled = false;
	}
	
	Hide();
	Show();
}


void
PretendoWindow::AddMenu (void)
{
	fMenu = new BMenuBar (BRect (0, 0, 0, 0), "menu");
	AddChild (fMenu);
	
	fFileMenu = new BMenu ("File");
	fMenu->AddItem (fFileMenu);
	
	fEmuMenu = new BMenu ("Emulator");
	fMenu->AddItem (fEmuMenu);
	
	fVideoMenu = new BMenu ("Render");
	fMenu->AddItem (fVideoMenu);

	fVideoMenu->AddItem(new BMenuItem ("No Output", new BMessage (MSG_CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem ("BView/BBitmap", new BMessage(MSG_CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem ("BView/Overlay", new BMessage(MSG_CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem ("BDirectWindow", new BMessage(MSG_CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem ("BWindowScreen", new BMessage(MSG_CHANGE_RENDER), 'F'));
	fVideoMenu->SetRadioMode(true);
	fVideoMenu->ItemAt(2)->SetMarked(false);
	
	fFileMenu->AddItem (new BMenuItem ("Free ROM", new BMessage (MSG_FREE_ROM)));
	fFileMenu->AddItem(new BMenuItem("ROM Info", new BMessage(MSG_CART_INFO)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem (new BMenuItem ("About" B_UTF8_ELLIPSIS, new BMessage(MSG_ABOUT)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem (new BMenuItem ("Quit", new BMessage (MSG_QUIT)));
	
	fEmuMenu->AddItem (new BMenuItem ("Start", new BMessage (MSG_CPU_RUN)));
	fEmuMenu->AddItem (new BMenuItem ("Pause", new BMessage (MSG_CPU_PAUSE)));
	fEmuMenu->AddItem (new BMenuItem ("Stop", new BMessage (MSG_CPU_STOP)));
	fEmuMenu->AddSeparatorItem();
	fEmuMenu->AddItem (new BMenuItem ("Reset (soft)", new BMessage (MSG_RST_SOFT)));
	fEmuMenu->AddItem (new BMenuItem ("Reset (hard)", new BMessage (MSG_RST_HARD)));
	fEmuMenu->AddSeparatorItem();
	fEmuMenu->AddItem (new BMenuItem ("Adjust Palette" B_UTF8_ELLIPSIS, 
		new BMessage (MSG_ADJ_PALETTE)));
	//fEmuMenu->AddItem (new BMenuItem ("Debug" B_UTF8_ELLIPSIS, new BMessage(MSG_CPU_DEBUG)));
	fMenuHeight = fMenu->Bounds().IntegerHeight();
	
	SetKeyMenuBar(fMenu);
}


void
PretendoWindow::OnLoadCart (BMessage *message)
{
	BString path;
	
	if (message->FindString ("path", &path) == B_OK) {
		OnFreeCart();
		if (nes::cart.load(path.String()) == false) {
			(new BAlert("Error", "Error, Invalid ROM Image", "Okay", NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		}
		
		if (fFramework == OVERLAY_FRAMEWORK) {
			ClearBitmap (true);
		} else {
			fView->SetViewColor (0, 0, 0);
			fView->Invalidate();
		}
	}
}


void
PretendoWindow::OnFreeCart (void)
{	
	OnStop();
	nes::cart.unload();
}


void
PretendoWindow::OnCartInfo (void)
{
	if (fCartInfoWindow && fCartInfoWindow->Lock()) {
		fCartInfoWindow->Quit();
		fCartInfoWindow = NULL;
	}
	
	if (nes::cart.prg() != NULL) { // && ! fCartInfoWindow) {
		fCartInfoWindow = new CartInfoWindow();
		fCartInfoWindow->Show();
	}
}

void
PretendoWindow::OnQuit (void)
{	
	this->QuitRequested();
}


void
PretendoWindow::OnRun (void)
{	
	if (! fRunning) {
		// make sure we have a cart loaded
		if(nes::cart.mapper()) {
			reset(nes::Reset::Hard);
			fMutex->Unlock(); // unlock the mutual exclusion
			fRunning = true;  // signal the thread that we're running
			fSoundPusher->Start(); // start the sound pusher interface
		}
	} else if (fPaused) {
		OnPause();
	}
}


void
PretendoWindow::OnStop (void)
{		
	// if we're running, set fRunning to false to signal the thread we're not
	// running, lock the mutual exclusion and stop the sound pusher interface
	if (fRunning) {
		fRunning = false;
		if (! fPaused) {
			fMutex->Lock();
			fSoundPusher->Stop();
		}

		// clear the window contents
		if (fFramework == OVERLAY_FRAMEWORK) {
			ClearBitmap(true);
		} else {
			ClearBitmap(false);
			fView->SetViewColor(0, 0, 0);
			fView->Invalidate();
		}
	}
	
	fPaused = false;
	fEmuMenu->ItemAt(1)->SetMarked(false);
}


void
PretendoWindow::OnPause (void)
{	
	if (fRunning) {
		if (fPaused) {
			// if we are paused, we want to unpause, so lock the mutual exclusion
			// update the recent roms menu and start the sound pusher interface
			fMutex->Unlock();
			fEmuMenu->ItemAt(1)->SetMarked(false);
			fSoundPusher->Start();
		} else {
			// otherwise, we want to pause, so lock the mutual exclusion
			// mark the menu accordingly and stop the sound pusher interface
			fMutex->Lock();
			fEmuMenu->ItemAt(1)->SetMarked(true);
			fSoundPusher->Stop();
		}
	
		fPaused = !fPaused;
	}
}


void
PretendoWindow::OnSoftReset (void)
{
	reset(nes::Reset::Soft);
}


void
PretendoWindow::OnHardReset (void)
{
	reset(nes::Reset::Hard);
}


void
PretendoWindow::OnDebug (void)
{
}


void
PretendoWindow::RenderLine8 (uint8 *dest, const uint8 *source, int intensity)
{
	int32 width = SCREEN_WIDTH / 4;
	uint8 *palette = reinterpret_cast<uint8 *>(fMappedPalette[intensity]);
	
	while (width--) {
		*(dest+0) = palette[*source++];
		*(dest+1) = palette[*source++];
		*(dest+2) = palette[*source++];
		*(dest+3) = palette[*source++];
		dest += 4 * sizeof(uint8);
	}	
}


void
PretendoWindow::RenderLine16 (uint8 *dest, const uint8 *source, int intensity)
{
	int32 width = SCREEN_WIDTH / 4;
	uint16 *palette = reinterpret_cast<uint16 *>(fMappedPalette[intensity]);

	while (width--) {
		*(uint16 *)(dest+0) = palette[*source++];
		*(uint16 *)(dest+2) = palette[*source++];
		*(uint16 *)(dest+4) = palette[*source++];
		*(uint16 *)(dest+6) = palette[*source++];
		dest += 4 * sizeof(uint16);
	}	
}


void
PretendoWindow::RenderLine32 (uint8 *dest, const uint8 *source, int intensity)
{
	int32 width = SCREEN_WIDTH / 4;
	uint32 const *palette = reinterpret_cast<uint32 *>(fMappedPalette[intensity]);
	
	while (width--) {
		*(uint32 *)(dest+0) = palette[*source++];
		*(uint32 *)(dest+4) = palette[*source++];
		*(uint32 *)(dest+8) = palette[*source++];
		*(uint32 *)(dest+12) = palette[*source++];
		dest += 4 * sizeof(uint32);
	}
}


void
PretendoWindow::ClearDirty (void)
{
	// clear our "dirty" buffers
	uint32 *start = reinterpret_cast<uint32 *>(fDirtyBuffer.bits);
	uint32 *end = reinterpret_cast<uint32 *>(fDirtyBuffer.bits) + (SCREEN_WIDTH*2) * (SCREEN_HEIGHT*2);
	
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
		for (int32 y = 0; y < PretendoWindow::SCREEN_HEIGHT; y++) {
			for (int32 row = 0; row < fOverlayBitmap->BytesPerRow(); row += 2) {
				*(uint16 *)(bits+row) = (128 << 8) | (16 >> 0);
			}
		
			bits += fOverlayBitmap->BytesPerRow();
		}
	} else {
		memset (fBitmapBits, 0x0, fBitmap->BitsLength());
	}
}


void 
PretendoWindow::SetRenderer (color_space cs)
{	
	switch (cs) {
		default:
		case B_CMAP8:
			for (int32 i = 0; i < 8; i++) {
				fMappedPalette[i] = reinterpret_cast<uint8 *>(&fPalette8[i]);
			}

			LineRenderer = &PretendoWindow::RenderLine8;
			break;
			
		case B_RGB15:
			for (int32 i = 0; i < 8; i++) {
				fMappedPalette[i] = reinterpret_cast<uint8 *>(&fPalette15[i]);
			}
			
			LineRenderer = &PretendoWindow::RenderLine16;
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
PretendoWindow::SetFrontBuffer (uint8 *bits, color_space cs, int32 pixel_width, 
	int32 row_bytes)
{
	// setup the front buffer
	fFrontBuffer.bits = bits;
	fFrontBuffer.pixel_format = cs;
	fFrontBuffer.pixel_width = pixel_width;
	fFrontBuffer.row_bytes = row_bytes;
	
	if (fFramework == WINDOWSCREEN_FRAMEWORK) {
		memset (fFrontBuffer.bits, 0x0, 480 * fFrontBuffer.row_bytes);
		memset (fDirtyBuffer.bits, 0xff, 480 * fFrontBuffer.row_bytes);
		fFrontBuffer.bits += (640 - SCREEN_WIDTH*2) / 2;
	} else {
		for (int32 y = 0; y < SCREEN_HEIGHT; y++) {
			fLineOffsets[y] = fBackBuffer.bits + y * SCREEN_WIDTH * pixel_width;
		}
	}
	
	SetRenderer (cs);
}


void
PretendoWindow::ChangeFramework (VIDEO_FRAMEWORK fw)
{	
	// change the video framework being used
	if (fFramework == fw) {
		return;	
	}
	
	fFrameworkChanging = true;
	fPrevFramework = fFramework;
	fFramework = fw;
	
	fVideoMenu->ItemAt(fFramework)->SetMarked(true);
		
	// tear down previous framework
	switch (fPrevFramework) {
		case NO_FRAMEWORK:
		case DIRECTWINDOW_FRAMEWORK:
			// nothing to do here
			break;
			
		case BITMAP_FRAMEWORK:
			ClearBitmap (false);
			break;
			
		case OVERLAY_FRAMEWORK:
			ClearBitmap (true);
			fView->ClearViewOverlay();
			fView->SetViewColor(0, 0, 0);
			fView->Invalidate();
			break;
			
		case WINDOWSCREEN_FRAMEWORK:			
			if (fVideoScreen->Lock()) {
				fVideoScreen->Quit();
			}
			
			break;
	}
	
	// build new framework
	switch (fFramework) {
		case NO_FRAMEWORK:	
			fView->Invalidate();
			break;
			
		case BITMAP_FRAMEWORK:
			SetFrontBuffer (fBitmapBits, B_CMAP8, 4, fBitmap->BytesPerRow());
			ClearBitmap (false);
			fView->Invalidate();
			break;
			
		case OVERLAY_FRAMEWORK:
			rgb_color key;
			SetFrontBuffer (fOverlayBits, B_RGB16, 2, fOverlayBitmap->BytesPerRow());
			ClearBitmap (true);
			fView->SetViewOverlay (fOverlayBitmap, fOverlayBitmap->Bounds(), 
				fView->Bounds(), &key, B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL 
				| B_OVERLAY_FILTER_VERTICAL);
			fView->SetViewColor (key);
			fView->Invalidate();
			break;
			
		case DIRECTWINDOW_FRAMEWORK:
			// front buffer *must* be set in DirectConnected(), not here.
			Hide();
			Show();
			SetRenderer (BScreen().ColorSpace());
			break;
			
		case WINDOWSCREEN_FRAMEWORK:
			fVideoScreen = new VideoScreen (this);
			fVideoScreen->Show();
			snooze (1000000); 	// wait a little while for the screen to connect
			SetFrontBuffer (fVideoScreen->Bits(), B_CMAP8, 
							fVideoScreen->PixelWidth() / 2, fVideoScreen->RowBytes());
			fFullScreen = true;
			fShowFPS = false;
			break;
	}
	
	fFrameworkChanging = false;
}


void
PretendoWindow::DrawDirect (void)
{
	// drawing code for the DirectWindow
	
	clipping_rect *clip = fClipInfo.clip_list;
	uint8 *dest;
	uint8 *source;
	uint8 *dirty;
	size_t size;
	
	if (! fDoubled) {
		for (int32 i = 0; i < fClipInfo.clip_count; i++, clip++) {
			int32 x = (clip->left - fClipInfo.bounds.left) * fPixelWidth;
			int32 y = (clip->top - fClipInfo.bounds.top) + 1;
			int32 w = clip->right - clip->left + 1;
			int32 h = clip->bottom - clip->top + 1;
			
			dest = fFrontBuffer.bits + y * fFrontBuffer.row_bytes + clip->left 
				* fPixelWidth;
			source = fBackBuffer.bits + y * fBackBuffer.row_bytes + x;
			dirty = fDirtyBuffer.bits + y * fBackBuffer.row_bytes + x;
			size = w * fPixelWidth;
		
			while (h--) {
				blit_windowed_dirty_mmx (source, dirty, dest, size, fPixelWidth);
				source += fBackBuffer.row_bytes;
				dirty += fBackBuffer.row_bytes;
				dest += fFrontBuffer.row_bytes;
			}
		}
	} else {
		int32 h = fClipInfo.bounds.bottom - fClipInfo.bounds.top + 1;
				
		for (int32 i = 0; i < fClipInfo.clip_count; i++, clip++) {
			int32 x = ((clip->left - fClipInfo.bounds.left) >> 1) * fPixelWidth;
			int32 w = clip->right - clip->left + 1;
		
			for (int32 y = 0; y < h; y += 2) {
				if (clip->top - fClipInfo.bounds.top <= y 
					&& clip->bottom - fClipInfo.bounds.top >= y) {
					
					source = fBackBuffer.bits + (y / 2) * fBackBuffer.row_bytes + x;
					dirty = fDirtyBuffer.bits + (y / 2) * fBackBuffer.row_bytes + x;
					dest = fFrontBuffer.bits + y * fFrontBuffer.row_bytes + clip->left 
						* fPixelWidth;
					size = w * fPixelWidth;						
					
					blit_2x_windowed_dirty_mmx (source, dirty, dest, size, 
						fPixelWidth, fFrontBuffer.row_bytes);									
				}
			}
		}
	}
}
		

void
PretendoWindow::BlitScreen (void)
{	
	// decide which blitter to use based on the current video framework
	
	if (fFrameworkChanging  || ! fFramework) {
		return;
	}

	uint8 *source;
	uint8 *dest;
	size_t size;
	size_t row_bytes;
	
	switch (fFramework) {
		case NO_FRAMEWORK:
			return; // we should never be here
			
		case BITMAP_FRAMEWORK:
			source = fBackBuffer.bits;
			dest = fBitmapBits;
			size = SCREEN_WIDTH;
			row_bytes = fBitmap->BytesPerRow();
		
			for (int32 y = 0; y < SCREEN_HEIGHT; y++) {
				mmx_copy (dest, source, size);
				source += fBackBuffer.row_bytes;
				dest += row_bytes;
			}
		
			PostMessage (MSG_DRAW_BITMAP);
			return;
			
		case OVERLAY_FRAMEWORK:
			// blit w/overlay
			// no point to compile this code since we don't have overlay

#if 0
			source = reinterpret_cast<uint8 *>(fBackBuffer.bits);
			dest = reinterpret_cast<uint8 *>(fOverlayBits);
			size = PretendoWindow::SCREEN_WIDTH / 2;
			
	//		blit_overlay(dest, source, size, fPaletteY, fPaletteYCbCr);
		
			
			for (int32 y = 0; y < PretendoWindow::SCREEN_HEIGHT; y++) {
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
		
			fVideoLocker.Unlock();
#endif
			return;
			
		case DIRECTWINDOW_FRAMEWORK:
			if (fDirectConnected) {				
				ClearDirty();
				DrawDirect();
			}
			
			return;
			
		case WINDOWSCREEN_FRAMEWORK:
			if (fFullScreen) {
				uint8 *dirty = fDirtyBuffer.bits;
				uint32 dx = fFrontBuffer.row_bytes;
				uint32 sx = fBackBuffer.row_bytes;
				
				dest = fFrontBuffer.bits;
				source = fBackBuffer.bits;

				for (int32 y = 0; y < SCREEN_HEIGHT; y++) {
					blit_2x_dirty_mmx(dest, source, dirty, dx, SCREEN_WIDTH);
					dest += (dx * 2);
					source += sx;
					dirty += sx;
				}
			}
			
			return;
	}
}


void
PretendoWindow::submit_scanline(int scanline, int intensity, const uint8_t *source)
{
	(this->*LineRenderer)(fLineOffsets[scanline], source, intensity);
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
			
			fPalette8[i][j] = BScreen().IndexForColor (c.red, c.green, c.blue);
			
			fPalette15[i][j] = ((c.red & 0xf8) << 7);
			fPalette15[i][j] |= ((c.green & 0xf8) << 2);
			fPalette15[i][j] |= ((c.blue & 0xf8) >> 3);
			
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
	if (fDirectConnected) {
		fPixelWidth = fFrontBuffer.pixel_width;
		fBackBuffer.row_bytes = SCREEN_WIDTH * fPixelWidth;
	}
	
	// unlock the next page of sound data
	fSoundPusher->UnlockPage();
}


void
PretendoWindow::end_frame()
{
	// frame lock to 60FPS, blit the screen, and lock the next page of audio
	uint64 prevCount;
	uint64 curCount;
	
	uint64 clocksPerFrame = fClockSpeed / 60;
	prevCount = real_time_clock_usecs(); // ReadTSC();
		
	do {
		curCount = real_time_clock_usecs(); //ReadTSC();
	} while(curCount - prevCount < clocksPerFrame);
	
	BlitScreen();
	fSoundPusher->LockNextPage();

	if (fShowFPS) {
		ShowFPS();
	}	
}


status_t
PretendoWindow::emulation_thread (void *data)
{
	// start the show!
	
//	PretendoWindow *window = reinterpret_cast<PretendoWindow *>(data);	
	
		//while (1) {
		//	if (window->Mutex()->Lock() != B_NO_ERROR) {
		//		break;
		//	}
			
		//	window->start_frame();
		//	nes::run_frame(window);
		//	window->end_frame();
		//	window->ReadKeyStates();
			
		//	window->Mutex()->Unlock();
		//}	
	
	return B_OK;
}


inline void
PretendoWindow::CheckKey (int32 index, int32 key)
{
	(void)index;
	(void)key;
	// read keystates as explained in the BeBook
	// note the window does not need to have focus for this to work
	
	//nes::input::controller1().keystate_[index] = 
	//	fKeyStates.key_states[key >> 3] & (1 << (7 - (key % 8)));
}

inline void
PretendoWindow::ReadKeyStates (void)
{
//	get_key_info(&fKeyStates);
	
//	CheckKey(Controller::INDEX_UP, kKeyUp);
//	CheckKey(Controller::INDEX_DOWN, kKeyDown);
//	CheckKey(Controller::INDEX_LEFT, kKeyLeft);
//	CheckKey(Controller::INDEX_RIGHT, kKeyRight);
//	CheckKey(Controller::INDEX_SELECT, kKeySelect);
//	CheckKey(Controller::INDEX_START, kKeyStart);
//	CheckKey(Controller::INDEX_B, kKeyB);
//	CheckKey(Controller::INDEX_A, kKeyA);
}


void
PretendoWindow::SetDefaultPalette (void)
{
	// if we couldn't load a palette from settings, use the defaults
	set_palette(Palette::intensity, 
				Palette::NTSC (Palette::default_saturation,
				Palette::default_hue,
				Palette::default_contrast,
				Palette::default_brightness,
				Palette::default_gamma));
}


uint64
PretendoWindow::ReadTSC(void)
{
	// we can't stuff the whole tsc into rax because it gives
	// unpredictable results.  sometimes it works, sometimes not,
	// so this is our safest bet for now.

	uint64 l;
	uint64 h;
	uint64 tsc;

   asm volatile ("rdtsc" 
    			: "=a" (l), "=d" (h));
    tsc = static_cast<uint64>(h << 32 | l);
	return tsc;
}

void
PretendoWindow::ShowFPS (void)
{
	// count and show current FPS in window title
	static bigtime_t curCount = 0;
	static bigtime_t prevCount = 0;
	static int32 frameCount = 0;
	BString title;
	
	curCount = real_time_clock_usecs(); // ReadTSC();

	if (curCount != 0) {
		frameCount++;
		uint64 const diff = curCount - prevCount;
		
		if (diff >= fClockSpeed) {
			
			title << "Pretendo: " << frameCount << " FPS";
			SetTitle(title.String());			
			
			frameCount = 0;
			prevCount = curCount;
		}
	}
}

