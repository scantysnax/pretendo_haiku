#ifndef OAM_DEBUG_VIEW_H_
#define OAM_DEBUG_VIEW_H_

#include <View.h>
#include <ScrollBar.h>
#include <Screen.h>

#include "Cart.h"
#include "Mapper.h"
#include "Ppu.h"


class PretendoWindow;
class PatternTableWindow;

class OAMDebugView : public BView
{
	public:
			OAMDebugView(BRect frame, PretendoWindow* parent);
	virtual ~OAMDebugView() ;

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void MessageReceived(BMessage* message);
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage *message);
	virtual void Pulse();
	
	public:
	void SetFirstSpriteFromScrollBar(int32 firstSprite);
	void SetHostPalette (uint8 *palette);
	void SetPatternTables(PatternTableWindow* pt0, PatternTableWindow* pt1);
	
	private:
	void DrawHeaderUI();
	void DrawOAMSummaryPanel();
	void DrawSpriteListPanel();
	void DrawSelectedSpritePanel();
	void DrawSpritePreview (BRect previewRect, int32 spriteIndex);
	rgb_color SpritePreviewColor (uint8 spritePalette, uint8 pixel) const;
	
	private:
	void UpdatePaletteDebuggerHighlight();
	void UpdatePatternTableHighlight();
	void ClearPatternTableHighlight();
	
	private:
	PretendoWindow *fParent = nullptr;
	
	private:
	BScrollBar *fSpriteScrollBar = nullptr;

	private:
	bool fFreezeUpdates = false;
	bool fMouseInside = false;
	bool fSpriteLocked = false;
	
	private:
	int32 fHoverSprite = -1;
	int32 fLockedSprite = -1;
	int32 fFirstSprite = 0;
	
	private:
	uint8 *fHostPalette = nullptr;
	
	 private:
	 PatternTableWindow *fPatternTable0 = nullptr;
	 PatternTableWindow *fPatternTable1 = nullptr;
};


#endif
