

#ifndef _PRETENDO_WINDOW_H_
#define _PRETENDO_WINDOW_H_

#include <Application.h>
#include <DirectWindow.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Screen.h>
#include <RecentItems.h>

#include <malloc.h>

#include "Palette.h"
#include "VideoScreen.h"
#include "ROMFilePanel.h"
#include "PaletteWindow.h"
#include "CartInfoWindow.h"
#include "Mutex.h"
#include "AudioStream.h"
#include "PretendoView.h"
#include "PatternTableWindow.h"
#include "NameTableWindow.h"
#include "PretendoView.h"
#include "InputWindow.h"

#include "asm/blitters.h"
#include "asm/copies.h"

// messages
constexpr uint32 MSG_ROM_LOADED =	'LOAD';
constexpr uint32 MSG_SHOW_OPEN =	'OPEN';
constexpr uint32 MSG_LOAD_RECENT =	'RCNT';
constexpr uint32 MSG_FREE_ROM =		'FREE';
constexpr uint32 MSG_ABOUT =		'BOUT';
constexpr uint32 MSG_CART_INFO =	'INFO';
constexpr uint32 MSG_QUIT =			'QUIT';
// emulator
constexpr uint32 MSG_CPU_RUN =		'RUN ';
constexpr uint32 MSG_CPU_STOP =		'STOP';
constexpr uint32 MSG_CPU_PAUSE =	'PAUS';
constexpr uint32 MSG_CPU_DEBUG =	'DEBG';
constexpr uint32 MSG_RST_SOFT =		'SOFT';
constexpr uint32 MSG_RST_HARD = 	'HARD';
// video
constexpr uint32 MSG_FULLSCREEN =		'FULL';
constexpr uint32 MSG_CHANGE_RENDER = 	'CHRN';
constexpr uint32 MSG_DRAW_BITMAP =		'DRAW';
constexpr uint32 MSG_ADJ_PALETTE =		'ADJP';
// input
constexpr uint32 MSG_CFG_INPUT = 'INPT';
// tools
constexpr uint32 MSG_PTNTBL0 = 	'PTB0';
constexpr uint32 MSG_PTNTBL1 = 	'PTB1';
constexpr uint32 MSG_NTBL0 = 	'NTB0';
constexpr uint32 MSG_NTBL1 = 	'NTB1';
constexpr uint32 MSG_NTBL2 = 	'NTB2';
constexpr uint32 MSG_NTBL3 = 	'NTB3';


// we need to forward declare this
class PretendoView;

class PretendoWindow : public BDirectWindow
{
	private:
	enum {
		kDefaultKeyUp = 0x57,
		kDefaultKeyDown = 0x62,
		kDefaultKeyLeft = 0x61,
		kDefaultKeyRight = 0x63,
		kDefaultKeySelect = 0x3c,
		kDefaultKeyStart = 0x3d,
		kDefaultKeyB = 0x4c,
		kDefaultKeyA = 0x4d
	};
	
	public:
	enum {
		SCREEN_WIDTH = 256,
		SCREEN_HEIGHT = 240
	};
	
	typedef enum {
		VF_NONE = 0,
		VF_BITMAP = 1,
		VF_OVERLAY = 2,
		VF_DIRECT = 3,
		VF_FULLSCREEN = 4
	} video_framework;
	
	typedef struct {
		uint8 *bits;
		color_space pixel_format;
		int32 pixel_width;	// in bytes
		int32 row_bytes;
	} video_buffer_t;
	
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
	
	private:
	void AddMenu();
	
	// handlers for ui
	private:
	void OnLoadCart (BMessage *message);
	void OnFreeCart();
	void OnCartInfo();
	void OnQuit();
	void OnRun();
	void OnStop();
	void OnPause();
	void OnSoftReset();
	void OnHardReset();
	void OnConfigureInput();
	void OnAdjustPalette();
	void OnViewPatternTable0();
	void OnViewPatternTable1();
	void OnViewNameTable0();
	void OnViewNameTable1();
	void OnViewNameTable2();
	void OnViewNameTable3();
	

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
	void submit_scanline(int scanline, const uint32_t *source);
	void set_palette(const color_emphasis_t *intensity, const rgb_color_t *pal);
	void start_frame();
	void end_frame();
	
	private:
	void SetDefaultPalette();
	
	// menus
	private:
	PretendoView *fView = nullptr;
	BMenuBar *fMenu = nullptr;
	BMenu *fFileMenu = nullptr;
	BMenu *fLoadMenu = nullptr;
	BMenu *fEmuMenu = nullptr;
	BMenu *fVideoMenu = nullptr;
	BMenu *fToolMenu = nullptr;
	BMenu *fPatternTableMenu = nullptr;
	BMenu *fNameTableMenu = nullptr;
	int32 fMenuHeight;
	
	// open panel
	private:
	ROMFilePanel *fOpenPanel = nullptr;
	
	// palettes	
	private:
	uint8 *fLineOffsets[SCREEN_HEIGHT];
	int32 fPixelWidth;
	uint8 fPalette8[8][64];
	uint16 fPalette16[8][64];
	uint32 fPalette32[8][64];
	uint32 fPaletteY[65536];
	uint32 fPaletteYCbCr[65536];
	uint8 *fMappedPalette[8];
		
	// video	
	private:
	video_framework fFramework = VF_NONE;
	video_framework fPrevFramework = VF_NONE;
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
	CartInfoWindow *fCartInfoWindow = nullptr;
	PaletteWindow *fPaletteWindow = nullptr;
	PatternTableWindow *fPatternTable0Window = nullptr;
	PatternTableWindow *fPatternTable1Window = nullptr;
	NameTableWindow *fNameTable0Window = nullptr;
	NameTableWindow *fNameTable1Window = nullptr;
	NameTableWindow *fNameTable2Window = nullptr;
	NameTableWindow *fNameTable3Window = nullptr;
	InputWindow *fInputWindow = nullptr;
	
	private:
	bool fPaused = false;
	
	// thread stuff
	private:
	thread_id fThread = B_BAD_THREAD_ID;
	static status_t emulator_thread (void *data);
	bool fRunning = false;
	
	public:
	bool Running() const { return fRunning; }

	// input
	private:
	key_info fKeyStates;
	inline void CheckKey (int32 index, int32 key) const;
	inline void ReadKeyStates();
	
	// mutex
	private:
	Mutex const *fMutex = nullptr;
	bool LockMutex() const	 { return fMutex->Lock();	}
	bool UnlockMutex() const { return fMutex->Unlock();	}
};
				
#endif // _PRETENDO_WINDOW_H_

