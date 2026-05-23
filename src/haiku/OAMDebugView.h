#ifndef OAM_DEBUG_VIEW_H_
#define OAM_DEBUG_VIEW_H_

#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <View.h>


class PretendoWindow;


class OAMDebugView : public BView
{
	public:
			OAMDebugView(BRect frame, PretendoWindow* parent);
	virtual ~OAMDebugView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* message);
	virtual void MouseDown(BPoint where);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Pulse();

	private:
	void DrawHeaderUI();
	void DrawOAMSummaryPanel();
	void DrawSpriteListPanel();
	void DrawSelectedSpritePanel();

	private:
	PretendoWindow* fParent = nullptr;

	bool fFreezeUpdates = false;
	bool fMouseInside = false;
	bool fSpriteLocked = false;

	int32 fHoverSprite = -1;
	int32 fLockedSprite = -1;
};


#endif
