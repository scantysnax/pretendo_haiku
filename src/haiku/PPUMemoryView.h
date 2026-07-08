#ifndef _PPU_MEMORY_VIEW_H_
#define _PPU_MEMORY_VIEW_H_

#include <View.h>


class PretendoWindow;


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

	private:
	void DrawHeaderPanel();
	void DrawMemoryPanel();
	
	private:
	void SetBaseAddress (uint16 address);
	void ScrollRows (int32 rows);

	private:
	const char *RegionName (uint16 address) const;

	private:
	PretendoWindow *fParent = nullptr;
	
	private:
	uint16 fBaseAddress = 0x2000;
	bool fFreezeUpdates = false;
};


#endif // _PPU_MEMORY_VIEW_H_

