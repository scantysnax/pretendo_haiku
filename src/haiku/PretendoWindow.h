

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
	virtual bool QuitRequested (void);
	virtual void ResizeTo (float width, float height);
	virtual void Zoom (BPoint origin, float width, float height);
	virtual void WindowActivated (bool flag);
	virtual void MenusBeginning (void);
	virtual void MenusEnded (void);
	
	private:
	void AddMenu (void);
	
	// handlers
	private:
	void OnLoadCart (BMessage *message);
	void OnFreeCart (void);
	void OnCartInfo (void);
	void OnQuit (void);
	void OnRun (void);
	void OnStop (void);
	void OnPause (void);
	void OnDebug (void);
	void OnSoftReset (void);
	void OnHardReset (void);
	

	// video stuff
	private:
	void (PretendoWindow::*LineRenderer)(uint8 *dest, const uint32_t *source /*, int intensity*/);
	void RenderLine8 (uint8 *dest, const uint32_t *source /*, int intensity*/);
	void RenderLine16 (uint8 *dest, const uint32_t *source /*, int intensity*/);
	void RenderLine32 (uint8 *dest, const uint32_t *source /*, int intensity*/);
	void ClearBitmap (bool overlay);
	void SetRenderer (color_space cs);
	void SetFrontBuffer (uint8 *bits, color_space cs, int32 pixel_width, int32 rowbytes);
	void ChangeFramework (VIDEO_FRAMEWORK fw);
	void BlitScreen (void);
	void ClearDirty (void);
	void DrawDirect (void);
	
	// video interface
	public:
	void submit_scanline(int scanline, /*int intensity,*/ const uint32_t *source);
	void set_palette(const color_emphasis_t *intensity, const rgb_color_t *pal);
	void start_frame();
	void end_frame();
	
	private:
	void SetDefaultPalette (void);
	
	private:
	//PretendoView *fView;
	BView *fView;
	BMenuBar *fMenu;
	BMenu *fFileMenu;
	BMenu *fLoadMenu;
	BMenu *fEmuMenu;
	BMenu *fVideoMenu;
	BMenu *RenderMenu;
	ROMFilePanel *fOpenPanel;
	int32 fMenuHeight;
	
	private:
	CartInfoWindow *fCartInfoWindow;
	
	private:
	uint8 *fLineOffsets[SCREEN_HEIGHT];
	int32 fPixelWidth;
	uint8 fPalette8[8][64];
	uint16 fPalette15[8][64];
	uint16 fPalette16[8][64];
	uint32 fPalette32[8][64];
	uint32 fPaletteY[65536];
	uint32 fPaletteYCbCr[65536];
	uint8 *fMappedPalette[8];	
	
	private:
	VIDEO_FRAMEWORK fFramework;
	VIDEO_FRAMEWORK fPrevFramework;
	BBitmap *fBitmap;
	BBitmap *fOverlayBitmap;
	uint8 *fBitmapBits;
	uint8 *fOverlayBits;
	area_id fBitsArea;
	area_id fDirtyArea;
	video_buffer_t fBackBuffer;
	video_buffer_t fFrontBuffer;
	video_buffer_t fDirtyBuffer;
	clipping_info_t fClipInfo;
	VideoScreen *fVideoScreen;
	bool fFullScreen;
	volatile bool fDirectConnected;
	bool fFrameworkChanging;
	bool fDoubled;
	int32 fClear;
	
	private:
	SoundPusher *fSoundPusher;
	
	private:
	PaletteWindow *fPaletteWindow;
	
	private:
	bool fPaused;
	
	private:
	thread_id fThread;
	static status_t emulation_thread (void *data);
	bool fRunning;
	
	private:
	bool Running (void) { return fRunning; }

	private:
	key_info fKeyStates;
	inline void CheckKey (int32 index, int32 key);
	inline void ReadKeyStates (void);
	
	private:
	SimpleMutex *fMutex;
	SimpleMutex *Mutex (void) { return fMutex; }
	
	private:
	uint64 ReadTSC (void);
	void ShowFPS (void);
	
	private:
	uint64 fClockSpeed;
	bool fShowFPS;
};
				
#endif // _PRETENDO_WINDOW_H_

