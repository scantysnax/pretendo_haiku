
#include "PretendoWindow.h"

#include <cstdio>

// -----------------------------------------------------------------------------
// InvalidateWindowContents
//
// Invalidates the first child view of a debugger/tool window.  Tool windows own
// their drawing views, and BWindow itself does not provide Invalidate().
//
// Parameters:
//   window - Tool/debugger window whose main child view should be redrawn.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
InvalidateWindowContents (BWindow *window)
{
	if (!window) {
		return;
	}

	if (!window->Lock()) {
		return;
	}

	BView *child = window->ChildAt(0);

	if (child) {
		child->Invalidate();
	}

	window->Unlock();
}


// -----------------------------------------------------------------------------
// ResetCPUDisasmWindow
//
// Resets and invalidates the CPU disassembly view if the disassembly window is
// currently open.  This is used on ROM load/reset because InvalidateDebugViews()
// intentionally skips the disassembler during single-step refreshes.
//
// Parameters:
//   window - CPU disassembly tool window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
ResetCPUDisasmWindow(BWindow *window)
{
	if (!window) {
		return;
	}

	if (!window->Lock()) {
		return;
	}

	BView *child = window->ChildAt(0);
	CPUDisasmView *view = dynamic_cast<CPUDisasmView *>(child);

	if (view) {
		view->ResetView();
	} else if (child) {
		child->Invalidate();
	}

	window->Unlock();
}


