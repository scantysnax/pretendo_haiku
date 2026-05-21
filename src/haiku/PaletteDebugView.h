#ifndef _PALETTE_DEBUG_VIEW_H_
#define _PALETTE_DEBUG_VIEW_H_


#include <View.h>


class PaletteDebugView : public BView
{	
	public:
			PaletteDebugView (BRect frame);
	virtual ~PaletteDebugView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:

};


#endif // _PALETTE_DEBUG_VIEW_H_
