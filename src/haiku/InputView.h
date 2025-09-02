
#ifndef _INPUT_VIEW_H_
#define _INPUT_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TextView.h>


constexpr int32 kControllerWidth = 539;
constexpr int32 kControllerHeight = 291;
constexpr int32 kControllerBorder = 16;


class ButtonTextView;


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
	
	ButtonTextView *fUpView = nullptr;
	ButtonTextView *fDownView = nullptr;
	ButtonTextView *fLeftView = nullptr;
	ButtonTextView *fRightView = nullptr;
	ButtonTextView *fSelectView = nullptr;
	ButtonTextView *fStartView = nullptr;
	ButtonTextView *fBView = nullptr;
	ButtonTextView *fAView = nullptr;
	
};


class ButtonTextView : public BTextView
{
	public:
			ButtonTextView (BRect frame);
	virtual ~ButtonTextView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	
	private:
	
};

#endif //_INPUT_VIEW_H_