// -----------------------------------------------------------------------------
// InvalidateViewTree
//
// Invalidates a view and all of its child views.  This is useful after display
// mode changes where app_server backing-store contents may be stale.
//
// Parameters:
//   view - Root view to invalidate.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
InvalidateViewTree(BView* view)
{
	if (!view) {
		return;
	}

	view->Invalidate(view->Bounds());

	for (int32 i = 0; ; i++) {
		BView* child = view->ChildAt(i);

		if (!child) {
			break;
		}

		InvalidateViewTree(child);
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::PretendoWindow
//
// Creates the main emulator window, initializes menus, video buffers, audio
// streaming, settings, palette state, and the emulator worker thread.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PretendoWindow::PretendoWindow()
	: BWindow (BRect (0, 0, screen_size::WIDTH-1, screen_size::HEIGHT-1), 
				"Pretendo", B_TITLED_WINDOW, B_NOT_RESIZABLE, 0)		
{
	// ui things
	AddMenu();
	BRect bounds(Bounds());
	bounds.OffsetTo(B_ORIGIN);
	bounds.top = fMenuHeight;
	fView = new PretendoView(bounds, this);
	AddChild(fView);
	fView->MakeFocus();
	SetFeel(B_NORMAL_WINDOW_FEEL);
	
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
	fBitmap = new BBitmap(BRect(0, 0, screen_size::WIDTH-1, screen_size::HEIGHT-1), 
							B_CMAP8, false, true);
	
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
	fFullScreen = 
	fFrameworkChanging = false;	
	fFramework = 
	fPrevFramework = video_framework::NONE;
	fDoubled = false;
	fClear = 0;
	
	if (overlayOK) {
		ChangeFramework(video_framework::OVERLAY);
	} else {
		ChangeFramework(video_framework::BITMAP);
	}
	
	// we can't change to full screen yet
	fVideoMenu->ItemAt(video_framework::FULLSCREEN)->SetEnabled(false);
	
	fOpenPanel = new ROMFilePanel();
	fOpenPanel->SetPanelDirectory(fROMDirectory);
	
	// sound
	// we don't need to upscale the buffer size if using the MediaKit, so divide it out
	fAudioStream = new AudioStream(nes::apu::frequency, 8, 1, nes::apu::buffer_size / 4);

	// this is the emulator processing loop
	// thread gets a cheeky name, as per Be Book
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
		"fibonacci sequence generator",
		"numbers station"
	};
	
	int32 const index = (rand() % 11);
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
	
	fSettingsMessage = new BMessage;
	LoadSettings();
	
	// do any post-settings setup
	
	// eli: we need to grab the palete from PaletteWindow and apply it
	// 		this is a super hack, but convenient for now
	// 		this will call the constructor to set the palette
	fPaletteWindow = new PaletteWindow(this, false); 
	
	// dispose of this since we don't need it anymore
	if (fPaletteWindow->Lock()) {
		fPaletteWindow->Quit();
		fPaletteWindow = nullptr;
	}
	
	// move MenuBarIcon according to window size
	int32 const scale = static_cast<int32>(fDoubled) + 1;
	int32 const x = screen_size::WIDTH * scale - MenuBarIcon::WIDTH - MenuBarIcon::PADDING;
	int32 const y = MenuBarIcon::PADDING;
	
	fMenuBarIcon->MoveTo(x, y);
}


// -----------------------------------------------------------------------------
// PretendoWindow::~PretendoWindow
//
// Stops and releases emulator resources, closes owned child windows, saves
// settings, and releases allocated video/audio objects.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
PretendoWindow::~PretendoWindow()
{	
	// break everything down and clean up
	fRunning = false;
	fThread = B_BAD_THREAD_ID;
	
	fAudioStream->Stop();
	delete fAudioStream;

	if (fBitmap->IsValid()) {
		delete fBitmap;
	}
	
	if (fOverlayBitmap->IsValid()) {
		delete fOverlayBitmap;
	}
	
	delete_area(fBitsArea);
	delete_area(fDirtyArea);
	
	// we don't delete BWindows, we call Quit()
	// note: calling  Quit() requires the window to be locked 
	
	if (fCPUDisasmWindow != nullptr) {
		fCPUDisasmWindow->Lock();
		fCPUDisasmWindow->Quit();
	}
	
	if (fCPUStatusWindow != nullptr) {
		fCPUStatusWindow->Lock();
		fCPUStatusWindow->Quit();
	}
	
	if (fInputWindow != nullptr) {
		if (fInputWindow->Lock()) {
			fInputWindow->Quit();
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
	
	if (fOAMDebugWindow != nullptr) {
		fOAMDebugWindow->Lock();
		fOAMDebugWindow->Quit();
	}
	
	if (fPaletteDebugWindow != nullptr) {
		fPaletteDebugWindow->Lock();
		fPaletteDebugWindow->Quit();
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
	
	if (fPPUMemoryWindow != nullptr) {
		fPPUMemoryWindow->Lock();
		fPPUMemoryWindow->Quit();
	}
	
	
	if (fPPUStatusWindow != nullptr) {
		fPPUStatusWindow->Lock();
		fPPUStatusWindow->Quit();
	}
	
	if (fPPUWriteLogWindow != nullptr) {
		fPPUWriteLogWindow->Lock();
		fPPUWriteLogWindow->Quit();
	}
	
	if (fROMInfoWindow != nullptr) {
		if (fROMInfoWindow->Lock()) {
			fROMInfoWindow->Quit();
		}
	}
	
	// long day.
	
	fMutex->Unlock();
	
	SaveSettings();
	
	delete fSettingsMessage;
	delete fOpenPanel;
	delete fROMDirectoryPanel;
	
	Hide();
	Sync();	
}


// -----------------------------------------------------------------------------
// PretendoWindow::MessageReceived
//
// Dispatches application messages from menus, file refs, video rendering,
// emulator controls, settings dialogs, and debugger/tool-window commands.
//
// Parameters:
//   message - Message received by the main window.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::MessageReceived (BMessage *message)
{
	switch (message->what) {
		case messages::DRAW_BITMAP:
			if (fView && fBitmap) {
				fView->SetDisplayBitmap(fBitmap);
				fView->CaptureLastFrame(fBitmap);

				// Keep the existing live-video path working.
				fView->DrawBitmap(fBitmap, fView->Bounds());
			}
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
			fOpenPanel->SetPanelDirectory(fROMDirectory);
			fOpenPanel->Show();
			break;
			
		case B_REFS_RECEIVED:
		{
			entry_ref ref;

			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);

				if (path.InitCheck() == B_OK) {
					LoadROMPath(path.Path());
				}
			}
			
		} break;

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
			
		case messages::SET_ROMDIR:
			OnSetRomDirectory();
			break;
			
		case messages::ADJ_PALETTE:
			OnAdjustPalette();
			break;
			
		case messages::VIEW_PTNTBL1:			
			OnViewPatternTable1();
			break;
			
		case messages::VIEW_PTNTBL2:
			OnViewPatternTable2();
			break;
		
		case messages::VIEW_NTBL1:
			OnViewNameTable1();
			break;
			
		case messages::VIEW_NTBL2:
			OnViewNameTable2();
			break;
			
		case messages::VIEW_NTBL3:
			OnViewNameTable3();
			break;
		
		case messages::VIEW_NTBL4:
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
			
		case messages::RECV_ROM_DIR:
			OnReceiveRomDirectory(message);
			break;
			
		case messages::VIEW_PALDBG:
			OnViewPaletteDebugger();
			break;
			
		case messages::VIEW_OAMDBG:
			OnViewOAMDebugger();
			break;
			
		case messages::VIEW_PPUSTAT:
			OnViewPPUStatusWindow();
			break;
			
		case messages::VIEW_PPULOG:
			OnViewPPUWriteLogWindow();
			break;
			
		case messages::VIEW_PPUMEM:
			OnViewPPUMemoryWindow();
			break;
			
		case messages::VIEW_CPUSTAT:
			OnViewCPUStatusWindow();
			break;
			
		case messages::VIEW_CPUDISASM:
			OnViewCPUDisasmWindow();
			break;
			
		default:
			break;
	}
		
	BWindow::MessageReceived (message);
}


// -----------------------------------------------------------------------------
// PretendoWindow::WindowActivated
//
// Handles main-window activation changes.  The current implementation delegates
// to BWindow and leaves emulator/tool focus state unchanged.
//
// Parameters:
//   flag - true when the window becomes active.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::WindowActivated (bool flag)
{
	BWindow::WindowActivated(flag);	
}


// -----------------------------------------------------------------------------
// PretendoWindow::MenusBeginning
//
// Builds the temporary recent-ROM submenu before the menu bar is displayed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::MenusBeginning()
{	
	// set up recently opened ROM menu, we keep 5 most recent
	// item list count seems to be off by 1
	int32 const recentItems = 5+1;
	BMenu *menu = BRecentFilesList::NewFileListMenu("Load ROM" B_UTF8_ELLIPSIS,
				  nullptr, nullptr, this->PreferredHandler(), recentItems, false, nullptr, 0, 
				  "application/x-vnd.scantysnax-Pretendo");
	
	fFileMenu->AddItem(new BMenuItem(menu, new BMessage(messages::SHOW_OPEN)), 0);
	
	BWindow::MenusBeginning();
}


// -----------------------------------------------------------------------------
// PretendoWindow::MenusEnded
//
// Removes the temporary recent-ROM submenu after menu tracking ends.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::MenusEnded()
{	
	// remove the recent files list
	fFileMenu->RemoveItem(static_cast<int32>(0)); // keep this 32-bit friendly
	
	BWindow::MenusEnded();
}


// -----------------------------------------------------------------------------
// PretendoWindow::QuitRequested
//
// Requests application shutdown, releases the emulator mutex to stop the worker
// thread, waits for the thread to exit, and posts B_QUIT_REQUESTED to the app.
//
// Parameters:
//   None.
//
// Returns:
//   true to allow the window to close.
// -----------------------------------------------------------------------------
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
	
	fRunning = false;
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


// -----------------------------------------------------------------------------
// PretendoWindow::ResizeTo
//
// Resizes the emulator window while accounting for the menu bar height.
//
// Parameters:
//   width  - Requested client width.
//   height - Requested emulator-view height, excluding the menu bar.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ResizeTo (float width, float height)
{
	height += fMenuHeight; // account for menubar height
	
	BWindow::ResizeTo (width, height);
}


// -----------------------------------------------------------------------------
// PretendoWindow::Zoom
//
// Toggles between 1x and 2x windowed video sizes and repositions the menu-bar
// icon.  The default BWindow zoom behavior is intentionally bypassed.
//
// Parameters:
//   origin - Requested zoom origin, unused.
//   width  - Requested zoom width, unused.
//   height - Requested zoom height, unused.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::Zoom (BPoint origin, float width, float height)
{
	(void)origin;
	(void)width;
	(void)height;
	
	int32 const w = Bounds().IntegerWidth();
			
	if (w == screen_size::WIDTH) {
		ResizeTo((screen_size::WIDTH*2), (screen_size::HEIGHT*2));
		fMenuBarIcon->MoveTo((w * 2) - MenuBarIcon::icon_size::PADDING - MenuBarIcon::icon_size::WIDTH, 
							MenuBarIcon::icon_size::PADDING);
		fDoubled = true;
	} else if (w == screen_size::WIDTH*2) {
		ResizeTo(screen_size::WIDTH, screen_size::HEIGHT);
		fMenuBarIcon->MoveTo((w / 2) - MenuBarIcon::icon_size::PADDING - MenuBarIcon::icon_size::WIDTH, 
							MenuBarIcon::icon_size::PADDING);
		fDoubled = false;
	} 
	
	fMenuHeight = fMenuBar->Bounds().IntegerHeight();
	
	// do not call the default //
}


// -----------------------------------------------------------------------------
// PretendoWindow::AddMenu
//
// Builds the main menu bar, emulator controls, video/audio settings menus, and
// debugger/tool menus.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::AddMenu()
{
	fMenuBar = new BMenuBar(BRect(0, 0, screen_size::WIDTH, screen_size::MENU_HEIGHT), "pretendo_menu");
	AddChild(fMenuBar);
	fFileMenu = new BMenu("File");
	fMenuBar->AddItem(fFileMenu);
	
	fEmuMenu = new BMenu("Emulator");
	fMenuBar->AddItem(fEmuMenu);
	
	fSettingsMenu = new BMenu("Settings");
	fMenuBar->AddItem(fSettingsMenu);
	
	fToolMenu = new BMenu("Tools");
	fMenuBar->AddItem(fToolMenu);
	
	// for "Load ROM..." see MenusBeginning and MenusEnded
	fFileMenu->AddItem(new BMenuItem("Free ROM", new BMessage(messages::FREE_ROM)));
	fFileMenu->AddItem(new BMenuItem("ROM Info" B_UTF8_ELLIPSIS, new BMessage(messages::ROM_INFO)));
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
	
	fVideoMenu = new BMenu("Video");
	fSettingsMenu->AddItem(fVideoMenu);
	fVideoMenu->AddItem(new BMenuItem("None", new BMessage (messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("Bitmap", new BMessage(messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("Overlay", new BMessage(messages::CHANGE_RENDER)));
	fVideoMenu->AddItem(new BMenuItem("WindowScreen", new BMessage(messages::CHANGE_RENDER), 'F'));
	fVideoMenu->SetRadioMode(true);
	
	fAudioMenu = new BMenu("Audio");
	fSettingsMenu->AddItem(fAudioMenu);
	fAudioMenu->AddItem(new BMenuItem("Square 1", new BMessage(messages::ENABLE_SQ1)));
	fAudioMenu->AddItem(new BMenuItem("Square 2", new BMessage(messages::ENABLE_SQ2)));
	fAudioMenu->AddItem(new BMenuItem("Triangle", new BMessage(messages::ENABLE_TRI)));
	fAudioMenu->AddItem(new BMenuItem("Noise", new BMessage(messages::ENABLE_NOISE)));
	fAudioMenu->AddItem(new BMenuItem("DMC/DPCM", new BMessage(messages::ENABLE_DMC)));
	
	// eli: move these to settings
	(fAudioMenu->ItemAt(nes::apu::sound_channel::SQUARE1))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::SQUARE2))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::TRIANGLE))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::NOISE))->SetMarked(true);
	(fAudioMenu->ItemAt(nes::apu::sound_channel::DPCM))->SetMarked(true);
	
	fSettingsMenu->AddItem(new BMenuItem("Adjust Palette" B_UTF8_ELLIPSIS, new BMessage(messages::ADJ_PALETTE)));
	fSettingsMenu->AddItem(new BMenuItem("Setup Input" B_UTF8_ELLIPSIS, new BMessage(messages::CFG_INPUT)));
	fSettingsMenu->AddItem(new BMenuItem("Set ROM Directory" B_UTF8_ELLIPSIS, new BMessage(messages::SET_ROMDIR)));
	
	fCPUToolMenu = new BMenu("CPU");
	fToolMenu->AddItem(fCPUToolMenu);
	fCPUToolMenu->AddItem(new BMenuItem("View Status" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_CPUSTAT)));
	fCPUToolMenu->AddItem(new BMenuItem("View Disassembly" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_CPUDISASM)));
	fCPUToolMenu->AddItem(new BMenuItem("View Memory" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_CPUMEM)));
	fCPUToolMenu->AddItem(new BMenuItem("View CPU Trace" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_CPUTRACE)));
	fCPUToolMenu->AddItem(new BMenuItem("View Zero Page", new BMessage(messages::VIEW_ZERO_PAGE)));
	fCPUToolMenu->AddItem(new BMenuItem("View Stack", new BMessage(messages::VIEW_STACK)));
	fCPUToolMenu->AddItem(new BMenuItem("View Breakpoints", new BMessage(messages::VIEW_BREAKPOINTS)));
	
	fPPUToolMenu = new BMenu("PPU");
	fToolMenu->AddItem(fPPUToolMenu);
	fPPUToolMenu->AddItem(new BMenuItem("View PPU Status" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_PPUSTAT)));
	fPPUToolMenu->AddItem(new BMenuItem("View PPU Write Log" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_PPULOG)));
	fPPUToolMenu->AddItem(new BMenuItem("View PPU Memory" B_UTF8_ELLIPSIS, new BMessage(messages::VIEW_PPUMEM)));
	fPPUToolMenu->AddItem(new BMenuItem("View Palettes", new BMessage(messages::VIEW_PALDBG)));
	fPPUToolMenu->AddItem(new BMenuItem("View OAM", new BMessage(messages::VIEW_OAMDBG)));
	fPPUToolMenu->AddSeparatorItem();

	fPatternTableMenu = new BMenu("View Pattern Tables");
	fPatternTableMenu->AddItem(new BMenuItem("1 ($0000)", new BMessage(messages::VIEW_PTNTBL1)));
	fPatternTableMenu->AddItem(new BMenuItem("2 ($1000)", new BMessage(messages::VIEW_PTNTBL2)));
	fPPUToolMenu->AddItem(fPatternTableMenu);
	
	fNameTableMenu = new BMenu("View Name Tables");
	fNameTableMenu->AddItem(new BMenuItem("1 ($2000)", new BMessage(messages::VIEW_NTBL1)));
	fNameTableMenu->AddItem(new BMenuItem("2 ($2400)", new BMessage(messages::VIEW_NTBL2)));
	fNameTableMenu->AddItem(new BMenuItem("3 ($2800)", new BMessage(messages::VIEW_NTBL3)));
	fNameTableMenu->AddItem(new BMenuItem("4 ($2C00)", new BMessage(messages::VIEW_NTBL4)));
	fPPUToolMenu->AddItem(fNameTableMenu);
	
	// menu icon
	fMenuBarIcon = new MenuBarIcon(fMenuBar);
	fMenuBar->AddChild(fMenuBarIcon);
	fMenuHeight = fMenuBar->Bounds().Height();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnLoadROM
