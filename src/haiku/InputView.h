
#ifndef _INPUT_VIEW_H_
#define _INPUT_VIEW_H_

#include <Bitmap.h>
#include <Button.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <View.h>

#include "Splitters.h"

#include <unordered_map>
#include <unordered_set>


class KeyTextView;


class InputView : public BView
{
	private:
	typedef enum {
		CANCEL =	'CNCL',
		DEFAULT = 	'DFLT',
		SAVE = 		'SAVE'
	} messages;
	
	public:
	typedef enum {
		WIDTH = 539,
		HEIGHT = 291,
		BORDER = 16
	} controller_size;
	
	private:
	typedef enum {
		UP = 0x57,
		DOWN = 0x62,
		LEFT = 0x61,
		RIGHT = 0x63,
		SELECT = 0x3c,
		START = 0x3d,
		B = 0x4c,
		A = 0x4d
	} default_keys;
 
	
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
	std::pmr::unordered_set<uint8> FindDuplicates (uint8 *list, size_t size);
	
	private:
	BBitmap *fControllerBitmap = nullptr;
	
	private:
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
	void ValidateKeys();
	
	private:
	uint8 fUpKey = 0;
	uint8 fDownKey = 0;
	uint8 fLeftKey = 0;
	uint8 fRightKey = 0;
	uint8 fSelectKey = 0;
	uint8 fStartKey = 0;
	uint8 fBKey = 0;
	uint8 fAKey = 0;
	
	public:
	uint8 UpKey() {
		return fUpKey;
	}
	
	uint8 DownKey() {
		return fDownKey;
	}
	
	uint8 LeftKey() {
		return fLeftKey;
	}
	
	uint8 RightKey() {
		return fRightKey;
	}
	
	uint8 SelectKey() {
		return fSelectKey;
	}
	
	uint8 StartKey() {
		return fStartKey;
	}
	
	uint8 BKey() {
		return fBKey;
	}
	
	uint8 AKey() {
		return fAKey;
	}
		
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
