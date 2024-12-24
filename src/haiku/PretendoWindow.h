

#ifndef _PRETENDO_WINDOW_H_
#define _PRETENDO_WINDOW_H_

#include <malloc.h>
#include <Application.h>
#include <DirectWindow.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Screen.h>
#include <RecentItems.h>
#include <Locker.h>
#include <OS.h>

#include "Palette.h"
#include "VideoScreen.h"
#include "ROMFilePanel.h"
#include "PaletteWindow.h"
#include "CartInfoWindow.h"
#include "SimpleMutex.h"
#include "SoundPusher.h"
#include "PretendoView.h"
#include "PatternTableWindow.h"

#include "asm/blitters.h"
#include "asm/copies.h"

#define MSG_ROM_LOADED 	'LOAD'
#define MSG_SHOW_OPEN	'OPEN'
#define MSG_LOAD_RECENT	'RCNT'
#define MSG_FREE_ROM	'FREE'
#define MSG_ABOUT		'BOUT'
#define MSG_CART_INFO	'INFO'
#define MSG_QUIT		'QUIT'
//
#define MSG_CPU_RUN		'RUN_'
#define MSG_CPU_STOP	'STOP'
#define MSG_CPU_PAUSE	'PAUS'
#define MSG_CPU_DEBUG	'DEBG'
#define MSG_RST_SOFT	'SOFT'
#define MSG_RST_HARD	'HARD'
//
#define MSG_FULLSCREEN 'FULL'

#define MSG_CHANGE_RENDER 'CHRN'
#define MSG_DRAW_BITMAP 'DRAW'
#define MSG_ADJ_PALETTE 'ADJP'

#define MSG_PTNTBL0 'PTB0'
#define MSG_PTNTBL1 'PTB1'


class PretendoWindow : public BDirectWindow
{
	private:
	enum {
		kKeyUp = 0x57,
		kKeyDown = 0x62,
		kKeyLeft = 0x61,
		kKeyRight = 0x63,
		kKeySelect = 0x3c,
		kKeyStart = 0x3d,
		kKeyB = 0x4c,
		kKeyA = 0x4d
	};
	
	public:
	enum {
		SCREEN_WIDTH = 256,
		SCREEN_HEIGHT = 240
	};
	
	typedef enum {
		NO_FRAMEWORK = 0,
		BITMAP_FRAMEWORK = 1,
		OVERLAY_FRAMEWORK = 2,
		DIRECTWINDOW_FRAMEWORK = 3,
		WINDOWSCREEN_FRAMEWORK = 4
	} VIDEO_FRAMEWORK;
	
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
	void AddMenu (void);
	
	// handlers
	private:
	void OnLoadCart (BMessage *message);
	void OnFreeCart();
	void OnCartInfo();
	void OnQuit();
	void OnRun();
	void OnStop();
	void OnPause();
	void OnDebug();
	void OnSoftReset();
	void OnHardReset();
	

	// video stuff
	private:
	void (PretendoWindow::*LineRenderer)(uint8 *dest, const uint32_t *source);
	void RenderLine8 (uint8 *dest, const uint32_t *source);
	void RenderLine16 (uint8 *dest, const uint32_t *source);
	void RenderLine32 (uint8 *dest, const uint32_t *source);
	void ClearBitmap (bool overlay);
	void SetRenderer (color_space cs);
	void SetFrontBuffer (uint8 *bits, color_space cs, int32 pixel_width, int32 rowbytes);
	void ChangeFramework (VIDEO_FRAMEWORK fw);
	void BlitScreen();
	void ClearDirty();
	void DrawDirect();
	
	// video interface
	public:
	void submit_scanline(int scanline, const uint32_t *source);
	void set_palette(const color_emphasis_t *intensity, const rgb_color_t *pal);
	void start_frame();
	void end_frame();
	
	private:
	void SetDefaultPalette();
	
	private:
	//PretendoView *fView;
	BView *fView = nullptr;
	BMenuBar *fMenu = nullptr;
	BMenu *fFileMenu = nullptr;
	BMenu *fLoadMenu = nullptr;
	BMenu *fEmuMenu = nullptr;
	BMenu *fVideoMenu = nullptr;
	BMenu *RenderMenu = nullptr;
	ROMFilePanel *fOpenPanel = nullptr;
	int32 fMenuHeight = 18;
	
	private:
	CartInfoWindow *fCartInfoWindow = nullptr;
	
	private:
	uint8 *fLineOffsets[SCREEN_HEIGHT];
	int32 fPixelWidth;
	uint8 fPalette8[8][64];
	uint16 fPalette16[8][64];
	uint32 fPalette32[8][64];
	uint32 fPaletteY[65536];
	uint32 fPaletteYCbCr[65536];
	uint8 *fMappedPalette[8];	
	
	private:
	VIDEO_FRAMEWORK fFramework = NO_FRAMEWORK;
	VIDEO_FRAMEWORK fPrevFramework = NO_FRAMEWORK;
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
	
	private:
	SoundPusher *fSoundPusher = nullptr;
	
	private:
	PaletteWindow *fPaletteWindow = nullptr;
	PatternTableWindow *fPatternTable0Window = nullptr;
	PatternTableWindow *fPatternTable1Window = nullptr;
	
	private:
	bool fPaused = false;
	
	private:
	thread_id fThread = B_BAD_THREAD_ID;
	static status_t emulation_thread (void *data);
	bool fRunning = false;
	
	private:
	bool Running() { return fRunning; }

	private:
	key_info fKeyStates;
	inline void CheckKey (int32 index, int32 key);
	inline void ReadKeyStates();
	
	private:
	SimpleMutex *fMutex = nullptr;
	SimpleMutex *Mutex() { return fMutex; }
	
	private:
	void ShowFPS();
	
	private:
	uint64 fClockSpeed = 0;
	bool fShowFPS = false;
};
				
#endif // _PRETENDO_WINDOW_H_