//
// Loads a ROM from a message containing a "rom_path" string.  The actual ROM
// load/reset/debug refresh work is centralized in LoadROMPath() so manual file
// loading, recent-document loading, and other future load paths behave the same.
//
// Parameters:
//   message - Message containing the "rom_path" string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnLoadROM (BMessage *message)
{
	BString path;
	
	if (message->FindString("rom_path", &path) != B_OK) {
		return;
	}

	LoadROMPath(path.String());
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnFreeROM
//
// Stops emulation, clears debugger/input/audio state, unloads the current ROM,
// clears the cached video frame, and refreshes debugger windows so no stale ROM
// state remains visible.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnFreeROM()
{	
	OnStop();

	ClearControllerInput();

	nes::apu::debug_set_audio_muted(false);
	nes::ppu::system_paused = false;

	if (fAudioStream) {
		fAudioStream->ClearBuffer();
		fAudioStream->SetMuted(false);
	}

	nes::cart.unload();
	
	ClearVideoView();

	nes::cpu::debug_clear_instruction_trace();

	InvalidateDebugViews();
	ResetCPUDisasmWindow(fCPUDisasmWindow);
}


// -----------------------------------------------------------------------------
// PretendoWindow::ForceFullRedraw
//
// Forces the main emulator window and its child views to repaint after a display
// mode transition, such as leaving fullscreen.  The function clears the root
// view background, clears the emulator view, invalidates the view hierarchy, and
// then asks the window to process pending updates.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ForceFullRedraw()
{
	if (!Lock()) {
		return;
	}

	BView* root = ChildAt(0);

	if (root) {
		root->SetHighColor(216, 216, 216);
		root->FillRect(root->Bounds());
		InvalidateViewTree(root);
	}

	if (fView) {
		fView->SetHighColor(0, 0, 0);
		fView->FillRect(fView->Bounds());
		fView->Invalidate(fView->Bounds());
	}

	UpdateIfNeeded();

	Unlock();

	RedrawLastFrame();
}


// -----------------------------------------------------------------------------
// PretendoWindow::ForceFullBitmapRedraw
//
// Forces the next DrawBitmap() call to copy the entire emulator back buffer into
// the windowed front bitmap instead of relying on the dirty buffer.  This is
// useful after returning from fullscreen, where stale windowed pixels may not be
// represented in the dirty buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ForceFullBitmapRedraw()
{
	fForceFullBitmapRedraw = true;
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnROMInfo
//
// Opens the ROM information window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnROMInfo()
{
	if (fROMInfoWindow) {
		if (fROMInfoWindow->Lock()) {
			if (fROMInfoWindow->IsHidden())
				fROMInfoWindow->Show();

			fROMInfoWindow->Activate(true);
			fROMInfoWindow->Unlock();
		}

		return;
	}

	fROMInfoWindow = new ROMInfoWindow(this);
	BeginToolInput();
	fROMInfoWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnQuit
//
// Handles the Quit menu command by asking the application to terminate.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnQuit()
{	
	// seeya!
	be_app->PostMessage(B_QUIT_REQUESTED);
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnRun
//
// Starts or resumes emulation when a ROM is loaded.  On first run this performs
// a hard reset, unlocks the emulator mutex, starts audio, and enables fullscreen
// mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnStop
//
// Stops emulation, reacquires the emulator mutex when needed, stops audio,
// clears the active video target, and disables fullscreen mode.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnPause
//
// Toggles pause while the emulator is running by locking or unlocking the
// emulator mutex and starting or stopping host audio playback.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnPause()
{	
	if (fRunning) {
		if (fPaused) {
			// if we are paused, we want to unpause, so unlock the mutual exclusion
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnSoftReset
//
// Performs a soft reset of the loaded emulator state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnSoftReset()
{
	reset(nes::Reset::Soft);
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnHardReset
//
// Performs a hard reset of the loaded emulator state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnHardReset()
{
	reset(nes::Reset::Hard);
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnConfigureInput
//
// Opens the input configuration window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnConfigureInput()
{
	if (fInputWindow) {
		if (fInputWindow->Lock()) {
			if (fInputWindow->IsHidden())
				fInputWindow->Show();

			fInputWindow->Activate(true);
			fInputWindow->Unlock();
		}

		return;
	}

	fInputWindow = new InputWindow(this);
	BeginToolInput();
	fInputWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnSetRomDirectory
//
// Opens a directory-selection panel used to choose the default ROM directory.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnSetRomDirectory()
{
	fROMDirectoryPanel = new BFilePanel(B_OPEN_PANEL, nullptr, nullptr, B_DIRECTORY_NODE, false, 
										new BMessage(messages::RECV_ROM_DIR), nullptr, true, true);
	fROMDirectoryPanel->SetTarget(BMessenger(nullptr, this));
	fROMDirectoryPanel->Window()->SetTitle("Choose a Directory" B_UTF8_ELLIPSIS);
	fROMDirectoryPanel->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnAdjustPalette
//
// Opens the interactive palette adjustment window or brings the existing one
// forward.  This path uses the normal notifying PaletteWindow constructor and
// claims tool-input ownership while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnAdjustPalette()
{
	
	if (fPaletteWindow) {
		if (fPaletteWindow->Lock()) {
			if (fPaletteWindow->IsHidden())
				fPaletteWindow->Show();

			fPaletteWindow->Activate(true);
			fPaletteWindow->Unlock();
		}

		return;
	}

	fPaletteWindow = new PaletteWindow(this);
	BeginToolInput();
	fPaletteWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPatternTable1
//
// Opens pattern table window 1, or brings the existing window forward.  New
// windows claim tool-input ownership and are connected to open name table views.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPatternTable1()
{
	if (fPatternTable1Window) {
		if (fPatternTable1Window->Lock()) {
			if (fPatternTable1Window->IsHidden())
				fPatternTable1Window->Show();

			fPatternTable1Window->Activate(true);
			fPatternTable1Window->Unlock();
		}

		return;
	}

	fPatternTable1Window = new PatternTableWindow(this, 0);
	BeginToolInput();
	fPatternTable1Window->Show();

	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPatternTable2
//
// Opens pattern table window 2, or brings the existing window forward.  New
// windows claim tool-input ownership and are connected to open name table views.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPatternTable2()
{
	if (fPatternTable2Window) {
		if (fPatternTable2Window->Lock()) {
			if (fPatternTable2Window->IsHidden())
				fPatternTable2Window->Show();

			fPatternTable2Window->Activate(true);
			fPatternTable2Window->Unlock();
		}

		return;
	}

	fPatternTable2Window = new PatternTableWindow(this, 1);
	BeginToolInput();
	fPatternTable2Window->Show();

	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewNameTable1
//
// Opens name table window 1, or brings the existing window forward.  The window
// is connected to the current pattern table windows and claims tool-input
// ownership while open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewNameTable1()
{
	if (fNameTable1Window) {
		if (fNameTable1Window->Lock()) {
			if (fNameTable1Window->IsHidden())
				fNameTable1Window->Show();

			fNameTable1Window->Activate(true);
			fNameTable1Window->Unlock();
		}

		return;
	}

	fNameTable1Window = new NameTableWindow(
		this,
		0,
		fPatternTable1Window,
		fPatternTable2Window
	);

	BeginToolInput();
	fNameTable1Window->Show();
		
	ConnectDebugViews();	
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewNameTable2
//
// Opens name table window 2, or brings the existing window forward.  The window
// is connected to the current pattern table windows and claims tool-input
// ownership while open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewNameTable2()
{
	if (fNameTable2Window) {
		if (fNameTable2Window->Lock()) {
			if (fNameTable2Window->IsHidden())
				fNameTable2Window->Show();

			fNameTable2Window->Activate(true);
			fNameTable2Window->Unlock();
		}

		return;
	}

	fNameTable2Window = new NameTableWindow(
		this,
		1,
		fPatternTable1Window,
		fPatternTable2Window
	);

	BeginToolInput();
	fNameTable2Window->Show();
		
	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewNameTable3
//
// Opens name table window 3, or brings the existing window forward.  The window
// is connected to the current pattern table windows and claims tool-input
// ownership while open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewNameTable3()
{
	if (fNameTable3Window) {
		if (fNameTable3Window->Lock()) {
			if (fNameTable3Window->IsHidden())
				fNameTable3Window->Show();

			fNameTable3Window->Activate(true);
			fNameTable3Window->Unlock();
		}

		return;
	}
	
	fNameTable3Window = new NameTableWindow(
		this,
		2,
		fPatternTable1Window,
		fPatternTable2Window
	);

	BeginToolInput();
	fNameTable3Window->Show();
		
	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewNameTable4
//
// Opens name table window 4, or brings the existing window forward.  The window
// is connected to the current pattern table windows and claims tool-input
// ownership while open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewNameTable4()
{
	if (fNameTable4Window) {
		if (fNameTable4Window->Lock()) {
			if (fNameTable4Window->IsHidden())
				fNameTable4Window->Show();

			fNameTable4Window->Activate(true);
			fNameTable4Window->Unlock();
		}

		return;
	}

	fNameTable4Window = new NameTableWindow(
		this,
		3,
		fPatternTable1Window,
		fPatternTable2Window
	);

	BeginToolInput();
	fNameTable4Window->Show();
		
	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnAudioSquare1
//
// Toggles the APU square 1 channel menu state and mutes or unmutes that channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnAudioSquare2
//
// Toggles the APU square 2 channel menu state and mutes or unmutes that channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnAudioTriangle
//
// Toggles the APU triangle channel menu state and mutes or unmutes that channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnAudioNoise
//
// Toggles the APU noise channel menu state and mutes or unmutes that channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

	
// -----------------------------------------------------------------------------
// PretendoWindow::OnAudioDMC
//
// Toggles the APU DMC/DPCM channel menu state and mutes or unmutes that channel.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::OnReceiveRomDirectory
//
// Stores the selected ROM directory from a file-panel message, or falls back to
// /boot/home when the message does not contain a valid ref.
//
// Parameters:
//   message - Directory-selection message from the ROM directory panel.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnReceiveRomDirectory (BMessage *message)
{
	entry_ref ref;
		
	if (message->FindRef("refs", 0, &ref) == B_OK) {
		BEntry entry;
		BPath path;
		
		entry.SetTo(&ref, true);
		entry.GetPath(&path);
		fROMDirectory = path.Path();
	} else {
		fROMDirectory = "/boot/home";
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPaletteDebugger
//
// Opens the palette debugger window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPaletteDebugger()
{
	// If the Palette Viewer already exists, do not toggle it closed.
	// Bring it forward instead.
	if (fPaletteDebugWindow) {
		if (fPaletteDebugWindow->Lock()) {
			if (fPaletteDebugWindow->IsHidden())
				fPaletteDebugWindow->Show();

			fPaletteDebugWindow->Activate(true);
			fPaletteDebugWindow->Unlock();
		}

		return;
	}

	fPaletteDebugWindow = new PaletteDebugWindow(this);
	BeginToolInput();
	fPaletteDebugWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewOAMDebugger
//
// Opens the OAM debugger window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewOAMDebugger()
{
	if (fOAMDebugWindow) {
		if (fOAMDebugWindow->Lock()) {
			if (fOAMDebugWindow->IsHidden())
				fOAMDebugWindow->Show();

			fOAMDebugWindow->Activate(true);
			fOAMDebugWindow->Unlock();
		}

		return;
	}

	fOAMDebugWindow = new OAMDebugWindow(this);
	BeginToolInput();
	fOAMDebugWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPPUStatusWindow
//
// Opens the PPU status window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPPUStatusWindow()
{
	if (fPPUStatusWindow) {
		if (fPPUStatusWindow->Lock()) {
			if (fPPUStatusWindow->IsHidden())
				fPPUStatusWindow->Show();

			fPPUStatusWindow->Activate(true);
			fPPUStatusWindow->Unlock();
		}

		return;
	}

	fPPUStatusWindow = new PPUStatusWindow(this);
	BeginToolInput();
	fPPUStatusWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPPUWriteLogWindow
//
// Opens the PPU write-log window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPPUWriteLogWindow()
{
	if (fPPUWriteLogWindow) {
		if (fPPUWriteLogWindow->Lock()) {
			if (fPPUWriteLogWindow->IsHidden())
				fPPUWriteLogWindow->Show();

			fPPUWriteLogWindow->Activate(true);
			fPPUWriteLogWindow->Unlock();
		}

		return;
	}

	fPPUWriteLogWindow = new PPUWriteLogWindow(this);
	BeginToolInput();
	fPPUWriteLogWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewPPUMemoryWindow
//
// Opens the PPU memory window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewPPUMemoryWindow()
{
	if (fPPUMemoryWindow) {
		if (fPPUMemoryWindow->Lock()) {
			if (fPPUMemoryWindow->IsHidden())
				fPPUMemoryWindow->Show();

			fPPUMemoryWindow->Activate(true);
			fPPUMemoryWindow->Unlock();
		}

		return;
	}

	fPPUMemoryWindow = new PPUMemoryWindow(this);
	BeginToolInput();
	fPPUMemoryWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewCPUStatusWindow
//
// Opens the CPU status window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewCPUStatusWindow()
{
	if (fCPUStatusWindow) {
		if (fCPUStatusWindow->Lock()) {
			if (fCPUStatusWindow->IsHidden())
				fCPUStatusWindow->Show();

			fCPUStatusWindow->Activate(true);
			fCPUStatusWindow->Unlock();
		}

		return;
	}

	fCPUStatusWindow = new CPUStatusWindow(this);
	BeginToolInput();
	fCPUStatusWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::OnViewCPUDisasmWindow
//
// Opens the CPU disassembly window or brings the existing one forward.  New
// interactive tool windows claim tool-input ownership so fullscreen global input
// polling is disabled while the window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OnViewCPUDisasmWindow()
{
	if (fCPUDisasmWindow) {
		if (fCPUDisasmWindow->Lock()) {
			if (fCPUDisasmWindow->IsHidden())
				fCPUDisasmWindow->Show();

			fCPUDisasmWindow->Activate(true);
			fCPUDisasmWindow->Unlock();
		}

		return;
	}

	fCPUDisasmWindow = new CPUDisasmWindow(this);
	BeginToolInput();
	fCPUDisasmWindow->Show();
}


// -----------------------------------------------------------------------------
// PretendoWindow::ROMInfoWindowClosed
//
// Releases tool-input ownership for the ROM info window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ROMInfoWindowClosed()
{
	EndToolInput();
	fROMInfoWindow = nullptr;
}

// -----------------------------------------------------------------------------
// PretendoWindow::PaletteWindowClosed
//
// Releases tool-input ownership for the palette window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PaletteWindowClosed()
{
	EndToolInput();
	fPaletteWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::InputWindowClosed
//
// Releases tool-input ownership for the input configuration window and clears
// the stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::InputWindowClosed()
{
	EndToolInput();
	fInputWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::PatternTable1WindowClosed
//
// Releases tool-input ownership for the first Pattern Table window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PatternTable1WindowClosed()
{
	EndToolInput();
	fPatternTable1Window = nullptr;

	ConnectDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::PatternTable2WindowClosed
//
// Releases tool-input ownership for the second Pattern Table window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PatternTable2WindowClosed()
{
	EndToolInput();
	fPatternTable2Window = nullptr;

	ConnectDebugViews();
}

// -----------------------------------------------------------------------------
// PretendoWindow::NameTableWindowClosed
//
// Releases tool-input ownership for the specified Name Table debugger window
// and clears the corresponding stored window pointer.
//
// Parameters:
//   which - Name-table window index:
//             0 = $2000
//             1 = $2400
//             2 = $2800
//             3 = $2C00
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::NameTableWindowClosed (int32 which)
{
	EndToolInput();

	switch (which) {
		case 0:
			fNameTable1Window = nullptr;
			break;

		case 1:
			fNameTable2Window = nullptr;
			break;

		case 2:
			fNameTable3Window = nullptr;
			break;

		case 3:
			fNameTable4Window = nullptr;
			break;

		default:
			break;
	}
}

// -----------------------------------------------------------------------------
// PretendoWindow::PaletteDebugWindowClosed
//
// Releases tool-input ownership for the palette debug window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PaletteDebugWindowClosed()
{
	EndToolInput();
	fPaletteDebugWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::OAMDebugWindowClosed
//
// Releases tool-input ownership for the OAM Debugger window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::OAMDebugWindowClosed()
{
	EndToolInput();
	fOAMDebugWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::PPUStatusWindowClosed
//
// Releases tool-input ownership for the PPU Status window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PPUStatusWindowClosed()
{
	EndToolInput();
	fPPUStatusWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::PPUWriteLogWindowClosed
//
// Releases tool-input ownership for the PPU Write Log window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PPUWriteLogWindowClosed()
{
	EndToolInput();
	fPPUWriteLogWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::PPUMemoryWindowClosed
//
// Releases tool-input ownership for the PPU Memory window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::PPUMemoryWindowClosed()
{
	EndToolInput();
	fPPUMemoryWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::CPUStatusWindowClosed
//
// Releases tool-input ownership for the CPU Status window and clears the
// stored window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::CPUStatusWindowClosed()
{
	EndToolInput();
	fCPUStatusWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::CPUDisasmWindowClosed
//
// Handles the CPU disassembly window closing.
//
// Execute BreakPoints and any latched BreakPoint-hit state are cleared so a
// debugger condition owned by the disassembly window cannot remain active after
// the window is gone.
//
// If the CPU debugger itself placed the emulator into debugger-paused state,
// closing the window resumes normal execution.  A normal user-requested emulator
// Pause is not disturbed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::CPUDisasmWindowClosed()
{
	fCPUDisasmWindow = nullptr;

	EndToolInput();

	nes::cpu::debug_clear_execute_breakpoints();
	nes::cpu::debug_clear_breakpoint_hit();

	/*
	 * Only resume a pause owned by the debugger.
	 *
	 * Do not use fPaused by itself here: fPaused is also used by the normal
	 * Emulator -> Pause command, and closing a debugger must not override a
	 * pause explicitly requested by the user.
	 */
	if (!fDebuggerPausedEmulation) {
		return;
	}

	if (!fRunning) {
		fDebuggerPausedEmulation = false;
		nes::ppu::system_paused = false;
		return;
	}

	DebugResumeExecution();
}


// -----------------------------------------------------------------------------
// PretendoWindow::CPUMemoryWindowClosed
//
// Releases tool-input ownership for the CPU Memory window and clears the stored
// window pointer.  This is called by the child window as it closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::CPUMemoryWindowClosed()
{
	EndToolInput();
	fCPUMemoryWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::CPUTraceWindowClosed
//
// Handles CPU trace window close notification.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::CPUTraceWindowClosed()
{
	fCPUTraceWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::StackWindowClosed
//
// Clears the Stack debugger window pointer after the window closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::StackWindowClosed()
{
	fStackWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::ZeroPageWindowClosed
//
// Clears the Zero Page debugger window pointer after the window closes.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ZeroPageWindowClosed()
{
	fZeroPageWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::BreakPointWindowClosed
//
// Clears the main-window pointer to the BreakPoint Manager after that debugger
// window has been closed.
//
// BreakPointWindow is owned by the application windowing system once created.
// When the user closes it, the BreakPointWindow object is destroyed.  This
// callback prevents PretendoWindow from retaining a stale pointer to that
// destroyed window.
//
// Without this reset, a later attempt to reopen the BreakPoint Manager could
// try to call methods such as Lock(), Show(), or Activate() through an invalid
// pointer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::BreakPointWindowClosed()
{
	fBreakPointWindow = nullptr;
}


// -----------------------------------------------------------------------------
// PretendoWindow::RenderLine8
//
// Converts one NES RGB scanline into an 8-bit color-map destination buffer using
// the active mapped palette.
//
// Parameters:
//   dest   - Destination scanline buffer.
//   source - Source NES color-index scanline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::RenderLine16
//
// Converts one NES RGB scanline into a 16-bit destination buffer using the
// active mapped palette.
//
// Parameters:
//   dest   - Destination scanline buffer.
//   source - Source NES color-index scanline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::RenderLine32
//
// Converts one NES RGB scanline into a 32-bit destination buffer using the
// active mapped palette.
//
// Parameters:
//   dest   - Destination scanline buffer.
//   source - Source NES color-index scanline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::ClearDirty
//
// Performs the deferred dirty-buffer clear/toggle operation used by the legacy
// dirty blitters to force pending screen updates.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::ClearBitmap
//
// Clears either the overlay bitmap or the normal windowed bitmap to the proper
// blank video value for its color format.
//
// Parameters:
//   overlay - true to clear the overlay bitmap, false to clear the BBitmap.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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



// -----------------------------------------------------------------------------
// PretendoWindow::SetRenderer
//
// Selects the scanline renderer and mapped palette tables for the active front
// buffer color space.
//
// Parameters:
//   cs - Color space of the active front buffer.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::SetFrontBuffer
//
// Configures the active video front buffer, prepares fullscreen/windowed buffer
// layout, and selects the appropriate scanline renderer.
//
// Parameters:
//   bits        - Front-buffer base pointer.
//   cs          - Front-buffer color space.
//   pixel_width - Number of destination bytes per NES pixel in the back buffer.
//   row_bytes   - Number of bytes per front-buffer row.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::FinishExitFullScreen
//
// Finishes the transition back from fullscreen mode.  Controller input is
// cleared so fullscreen-polled keys do not stick.  The next windowed bitmap
// update is forced to repaint the full frame so stale fullscreen/windowed pixels
// do not remain visible.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::FinishExitFullScreen()
{
	fFullScreen = false;

	ClearControllerInput();

	ForceFullBitmapRedraw();
	ForceFullRedraw();

	snooze(50000);

	ForceFullBitmapRedraw();
	ForceFullRedraw();

	InvalidateDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::ChangeFramework
//
// Switches between video output frameworks, tears down the previous video path,
// configures the new path, and performs fullscreen-exit cleanup when needed.
//
// Parameters:
//   fw - New video framework to activate.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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
			
	// Break down previous framework.
	const bool leavingFullScreen = fPrevFramework == video_framework::FULLSCREEN;

	switch (fPrevFramework) {
		case video_framework::NONE:
			// Nothing to do here.
			break;
			
		case video_framework::BITMAP:
			ClearBitmap(false);
			break;
			
		case video_framework::OVERLAY:
			ClearBitmap(true);

			if (fView) {
				fView->ClearViewOverlay();
				fView->SetViewColor(0, 0, 0);
				fView->Invalidate();
			}

			break;
			
		case video_framework::FULLSCREEN:
			ClearControllerInput();

			if (fVideoScreen && fVideoScreen->Lock()) {
				fVideoScreen->Quit();
			}

			fVideoScreen = nullptr;
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
			
		case video_framework::FULLSCREEN:
			ClearControllerInput();

			fVideoScreen = new VideoScreen(this);
			fVideoScreen->Show();

			snooze(1000000);	// Wait a little while for the screen to connect.

			SetFrontBuffer(
				fVideoScreen->Bits(),
				B_CMAP8,
				fVideoScreen->PixelWidth() / 2,
				fVideoScreen->RowBytes()
			);

			fFullScreen = true;
			ClearControllerInput();
			break;
		}
	
		if (leavingFullScreen && fFramework != video_framework::FULLSCREEN) {
			FinishExitFullScreen();
		}
	
	fFrameworkChanging = false;
}


// -----------------------------------------------------------------------------
// PretendoWindow::DrawBitmap
//
// Copies the emulator back buffer into the windowed bitmap.  Normally this uses
// the dirty buffer to update only changed pixels.  After fullscreen/display-mode
// transitions, a one-shot full copy is used so stale windowed pixels are
// completely overwritten.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::DrawBitmap()
{
	uint8* dest = fBitmapBits;
	uint8* source = fBackBuffer.bits;
	uint8* dirty = fDirtyBuffer.bits;
	
	size_t const size = screen_size::WIDTH;
	size_t height = screen_size::HEIGHT;

	if (fForceFullBitmapRedraw) {
		while (height--) {
			// fBitmap is B_CMAP8, so copy one byte per visible pixel.
			// Do not multiply by fPixelWidth here.
			mmx_copy(dest, source, size);

			dest += fFrontBuffer.row_bytes;
			source += fBackBuffer.row_bytes;
		}

		fForceFullBitmapRedraw = false;

		PostMessage(messages::DRAW_BITMAP);
		return;
	}
	
	while (height--) {
		blit_windowed_dirty_mmx(source, dirty, dest, size, fPixelWidth);

		dest += fFrontBuffer.row_bytes;
		source += fBackBuffer.row_bytes;
		dirty += fBackBuffer.row_bytes;
	}

	PostMessage(messages::DRAW_BITMAP);	
}


// -----------------------------------------------------------------------------
// PretendoWindow::DrawOverlay
//
// Draws the overlay video path.  The legacy implementation is currently disabled
// because overlay support is unavailable on the target hardware/path.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// PretendoWindow::DrawFullScreen
//
// Copies dirty NES back-buffer pixels to the fullscreen front buffer using the
// fullscreen 2x dirty blitter.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------
// PretendoWindow::BlitScreen
//
// Dispatches the completed frame to the currently selected video framework,
// unless a framework change is in progress.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
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
			
		case video_framework::FULLSCREEN:
			DrawFullScreen();
			break;
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::RedrawLastFrame
//
// Redraws the most recently completed video frame while the emulator is paused.
// The main window is locked before touching its child view because this may be
// called from a debugger window thread.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::RedrawLastFrame()
{
	if (!Lock()) {
		return;
	}

	BView *child = ChildAt(0);

	if (child) {
		child->Invalidate();
		child->Window()->UpdateIfNeeded();
	}

	Unlock();
}


// -----------------------------------------------------------------------------
// PretendoWindow::ClearVideoView
//
// Clears the cached video frame and redraws the main emulator view black.  This
// prevents the previous ROM's final frame from remaining visible after a ROM is
// unloaded or replaced.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ClearVideoView()
{
	if (fView) {
		fView->ClearLastFrame();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::SubmitScanline
//
// Receives a completed scanline from the OS-neutral NES core and forwards it to
// the existing PretendoWindow scanline submission path.
//
// Parameters:
//   y      - Output scanline number in the range 0..239.
//   pixels - Pointer to the 256 rendered 32-bit pixels.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::SubmitScanline(int32_t y, const uint32_t *pixels)
{
	submit_scanline(y, pixels);
}


// -----------------------------------------------------------------------------
// PretendoWindow::submit_scanline
//
// Receives one rendered NES scanline and converts it into the emulator back
// buffer using the active scanline renderer.
//
// Parameters:
//   scanline - Scanline index to write.
//   source   - Source NES color-index scanline.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::submit_scanline(int scanline, const uint32_t *source)
{
	(this->*LineRenderer)(fLineOffsets[scanline], source);
}


// -----------------------------------------------------------------------------
// PretendoWindow::set_palette
//
// Builds mapped host palettes for 8-bit, 16-bit, 32-bit, and overlay output
// from the emulator RGB palette and color-emphasis table.
//
// Parameters:
//   intensity - Color-emphasis multiplier table.
//   pal       - Source RGB NES palette.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
PretendoWindow::set_palette (const color_emphasis_t *intensity, const rgb_color_t *pal)
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


// -----------------------------------------------------------------------------
// PretendoWindow::start_frame
//
// Prepares per-frame video state before the emulator renders a frame.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void  
PretendoWindow::start_frame()
{	
	fPixelWidth = fFrontBuffer.pixel_width;
	fBackBuffer.row_bytes = screen_size::WIDTH * fPixelWidth;
}


// -----------------------------------------------------------------------------
// PretendoWindow::end_frame
//
// Finalizes one emulator frame by blitting video and streaming one frame worth
// of APU samples to the host audio stream.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void	
PretendoWindow::end_frame()
{
	size_t const bufferSize = nes::apu::frequency / nes::apu::frame_rate;
	uint8 sampleBuffer[bufferSize];
	size_t const bufferCount = nes::apu::read_samples(sampleBuffer, sizeof(sampleBuffer));
	
	BlitScreen();
	fAudioStream->Stream(sampleBuffer, bufferCount);
}


// -----------------------------------------------------------------------------
// PretendoWindow::emulator_thread
//
// Main emulator thread.  In windowed mode, controller input is handled by
// PretendoView key events.  In fullscreen mode, global key polling is used
// because normal BView keyboard focus may not be reliable.
//
// Global polling is disabled while debugger input is active or while the
// emulator is debugger-paused, preventing debugger shortcuts from leaking into
// NES controller input.
//
// Parameters:
//   data - PretendoWindow pointer.
//
// Returns:
//   B_OK when the thread exits.
// -----------------------------------------------------------------------------
status_t
PretendoWindow::emulator_thread(void* data)
{
	PretendoWindow* window = reinterpret_cast<PretendoWindow*>(data);	
	
	while (1) {
		if (window->LockMutex() == false) { 
			break;
		}
		
		if (!nes::ppu::system_paused) {
			window->start_frame();
			nes::run_frame(window);
			window->end_frame();

			if (window->ShouldPollGlobalInput()) {
				window->ReadKeyStates();
			}
		}
		
		window->UnlockMutex();

		if (nes::ppu::system_paused) {
			snooze(10000);
		}
	}	
	
	return B_OK;
}


// -----------------------------------------------------------------------------
// PretendoWindow::CheckKey
//
// Updates one NES controller button from Haiku's global key state table.  This
// is used only while fullscreen mode is active, where normal BView key events
// may not be delivered reliably.
//
// Parameters:
//   index - NES controller button index.
//   key   - Haiku key code to test.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
inline void
PretendoWindow::CheckKey (int32 index, int32 key)
{
	nes::input::controller1.keystate_[index] = 
		fKeyStates.key_states[key >> 3] & (1 << (7 - (key % 8)));
}


// -----------------------------------------------------------------------------
// PretendoWindow::ReadKeyStates
//
// Polls global keyboard state and maps configured keys to NES controller input.
// This is intentionally used only in fullscreen mode so debugger/tool-window
// shortcuts do not also trigger emulator controller buttons.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
inline void
PretendoWindow::ReadKeyStates()
{
	get_key_info(&fKeyStates);
	
	CheckKey(Controller::INDEX_UP, 		fUpKey);
	CheckKey(Controller::INDEX_DOWN, 	fDownKey);
	CheckKey(Controller::INDEX_LEFT, 	fLeftKey);
	CheckKey(Controller::INDEX_RIGHT, 	fRightKey);
	CheckKey(Controller::INDEX_SELECT,	fSelectKey);
	CheckKey(Controller::INDEX_START, 	fStartKey);
	CheckKey(Controller::INDEX_B, 		fBKey);
	CheckKey(Controller::INDEX_A, 		fAKey);
}


// -----------------------------------------------------------------------------
// PretendoWindow::HandleEmulatorKey
//
// Updates controller-1 input state from a focused PretendoView key event.  This
// is the normal windowed-mode input path and prevents debugger/tool-window
// shortcuts from also affecting emulator input.
//
// Parameters:
//   key     - Haiku key code.
//   pressed - true on key down, false on key up.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::HandleEmulatorKey(int32 key, bool pressed)
{
	const uint8 value = pressed ? 1 : 0;

	if (key == fUpKey) {
		nes::input::controller1.keystate_[Controller::INDEX_UP] = value;
	} else if (key == fDownKey) {
		nes::input::controller1.keystate_[Controller::INDEX_DOWN] = value;
	} else if (key == fLeftKey) {
		nes::input::controller1.keystate_[Controller::INDEX_LEFT] = value;
	} else if (key == fRightKey) {
		nes::input::controller1.keystate_[Controller::INDEX_RIGHT] = value;
	} else if (key == fSelectKey) {
		nes::input::controller1.keystate_[Controller::INDEX_SELECT] = value;
	} else if (key == fStartKey) {
		nes::input::controller1.keystate_[Controller::INDEX_START] = value;
	} else if (key == fBKey) {
		nes::input::controller1.keystate_[Controller::INDEX_B] = value;
	} else if (key == fAKey) {
		nes::input::controller1.keystate_[Controller::INDEX_A] = value;
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::ClearControllerInput
//
// Releases all controller-1 buttons.  This prevents stuck buttons when changing
// focus, entering debugger step mode, or switching between windowed and
// fullscreen input paths.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ClearControllerInput()
{
	nes::input::controller1.keystate_[Controller::INDEX_UP] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_DOWN] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_LEFT] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_RIGHT] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_SELECT] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_START] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_B] = 0;
	nes::input::controller1.keystate_[Controller::INDEX_A] = 0;
}


// -----------------------------------------------------------------------------
// PretendoWindow::BeginToolInput
//
// Marks that a debugger/tool window owns keyboard input.  While at least one
// tool window is active, fullscreen global keyboard polling is disabled so tool
// shortcuts cannot leak into NES controller input.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::BeginToolInput()
{
	fToolInputDepth++;

	ClearControllerInput();
}


// -----------------------------------------------------------------------------
// PretendoWindow::EndToolInput
//
// Releases one debugger/tool-window keyboard ownership claim.  The counter is
// clamped at zero so duplicate close/destruction paths cannot make it negative.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::EndToolInput()
{
	if (fToolInputDepth > 0) {
		fToolInputDepth--;
	} else {
		fToolInputDepth = 0;
	}

	if (fToolInputDepth == 0) {
		ClearControllerInput();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::ResetToolInput
//
// Clears all debugger/tool keyboard ownership state and releases controller
// input.  Use this only when all tool windows have also been closed or
// invalidated, because it discards the open-window counter.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ResetToolInput()
{
	fToolInputDepth = 0;

	ClearControllerInput();
}


// -----------------------------------------------------------------------------
// PretendoWindow::ToolInputActive
//
// Returns whether any debugger/tool window currently owns keyboard input.
//
// Parameters:
//   None.
//
// Returns:
//   true if one or more tool windows own keyboard input.
// -----------------------------------------------------------------------------
bool
PretendoWindow::ToolInputActive() const
{
	return fToolInputDepth > 0;
}


// -----------------------------------------------------------------------------
// PretendoWindow::ShouldPollGlobalInput
//
// Returns whether global keyboard polling should be used for emulator input.
// Global polling is needed in fullscreen mode, but it is disabled whenever a
// debugger/tool window owns keyboard input or the emulator is debugger-paused.
//
// Parameters:
//   None.
//
// Returns:
//   true if fullscreen global input polling should be active.
// -----------------------------------------------------------------------------
bool
PretendoWindow::ShouldPollGlobalInput() const
{
	if (!fFullScreen) {
		return false;
	}

	if (ToolInputActive()) {
		return false;
	}

	if (nes::ppu::system_paused) {
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------
// PretendoWindow::ConnectDebugViews
//
// Reconnects open name table windows to the currently open pattern table
// windows so cross-window highlighting remains valid.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ConnectDebugViews()
{
	if (fNameTable1Window && fNameTable1Window->Lock()) {
		fNameTable1Window->SetPatternTables(fPatternTable1Window, fPatternTable2Window);
		fNameTable1Window->Unlock();
	}

	if (fNameTable2Window && fNameTable2Window->Lock()) {
		fNameTable2Window->SetPatternTables(fPatternTable1Window, fPatternTable2Window);
		fNameTable2Window->Unlock();
	}

	if (fNameTable3Window && fNameTable3Window->Lock()) {
		fNameTable3Window->SetPatternTables(fPatternTable1Window, fPatternTable2Window);
		fNameTable3Window->Unlock();
	}

	if (fNameTable4Window && fNameTable4Window->Lock()) {
		fNameTable4Window->SetPatternTables(fPatternTable1Window, fPatternTable2Window);
		fNameTable4Window->Unlock();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::DebugStepInstruction
//
// Enters debugger step mode and advances one CPU instruction.  Controller input
// is cleared before stepping so debugger shortcuts, such as S, do not also act
// as emulator controller buttons.
//
// The debugger pause is tracked separately from the emulator's normal Pause
// command so closing the debugger can safely resume execution only when the
// debugger itself caused the pause.
//
// Audio output is muted while debugger stepping is active.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::DebugStepInstruction()
{
	if (!nes::cart.mapper()) {
		return;
	}

	nes::apu::debug_set_audio_muted(true);

	if (fAudioStream) {
		fAudioStream->SetMuted(true);
	}

	nes::ppu::system_paused = true;
	fDebuggerPausedEmulation = true;

	ClearControllerInput();

	if (!LockMutex(2000000)) {
		printf("DebugStepInstruction: timed out waiting for emulator mutex\n");
		return;
	}

	nes::debug_step_instruction();

	UnlockMutex();

	RedrawLastFrame();
	InvalidateDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::LoadROMPath
//
// Loads a ROM from a filesystem path using the shared ROM-load path.  This keeps
// manual loads, recent-document loads, and future drag/drop loads consistent.
//
// Parameters:
//   path - Filesystem path to the ROM image.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::LoadROMPath(const char* path)
{
	if (!path) {
		return;
	}

	OnFreeROM();

	if (nes::cart.load(path) == false) {
		(new BAlert(
			"Error",
			"Error.  Couldn't load ROM Image.",
			"Okay",
			nullptr,
			nullptr,
			B_WIDTH_AS_USUAL,
			B_STOP_ALERT
		))->Go();

		return;
	}

	nes::reset(nes::Reset::Hard);

	ClearControllerInput();

	nes::apu::debug_set_audio_muted(false);
	nes::ppu::system_paused = false;

	if (fAudioStream) {
		fAudioStream->ClearBuffer();
		fAudioStream->SetMuted(false);
	}

	nes::cpu::debug_clear_instruction_trace();

	InvalidateDebugViews();
	ResetCPUDisasmWindow(fCPUDisasmWindow);
}


// -----------------------------------------------------------------------------
// PretendoWindow::DebugResumeExecution
//
// Leaves debugger step mode, clears controller input, restores audio, releases
// the debugger-owned pause state, resumes normal PPU execution, and refreshes
// debugger windows.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::DebugResumeExecution()
{
	ClearControllerInput();

	nes::apu::debug_set_audio_muted(false);

	if (fAudioStream) {
		fAudioStream->ClearBuffer();
		fAudioStream->SetMuted(false);
	}

	fDebuggerPausedEmulation = false;
	nes::ppu::system_paused = false;

	InvalidateDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::DebugStepFrame
//
// Enters debugger frame-step mode and advances one full PPU frame.  Controller
// input is cleared before stepping so debugger shortcuts do not also act as
// emulator controller buttons.
//
// The debugger pause is tracked separately from the emulator's normal Pause
// command so closing the debugger can safely resume execution only when the
// debugger itself caused the pause.
//
// Audio output is muted while debugger stepping is active.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::DebugStepFrame()
{
	if (!nes::cart.mapper()) {
		return;
	}

	nes::apu::debug_set_audio_muted(true);

	if (fAudioStream) {
		fAudioStream->SetMuted(true);
	}

	nes::ppu::system_paused = true;
	fDebuggerPausedEmulation = true;

	ClearControllerInput();

	if (!LockMutex(2000000)) {
		printf("DebugStepFrame: timed out waiting for emulator mutex\n");
		return;
	}

	nes::debug_step_frame();

	UnlockMutex();

	RedrawLastFrame();
	InvalidateDebugViews();
}


// -----------------------------------------------------------------------------
// PretendoWindow::HighlightPaletteDebugger
//
// Sends an external palette-entry highlight request to the palette debugger when
// that debugger window is open.
//
// Parameters:
//   sprites - true for sprite palettes, false for background palettes.
//   palette - Palette index to highlight.
//   entry   - Entry index within the palette.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::HighlightPaletteDebugger (bool sprites, int32 palette, int32 entry)
{
	if (!fPaletteDebugWindow)
		return;

	if (fPaletteDebugWindow->Lock()) {
		fPaletteDebugWindow->SetExternalHighlight(sprites, palette, entry);
		fPaletteDebugWindow->Unlock();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::JumpCPUDisasmToAddress
//
// Opens or activates the CPU disassembly window and jumps it to the requested
// CPU address.  This may be called from another debugger window's looper, so the
// CPU disassembly window must be locked before its view is modified.
//
// Parameters:
//   address - CPU address to show in the disassembly view.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::JumpCPUDisasmToAddress (uint16 address)
{
	if (!fCPUDisasmWindow) {
		fCPUDisasmWindow = new CPUDisasmWindow(this);
		fCPUDisasmWindow->Show();
	} else {
		fCPUDisasmWindow->Activate(true);
	}

	if (!fCPUDisasmWindow) {
		return;
	}

	if (fCPUDisasmWindow->Lock()) {
		fCPUDisasmWindow->JumpToAddress(address);
		fCPUDisasmWindow->Unlock();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::IsEmulatorRunning
//
// Returns whether the emulator session has been started.
//
// A loaded ROM does not necessarily mean the emulator is running.  Before the
// user starts execution, or after Stop, fRunning is false.
//
// Parameters:
//   None.
//
// Returns:
//   true if the emulator session is active; false otherwise.
// -----------------------------------------------------------------------------
bool
PretendoWindow::IsEmulatorRunning() const
{
	return fRunning;
}


// -----------------------------------------------------------------------------
// PretendoWindow::IsEmulatorPaused
//
// Returns whether emulator execution is currently paused.
//
// This includes both the normal emulator Pause state and a debugger-controlled
// pause caused by stepping or hitting a BreakPoint.
//
// Parameters:
//   None.
//
// Returns:
//   true if emulator execution is paused; false otherwise.
// -----------------------------------------------------------------------------
bool
PretendoWindow::IsEmulatorPaused() const
{
	return fPaused || fDebuggerPausedEmulation;
}


// -----------------------------------------------------------------------------
// PretendoWindow::InvalidateDebugViews
//
// Refreshes open debugger/tool windows after a debugger-controlled state
// change, such as single-stepping one CPU instruction.
//
// CPUDisasmWindow is intentionally not invalidated here because the disassembly
// view is usually the caller during single-step and will invalidate itself after
// updating its PC/follow state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::InvalidateDebugViews()
{
	InvalidateWindowContents(fROMInfoWindow);
	InvalidateWindowContents(fPaletteWindow);

	InvalidateWindowContents(fPatternTable1Window);
	InvalidateWindowContents(fPatternTable2Window);

	InvalidateWindowContents(fNameTable1Window);
	InvalidateWindowContents(fNameTable2Window);
	InvalidateWindowContents(fNameTable3Window);
	InvalidateWindowContents(fNameTable4Window);

	InvalidateWindowContents(fInputWindow);

	InvalidateWindowContents(fPaletteDebugWindow);
	InvalidateWindowContents(fOAMDebugWindow);

	InvalidateWindowContents(fPPUStatusWindow);
	InvalidateWindowContents(fPPUWriteLogWindow);
	InvalidateWindowContents(fPPUMemoryWindow);

	InvalidateWindowContents(fCPUStatusWindow);

	// Do not invalidate fCPUDisasmWindow here.
	// CPUDisasmView::KeyDown() updates and invalidates itself after stepping.
}


// -----------------------------------------------------------------------------
// PretendoWindow::ClearPaletteDebuggerHighlight
//
// Clears any external highlight currently displayed by the palette debugger
// window, when that window is open.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::ClearPaletteDebuggerHighlight()
{
	if (!fPaletteDebugWindow)
		return;

	if (fPaletteDebugWindow->Lock()) {
		fPaletteDebugWindow->ClearExternalHighlight();
		fPaletteDebugWindow->Unlock();
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::LoadSettings
//
// Loads persisted window position, scale, ROM directory, and input bindings from
// the Pretendo settings file, creating default settings when needed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::LoadSettings()
{
	BString path = Settings::configDirectory().c_str();
	path += "/pretendo_window";
		
	// open settings file.  create a new one if it doesn't exist
	BFile file;
	status_t status;
	off_t size;
	
	status = file.SetTo(path, B_READ_WRITE|B_CREATE_FILE);
	
	if (status == B_OK) {
		file.GetSize(&size);
		
		// if file is empty, load some defaults
		if (size == 0) {
			CenterOnScreen();
			
			int32 const x = Frame().left;
			int32 const y = Frame().top;
			bool const doubled = false;
			BString const path = "/boot/home";
			
			// stash default settings
			fSettingsMessage->AddInt32("window_x", x);
			fSettingsMessage->AddInt32("window_y", y);
			fSettingsMessage->AddBool("double_size", doubled);
			fSettingsMessage->AddString("rom_dir", path);
			
			// input
			fSettingsMessage->AddInt8("input_up_key", default_keys::UP);
			fSettingsMessage->AddInt8("input_down_key", default_keys::DOWN);
			fSettingsMessage->AddInt8("input_left_key", default_keys::LEFT);
			fSettingsMessage->AddInt8("input_right_key", default_keys::RIGHT);
			fSettingsMessage->AddInt8("input_select_key", default_keys::SELECT);
			fSettingsMessage->AddInt8("input_start_key", default_keys::START);
			fSettingsMessage->AddInt8("input_b_key", default_keys::B);
			fSettingsMessage->AddInt8("input_a_key", default_keys::A);

			fSettingsMessage->Flatten(&file);
	
			// apply settings
			MoveTo(x, y);
			fROMDirectory = path;
			fUpKey = default_keys::UP;
			fDownKey = default_keys::DOWN;
			fLeftKey = default_keys::LEFT;
			fRightKey = default_keys::RIGHT;
			fSelectKey = default_keys::SELECT;
			fStartKey = default_keys::START;
			fBKey = default_keys::B;
			fAKey = default_keys::A;
			
			// we don't need to (re)size the window, as it will be 1:1 by default			
		} else {
			// load from file
			status = fSettingsMessage->Unflatten(&file);
			
			if (status == B_OK) {
				// read settings
				int32 x;
				int32 y;
				bool doubled;
				BString path;
				int8 up, down, left, right, select, start, b, a;
				
				fSettingsMessage->FindInt32("window_x", &x);
				fSettingsMessage->FindInt32("window_y", &y);
				fSettingsMessage->FindBool("double_size", &doubled);
				fSettingsMessage->FindString("rom_dir", &path);
				
				fSettingsMessage->FindInt8("input_up_key", &up);
				fSettingsMessage->FindInt8("input_down_key", &down);
				fSettingsMessage->FindInt8("input_left_key", &left);
				fSettingsMessage->FindInt8("input_right_key", &right);
				fSettingsMessage->FindInt8("input_select_key", &select);
				fSettingsMessage->FindInt8("input_start_key", &start);
				fSettingsMessage->FindInt8("input_b_key", &b);
				fSettingsMessage->FindInt8("input_a_key", &a);
				
				// apply settings
				MoveTo(x, y);
				
				// check if we're doubled, and resize the window accordingly
				fDoubled = doubled;
				int32 const scale = static_cast<int32>(fDoubled)+1;
				ResizeTo(screen_size::WIDTH*scale, screen_size::HEIGHT*scale);

				// set rom  directory
				fROMDirectory = path;
				
				// set inputs
				fUpKey = up;
				fDownKey = down;
				fLeftKey = left;
				fRightKey = right;
				fSelectKey = select;
				fStartKey = start;
				fBKey = b;
				fAKey = a;		
			} else {
				// eli: handle error if unflatten fails?
			}	
		}
	}
}


// -----------------------------------------------------------------------------
// PretendoWindow::SaveSettings
//
// Saves window position, scale, ROM directory, and input bindings to the
// Pretendo settings file.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
PretendoWindow::SaveSettings()
{
	// assemble path
	BString path = Settings::configDirectory().c_str();
	path += "/pretendo_window";
	
	BFile file;
	status_t status;
	off_t size;

	// load settings file
	status = file.SetTo(path, B_READ_WRITE|B_CREATE_FILE);
	
	if (status == B_OK) {
		file.GetSize(&size);
		
		if (size == 0) {
			// file is empty, stash default settings
			fSettingsMessage->AddInt32("window_x", Frame().left);
			fSettingsMessage->AddInt32("window_y", Frame().top);
			fSettingsMessage->AddBool("double_size", false);
			fSettingsMessage->AddString("rom_dir", "/boot/home");
			
			fSettingsMessage->AddInt8("input_up_key", default_keys::UP);
			fSettingsMessage->AddInt8("input_down_key", default_keys::DOWN);
			fSettingsMessage->AddInt8("input_left_key", default_keys::LEFT);
			fSettingsMessage->AddInt8("input_right_key", default_keys::RIGHT);
			fSettingsMessage->AddInt8("input_select_key", default_keys::SELECT);
			fSettingsMessage->AddInt8("input_start_key", default_keys::START);
			fSettingsMessage->AddInt8("input_b_key", default_keys::B);
			fSettingsMessage->AddInt8("input_a_key", default_keys::A);	
		} else {
			// replace old settings
			fSettingsMessage->ReplaceInt32("window_x", Frame().left);
			fSettingsMessage->ReplaceInt32("window_y", Frame().top);
			fSettingsMessage->ReplaceBool("double_size", fDoubled);
			fSettingsMessage->ReplaceString("rom_dir", fROMDirectory);
			
			fSettingsMessage->ReplaceInt8("input_up_key", fUpKey);
			fSettingsMessage->ReplaceInt8("input_down_key", fDownKey);
			fSettingsMessage->ReplaceInt8("input_left_key", fLeftKey);
			fSettingsMessage->ReplaceInt8("input_right_key", fRightKey);
			fSettingsMessage->ReplaceInt8("input_select_key", fSelectKey);
			fSettingsMessage->ReplaceInt8("input_start_key", fStartKey);
			fSettingsMessage->ReplaceInt8("input_b_key", fBKey);
			fSettingsMessage->ReplaceInt8("input_a_key", fAKey);
		}		
		
		// write to file
		fSettingsMessage->Flatten(&file);
	} else {
		// eli: handle error if we can't open the file?
	}
}

static uint8 asciiToKeyCode[] = {
	/* 0x00 */	0x0, 	// B_ASCII_NUL
	/* 0x01 */	0x20,	// B_HOME	 
	/* 0x02 */	0x0, 	// B_ASCII_START_TEXT
	/* 0x03 */	0x0, 	// B_ASCII_END_TEXT
	/* 0x04 */	0xf, 	// B_END
	/* 0x05 */	0x7e, 	// B_INSERT
	/* 0x06 */	0x0, 	// B_ASCII_ACKNOWLEDGE
	/* 0x07 */	0x0, 	// B_ASCIII_BELL
	/* 0x08 */	0x0,	// B_BACKSPACE 
	/* 0x09 */	0x26,	// B_TAB
	/* 0x0a */	0x47,	// B_RETURN B_ENTER
	/* 0x0b */	0x7f,	// B_PAGE_UP
	/* 0x0c */	0x10,	// B_PAGE_DOWN	
	/* 0x0d */	0x47,	// B_ASCII_CARRIAGE_RETURN
	/* 0x0e */	0x0,	// B_ASCII_SHIFT_OUT
	/* 0x0f */	0x0,	// B_ASCII_SHIFT_IN
	
	/* 0x10 */	0x0,	// B_FUNCTION_KEY
	/* 0x11 */	0x0,	// ASCII_XON
	/* 0x12 */	0x0,	// B_DEVICE_CONTROL_2
	/* 0x13 */	0x0,	// B_ASCII_XOFF	
	/* 0x14 */	0x0,	// B_DEVICE_CONTROL_4	
	/* 0x15 */	0x0,	// B_ASCII_NEGATIVE_ACK
	/* 0x16 */	0x0,	// B_ASCII_SYNC_IDLE
	/* 0x17 */	0x0,	// B_ASCII_END_TRANSMISSION_BLOCK
	/* 0x18 */	0x0,	// B_ASCII_CANCEL
	/* 0x19 */	0x0, 	// B_ASCII_END_MEDIUM
	/* 0x1a */	0x0,	// B_SUBSTITUTE	
	/* 0x1b */	0x01, 	// ESC 		(B_ESCAPE) 
	/* 0x1c */	0x61,	// LEFT 	(B_LEFT_ARROW)	
	/* 0x1d */	0x63,	// RIGHT	(B_RIGHT_ARROW)
	/* 0x1e */	0x57,	// UP		(B_UP_ARRROW)
	/* 0x1f */	0x62,	// DOWN		(B_DOWN_ARROW)
	
	/* 0x20 */	0x5e,	// SPACE	(B_SPACE)
	/* 0x21 */	0x12,	// (1 key with shift)
	/* 0x22 */	0x46,	// (' key with shift)
	/* 0x23 */	0x14,	// # (3 key with shift)
	/* 0x24 */	0x15,	// $ (4 key with shift)
	/* 0x25 */	0x16,	// % (5 key with shift)
	/* 0x26 */	0x18,	// & (7 key with shift)
	/* 0x27 */	0x46,	// ' (single quote, same as 0x22, with shift)
	/* 0x28 */	0x1b,	// ) (0 key with shift)
	/* 0x29 */	0x1e,	// ( (9 key with shift)
	/* 0x2a */	0x19,	// * (8 key with shift)
	/* 0x2b */	0x1d,	// = (key with shift)
	/* 0x2c */	0x53,	// ,
	/* 0x2d */	0x1c,	// -
	/* 0x2e */	0x54,	// .
	/* 0x2f */	0x55,	// /
	
	/* 0x30 */	0x1b,	// 0
	/* 0x31 */ 	0x12,	// 1
	/* 0x32 */ 	0x13,	// 2
	/* 0x33 */ 	0x14,	// 3
	/* 0x34 */ 	0x15,	// 4
	/* 0x35 */ 	0x16,	// 5
	/* 0x36 */ 	0x17,	// 6
	/* 0x37 */ 	0x18,	// 7
	/* 0x38 */ 	0x19,	// 8
	/* 0x39 */ 	0x1a,	// 9
	/* 0x3a */ 	0x45,	// : (; key with shift)
	/* 0x3b */ 	0x45,	// ;
	/* 0x3c */ 	0x53,	// < (, key with shift)
	/* 0x3d */ 	0x1d,	// =
	/* 0x3e */ 	0x54,	// (. key with shift)
	/* 0x3f */ 	0x55,	// (/ key with shift)
	
	/* 0x40 */	0x13, // ! (1 key with shift)
	/* 0x41 */	0x3c, // A
	/* 0x42 */	0x50, // B
	/* 0x43 */	0x4e, // C
	/* 0x44 */	0x3e, // D
	/* 0x45 */	0x29, // E
	/* 0x46 */	0x3f, // F
	/* 0x47 */	0x40, // G
	/* 0x48 */	0x41, // H
	/* 0x49 */	0x2e, // I
	/* 0x4a */	0x42, // J
	/* 0x4b */	0x43, // K
	/* 0x4c */	0x44, // L
	/* 0x4d */	0x52, // M
	/* 0x4e */	0x51, // N
	/* 0x4f */	0x2f, // O
	
	/* 0x50 */	0x30, // P
	/* 0x51 */	0x27, // Q
	/* 0x52 */	0x2a, // R
	/* 0x53 */	0x3d, // S
	/* 0x54 */	0x2b, // T
	/* 0x55 */	0x2d, // U
	/* 0x56 */	0x4f, // V
	/* 0x57 */	0x28, // W
	/* 0x58 */	0x4d, // X
	/* 0x59 */	0x2c, // Y
	/* 0x5a */	0x4c, // Z
	/* 0x5b */	0x31, // [
	/* 0x5c */	0x33, // '\'
	/* 0x5d */	0x32, // ]
	/* 0x5e */	0x17, // ^ (6 key with shift)
	/* 0x5f */	0x1c, // _ (- key with shift)
	
	/* 0x60 */	0x11, // `
	/* 0x61 */	0x3c, // a
	/* 0x62 */	0x50, // b
	/* 0x63 */	0x4e, // c
	/* 0x64 */	0x3e, // d
	/* 0x65 */	0x29, // e
	/* 0x66 */	0x3f, // f
	/* 0x67 */	0x40, // g
	/* 0x68 */	0x41, // h
	/* 0x69 */	0x2e, // i
	/* 0x6a */	0x42, // j
	/* 0x6b */	0x43, // k
	/* 0x6c */	0x44, // l
	/* 0x6d */	0x52, // m
	/* 0x6e */	0x51, // n
	/* 0x6f */	0x2f, // o
	
	/* 0x70 */	0x30, // p
	/* 0x71 */	0x27, // q
	/* 0x72 */	0x2a, // r
	/* 0x73 */	0x3d, // s
	/* 0x74 */	0x2b, // t
	/* 0x75 */	0x2d, // u
	/* 0x76 */	0x4f, // v
	/* 0x77 */	0x28, // w
	/* 0x78 */	0x4d, // x
	/* 0x79 */	0x2c, // y
	/* 0x7a */	0x4c, // z
	/* 0x7b */	0x31, // { ([ key with shift)
	/* 0x7c */	0x33, // | (\ key with shift)
	/* 0x7d */	0x32, // } (] key with shift)
	/* 0x7e */	0x11, // ~ (` key with shift)
	/* 0x7f */	0xe	  // DEL (B_DELETE)

};

