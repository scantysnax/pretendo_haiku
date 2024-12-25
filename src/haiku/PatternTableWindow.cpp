
#include "PatternTableWindow.h"

#include "Nes.h"
#include "Cart.h"


PatternTableWindow::PatternTableWindow (PretendoWindow *parent,uint32 address)
	: BWindow(BRect(200, 200, 0, 0), "0", B_FLOATING_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fParent(parent),
	fAddress(address)
{
	if (address == 0x0) {
		SetTitle ("Pattern Table 0");
	} else if (address == 0x1000) {
		SetTitle("Pattern Table 1");
	}
	
	
	ResizeTo(256, 256);
	//CenterOnScreen();
	
	BView *backView = new BView(Bounds(), "_back_view", B_FOLLOW_ALL, 0);
	AddChild (backView);
	
	fBitmap = new BBitmap (Bounds(), B_CMAP8);
	backView->SetViewBitmap(fBitmap);
	
	uint8 *bits = (uint8 *)fBitmap->Bits();
	memset (bits, 0, fBitmap->BitsLength());
	
	int32 height = 256;
	int32 width = 256;
	int32 row_bytes = fBitmap->BytesPerRow();
	
	
	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			*(uint8 *)(bits+y*row_bytes+x) = 0x0f;	
	}	}
}

PatternTableWindow::~PatternTableWindow()
{
}


void
PatternTableWindow::MessageReceived (BMessage *message)
{	
	BWindow::MessageReceived(message);
}


bool
PatternTableWindow::QuitRequested()
{
	Hide();
	return false;
}


