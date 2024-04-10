
#ifndef _LINKVIEW_H_
#define _LINKVIEW_H_

#include <StringView.h>
#include <Cursor.h>
#include <Roster.h>
#include <Alert.h>

class LinkView : public BStringView
{
	public:
	LinkView (BRect frame, const char *text, const char *link);
	~LinkView();
	
	public:
	virtual void AttachedToWindow (void);
	virtual void Draw (BRect updateRect);
	virtual void MouseUp (BPoint point);
	virtual void MouseDown (BPoint point);
	virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *message);
	
	private:
	const char *fText;
	const char *fLink;
	BCursor *fLinkCursor;
};

#endif // _LINKVIEW_H_
