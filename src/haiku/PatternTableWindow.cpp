
#include <iostream>

#include "PatternTableWindow.h"

#include "Nes.h"
#include "Cart.h"




PatternTableWindow::PatternTableWindow (PretendoWindow *parent,uint32 address)
	: BWindow(BRect(0, 0, 0, 0), "0", B_FLOATING_WINDOW_LOOK, 
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
	CenterOnScreen();
	
	BView *backView = new BView(Bounds(), "_back_view", B_FOLLOW_ALL, 0);
	backView->SetViewColor(0xff, 0xff, 0xff);
	AddChild (backView);
	
	fBitmap = new BBitmap (Bounds(), B_CMAP8);
	backView->SetViewBitmap(fBitmap);
	
	uint8 *bits = (uint8 *)fBitmap->Bits();
	memset (bits, 0, fBitmap->BitsLength());
	
	*(uint32 *)bits = 		0xffffffff;
	*(uint32 *)(bits+4) = 	0xffffffff;
	
	std::cout << "bytes per row: " << fBitmap->BytesPerRow() << std::endl;
	
	//*(uint32 *)(bits+fBitmap->BytesPerRow()) = 0x;
}


PatternTableWindow::~PatternTableWindow()
{
	// 	((Bitplane0_ROW[p] >> N) & 1) | (((Bitplane1_ROW[p] >> N) & 1) << 1)

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
