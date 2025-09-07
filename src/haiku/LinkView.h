
#ifndef _LINKVIEW_H_
#define _LINKVIEW_H_

#include <StringView.h>
#include <Cursor.h>
#include <Roster.h>
#include <Application.h>


class LinkView : public BStringView
{
	public:
	LinkView (BRect frame, char const  *text, char const *link);
	~LinkView();
	
	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void MouseUp (BPoint point);
	virtual void MouseDown (BPoint point);
	virtual void MouseMoved (BPoint point, uint32 transit, const BMessage *message);
	
	private:
	char const *fText = nullptr;
	char const *fLink = nullptr;
	BCursor *fLinkCursor = nullptr;
};

#endif // _LINKVIEW_H_
