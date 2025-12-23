#ifndef _PRETENDO_WINDOW_H_
#define _PRETENDO_WINDOW_H_

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <DirectWindow.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <RecentItems.h>
#include <Screen.h>

#include <malloc.h>

#include "AudioStream.h"
#include "InputWindow.h"
#include "Mutex.h"
#include "NameTableWindow.h"
#include "Palette.h"
#include "PaletteWindow.h"
#include "PatternTableWindow.h"
#include "PretendoView.h" 
#include "ROMFilePanel.h"
#include "ROMInfoWindow.h"
#include "VideoScreen.h"

#include "asm/blitters.h"
#include "asm/copies.h"

class PretendoView;

class PretendoWindow : public BDirectWindow
{
	public:
	typedef enum {
		// file
		ROM_LOADED = 	'LOAD',
		SHOW_OPEN = 	'OPEN',
		LOAD_RECENT = 	'RCNT',
		FREE_ROM = 		'FREE',
		SHOW_ABOUT = 	'ABOU',
		ROM_INFO = 		'INFO',
		QUIT = 			'QUIT',
		// emulator
		CPU_RUN = 		'RUN_',
		CPU_STOP = 		'STOP',
		CPU_PAUSE =		'PAUS',
		CPU_DEBUG = 	'DBUG',
		RST_SOFT = 		'SOFT',
		RST_HARD = 		'HARD',
		// video
		CHANGE_RENDER = 	'CHRN',
		DRAW_BITMAP = 		'DRAW',
		ENTER_FULLSCREEN = 	'ENFS', 
		LEAVE_FULLSCREEN = 	'LVFS',
		// input
		CFG_INPUT = 'CFGI',
		// rom directory
		SET_ROMDIR = 	'ROMS',
		RECV_ROM_DIR = 	'RECV',
		// sound channel enable/disable
		ENABLE_SQ1 = 	'SQR1',
		ENABLE_SQ2 = 	'SQR2',
		ENABLE_TRI = 	'TRIA',
		ENABLE_NOISE = 	'NOIS',
		ENABLE_DMC = 	'DPCM',
		// tools
		ADJ_PALETTE =	'ADJP',
		SHOW_PTNTBL1 = 	'PTB1',
		SHOW_PTNTBL2 = 	'PTB2',
		SHOW_NTBL1 = 	'NTB1',
		SHOW_NTBL2 = 	'NTB2',
		SHOW_NTBL3 = 	'NTB3',
		SHOW_NTBL4 = 	'NTB4'
	} messages;	
	
	private:
	typedef enum {
		UP = 0x57,
		DOWN = 0x62,
		LEFT = 0x61,
		RIGHT = 0x63,
		SELECT = 0x3c,
		START = 0x3d,
		B = 0x4c,
		A = 0x4d
	} default_keys;
	
	private:
	typedef enum {
		WIDTH = 256,
		HEIGHT = 240
	} screen_size;
	
	private:
	typedef enum {
		NONE = 0,
		BITMAP = 1,
		OVERLAY = 2,
		DIRECT = 3,
		FULLSCREEN = 4
	} video_framework;
	
	
	private:
	typedef struct {
		uint8 *bits;
		color_space pixel_format;
		int32 pixel_width;	// in bytes
		int32 row_bytes;
	} video_buffer_t;
	
	private:
	typedef struct {
		clipping_rect bounds;
		int32 clip_count;
		clipping_rect *clip_list;
	} clipping_info_t;

	
	public:
			PretendoWindow();
	virtual ~PretendoWindow();
	
	
	// inherited from B(Direct)Window
	public:
	virtual void DirectConnected (direct_buffer_info *info);
	virtual void MessageReceived (BMessage *message);
	virtual bool QuitRequested();
	virtual void ResizeTo (float width, float height);
	virtual void Zoom (BPoint origin, float width, float height);
	virtual void WindowActivated (bool flag);
	virtual void MenusBeginning();
	virtual void MenusEnded();
	
	// menu
	private:
	void AddMenu();
	
	// handlers for ui
	private:
	void OnLoadROM (BMessage *message);
	void OnFreeROM();
	void OnROMInfo();
	void OnQuit();
	void OnRun();
	void OnStop();
	void OnPause();
	void OnSoftReset();
	void OnHardReset();
	void OnConfigureInput();
	void OnSetRomDirectory();
	void OnAdjustPalette();
	void OnViewPatternTable1();
	void OnViewPatternTable2();
	void OnViewNameTable1(); 
	void OnViewNameTable2();
	void OnViewNameTable3();
	void OnViewNameTable4();
	void OnAudioSquare1();
	void OnAudioSquare2();
	void OnAudioTriangle();
	void OnAudioNoise();
	void OnAudioDMC();

