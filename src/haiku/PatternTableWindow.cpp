
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
			*(uint8 *)(bits+y*row_bytes+x) = 0x00;	
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

/*
const uint8_t p0     = sprite.patterns[0];
const uint8_t p1     = sprite.patterns[1];
const uint16_t shift = 7 - x_offset;
const uint8_t sprite_pixel =
	((p0 >> shift) & 0x01) | (((p1 >> shift) << 0x01) & 0x02);
*/

/*
void 
convert(char* buffer, int tileCount, int rowCount)
{
	int tile;
 	int row;
  	int col;
  	int bitMask;
  	char pixelOut;
  	char black[3],blue[3],red[3],white[3],gray[3];

  	// Init. colors
  	for(row=0;row<3;row++) {
    	black[row]=0;
    	white[row]=0xFF;
    	blue[row]=0;
    	red[row]=0;
    	gray[row]=0x80;
    }
  	
  	blue[0]=0xFF;
  	red[2]=0xFF;

  for(row=7;row>-1;row--) {
    for(tile=0; tile < tileCount; tile++) {
      bitMask=128;
      for(col=0;col<8;col++) {
        pixelOut=0;
        if( (int) buffer[row + tile * 16] & bitMask)
	  pixelOut++;
        if( (int) buffer[row + tile * 16 + 8] & bitMask)
	  pixelOut+=2;
        switch(pixelOut){
	  case 0:	//00
	  	break;
	  
	  case 1:	// 01
	  	break;
	  	
	  case 2:	// 10
	  	break;
	  	
	  case 3:	// 11
	  	break;
      	  }
        bitMask /= 2;
        }
      }
      if(tileCount < 32 && rowCount > 1)
		for(col=tileCount * 8; col < 256; col++)
	  		putchar('o');//_write(output,gray,3);
	}	
}
*/
