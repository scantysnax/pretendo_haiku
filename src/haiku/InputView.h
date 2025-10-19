
#ifndef _INPUT_VIEW_H_
#define _INPUT_VIEW_H_

#include <Bitmap.h>
#include <Button.h>
#include <TextView.h>
#include <TranslationUtils.h>


#include "Splitters.h"


class KeyTextView;


class InputView : public BView
{
	private:
	typedef enum {
		CANCEL = 	'CNCL',
		DEFAULT = 	'DFLT',
		SAVE = 		'SAVE'
	} messages;
	
	public:
	typedef enum {
		WIDTH = 539,
		HEIGHT = 291,
		BORDER = 16
	} controller_size;
	
	public:
			InputView (BRect frame);
	virtual ~InputView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
	
	public:
	void SetDefaultKeys();
	
	private:
	void OnCancel();
	void OnDefault();
	void OnSave();
	
	private:
	BBitmap *fControllerBitmap = nullptr;
	
	KeyTextView *fUpView = nullptr;
	KeyTextView *fDownView = nullptr;
	KeyTextView *fLeftView = nullptr;
	KeyTextView *fRightView = nullptr;
	KeyTextView *fSelectView = nullptr;
	KeyTextView *fStartView = nullptr;
	KeyTextView *fBView = nullptr;
	KeyTextView *fAView = nullptr;
	
	private:
	BButton *fCancelButton = nullptr;
	BButton *fDefaultButton = nullptr;
	BButton *fSaveButton = nullptr;	
	
	private:
	void CheckForDuplicates();
};


class KeyTextView : public BTextView
{
	public:
			KeyTextView (BRect frame);
	virtual ~KeyTextView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	
	private:	
};


#endif //_INPUT_VIEW_H_