	// video stuff
	private:
	void (PretendoWindow::*LineRenderer)(uint8 *dest, const uint32_t *source);
	void RenderLine8 (uint8 *dest, const uint32_t *source);
	void RenderLine16 (uint8 *dest, const uint32_t *source);
	void RenderLine32 (uint8 *dest, const uint32_t *source);
	void ClearBitmap (bool overlay);
	void SetRenderer (color_space cs);
	void SetFrontBuffer (uint8 *bits, color_space cs, int32 pixel_width, int32 rowbytes);
	void ChangeFramework (video_framework fw);
	void BlitScreen();
	void ClearDirty();
	void DrawDirect();
	void DrawBitmap();
	void DrawOverlay();
	void DrawFullScreen();
	
	// video interface
	public:
	void submit_scanline (int scanline, const uint32_t *source);
	void set_palette (const color_emphasis_t *intensity, const rgb_color_t *pal);
	void start_frame();
	void end_frame();
	
	// settings
	private:
	void LoadSettings();
	void SaveSettings();
	BMessage *fSettingsMessage = nullptr;
	
	// menus
	private:
	PretendoView *fView = nullptr;
	BMenuBar *fMenu = nullptr;
	BMenu *fFileMenu = nullptr;
	BMenu *fEmuMenu = nullptr;
	BMenu *fSettingsMenu = nullptr;
	BMenu *fVideoMenu = nullptr;
	BMenu *fAudioMenu = nullptr;
	BMenu *fToolMenu = nullptr;
	BMenu *fPatternTableMenu = nullptr;
	BMenu *fNameTableMenu = nullptr;
	int32 fMenuHeight;
	
	// panels
	private:
	ROMFilePanel *fOpenPanel = nullptr;
	BFilePanel *fROMDirectoryPanel = nullptr;
	
	// palettes	
	private:
	uint8 *fLineOffsets[screen_size::HEIGHT];
	int32 fPixelWidth;
	uint8 fPalette8[8][64];
	uint16 fPalette16[8][64];
	uint32 fPalette32[8][64];
	uint32 fPaletteY[65536];
	uint32 fPaletteYCbCr[65536];
	uint8 *fMappedPalette[8];
		
	// video	
	private:
	video_framework fFramework = video_framework::NONE;
	video_framework fPrevFramework = video_framework::NONE;
	BBitmap *fBitmap = nullptr;
	BBitmap *fOverlayBitmap = nullptr;
	uint8 *fBitmapBits = nullptr;
	uint8 *fOverlayBits = nullptr;
	area_id fBitsArea = B_ERROR;
	area_id fDirtyArea = B_ERROR;
	video_buffer_t fBackBuffer;
	video_buffer_t fFrontBuffer;
	video_buffer_t fDirtyBuffer;
	clipping_info_t fClipInfo;
	VideoScreen *fVideoScreen = nullptr;
	bool fFullScreen = false;
	volatile bool fDirectConnected = false;
	bool fFrameworkChanging = false;
	bool fDoubled = false;
	int32 fClear = 0;
	
	// sound
	private:
	AudioStream *fAudioStream = nullptr;
	
	// children
	private:
	ROMInfoWindow *fROMInfoWindow = nullptr;
	PaletteWindow *fPaletteWindow = nullptr;
	PatternTableWindow *fPatternTable1Window = nullptr;
	PatternTableWindow *fPatternTable2Window = nullptr;
	NameTableWindow *fNameTable1Window = nullptr;
	NameTableWindow *fNameTable2Window = nullptr;
	NameTableWindow *fNameTable3Window = nullptr;
	NameTableWindow *fNameTable4Window = nullptr;
	InputWindow *fInputWindow = nullptr;
	
	private:
	bool fPaused = false;
	
	private:
	BString fROMDirectory = nullptr;
		
	// thread stuff
	private:
	thread_id fThread = B_BAD_THREAD_ID;
	static status_t emulator_thread (void *data);
	bool fRunning = false;
	
	public:
	bool Running() const { 
		return fRunning;
	}

	// input
	private:
	key_info fKeyStates;
	inline void CheckKey (int32 index, int32 key) const;
	inline void ReadKeyStates();
	
	// mutex
	private:
	Mutex const *fMutex = nullptr;
	
	public:
	bool LockMutex() const { 
		return fMutex->Lock();
	}
	
	bool UnlockMutex() const { 
		return fMutex->Unlock();
	}
};
				
#endif // _PRETENDO_WINDOW_H_


