
#include "PatternTableWindow.h"

#include "Nes.h"
#include "Cart.h"

void
convert(unsigned char *buffer, int tileCount /*, int rowCount */)
{
	int32 mask;
	unsigned char pixel;
	
	for (int32 row = 7; row > -1; row--) {
		for (int32 tile = 0; tile < tileCount; tile++) {
			mask = 0x80;
			
			for (int32 col = 0; col < 8; col++) {
				pixel = 0;
				
				if (buffer[row+tile*16+0] & mask) {
					pixel++;
				}
				
				if (buffer[row+tile*16+8] & mask) {
					pixel += 2;
                }
                
				switch (pixel) {
					case 0: //00
					break;
					
					case 1: // 01
					break;

					case 2: // 10
                    break;
                    
                    case 3: // 11
                    break;
				}
				
				mask /= 2;
			}
		}
		
		/*
		if (tileCount < 32 && rowCount > 1) {
			for (col = tileCount * 8; col < 256; col++) {
				putchar('o'); //_write(output,gray,3);
			}
		}	
		*/
    }
}


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
	
	convert(bits, 1);
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

