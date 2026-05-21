#ifndef _PALETTE_INFO_VIEW_H_
#define _PALETTE_INFO_VIEW_H_

#include <View.h>


class PaletteInfoView : public BView
{	
	public:
			PaletteInfoView (BRect frame);
	virtual ~PaletteInfoView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:

};


#endif // _PALETTE_INFO_VIEW_H_
