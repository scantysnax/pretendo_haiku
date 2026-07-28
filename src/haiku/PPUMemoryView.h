#ifndef _PPU_MEMORY_VIEW_H_
#define _PPU_MEMORY_VIEW_H_

#include <View.h>
#include <ScrollBar.h>


class PretendoWindow;
class BScrollBar;
class PPUMemoryScrollBar;


// -----------------------------------------------------------------------------
// PPUMemoryView
//
// Debugger view for inspecting raw PPU memory.  The view reads PPU memory through
// the side-effect-free debug_read_ppu_memory() accessor, so it does not disturb
// the normal PPUDATA read buffer or VRAM address increment behavior.
//
// The view displays 16 bytes per row and supports quick jumps to pattern tables,
// nametables, and palette RAM.
// -----------------------------------------------------------------------------
class PPUMemoryView : public BView
{
	public:
			PPUMemoryView (BRect frame, PretendoWindow *parent);
	virtual ~PPUMemoryView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void Pulse();
	virtual void FrameResized (float width, float height);
	virtual void MouseDown (BPoint where);
	virtual void MouseMoved (BPoint where, uint32 transit, const BMessage *dragMessage);
	virtual void MessageReceived (BMessage *message);

	private:
	void DrawHeaderPanel();
	void DrawMemoryPanel();
	
	private:
	bool HasROMLoaded() const;
	void DrawNoROMMessage (BRect panel);

	private:
	void SetBaseAddress (uint16 address);
	void ScrollRows (int32 rows);

	private:
	void LayoutScrollBar();
	void UpdateScrollBar();
	void ScrollBarChanged (float value);
	void ScrollLines (int32 lines);
	friend class PPUMemoryScrollBar;
	
	private:
	void DrawByteCell (float x, float y, uint16 address, uint8 value, bool hovered, bool locked);
	void DrawASCIICharCell (float x, float y, uint16 address, char value, bool hovered, bool locked);
	void DrawSelectedByteInfo (float x, float y);

	private:
	bool AddressForPoint (BPoint where, uint16& address) const;
	bool HoverAddressForPoint (BPoint where);
	bool ActiveInspectAddress (uint16& address) const;
	
	private:
	const char* RegionName (uint16 address) const;

	private:
	PretendoWindow *fParent = nullptr;

	private:
	uint16 fBaseAddress = 0x0000;
	bool fFreezeUpdates = false;

	private:
	BScrollBar *fScrollBar = nullptr;
	bool fUpdatingScrollBar = false;
	
	private:
	bool fHasHoveredAddress = false;
	uint16 fHoveredAddress = 0x0000;
	bool fHasLockedAddress = false;
	uint16 fLockedAddress = 0x0000;

};


#endif // _PPU_MEMORY_VIEW_H_

