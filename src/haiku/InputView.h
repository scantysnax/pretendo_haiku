
#ifndef _INPUT_VIEW_H_
#define _INPUT_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TextView.h>


constexpr int32 kControllerWidth = 539;
constexpr int32 kControllerHeight = 291;
constexpr int32 kBorderWidth = 16;
//constexpr int32 kControllerWidth = 673;
//constexpr int32 kControllerHeight = 364;

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
	BTextView *fUpTextView = nullptr;
};

#endif //_INPUT_VIEW_H_
