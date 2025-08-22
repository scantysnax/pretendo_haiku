
#ifndef _INPUT_VIEW_H_
#define _INPUT_VIEW_H_

#include <View.h>
#include <Bitmap.h>


class InputView : public BView
{
	public:
			InputView(BRect frame);
	virtual ~InputView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	private:
	BBitmap *fControllerBitmap = nullptr;
	BView *fControllerView = nullptr;
};

#endif //_INPUT_VIEW_H_
