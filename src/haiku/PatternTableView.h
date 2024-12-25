
#ifndef _PATTERN_TABLE_VIEW_H_
#define _PATTERN_TABLE_VIEW_H_

#include <View.h>

class PatternTableView : public BView
{
	public:
	PatternTableView (BRect frame);
	virtual ~PatternTableView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MessageReceived (BMessage *message);
		
	//private:
	//BMessage *fMsgChangePalette;
	
	//private:
	//PretendoWindow *fParent;
};


#endif //_PATTERN_TABLE_VIEW_H_

