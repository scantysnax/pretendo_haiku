
#ifndef _CPU_MEMORY_VIEW_H_
#define _CPU_MEMORY_VIEW_H_

#include <ScrollBar.h>
#include <String.h>
#include <View.h>

#include <cmath>
#include <cstring>

#include "Bus.h"
#include "Cart.h"
#include "Cpu.h"
#include "CPUDisasm.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"


class BScrollBar;
class CPUMemoryScrollBar;
class PretendoWindow;


class CPUMemoryView : public BView
{
	public:
			CPUMemoryView(BRect frame, PretendoWindow* parent);
	virtual ~CPUMemoryView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void Pulse();
	virtual void FrameResized (float width, float height);
	virtual void MouseDown (BPoint where);
	virtual void MouseMoved (BPoint where, uint32 transit, const BMessage *dragMessage);
	
	private:
	bool AddressForPoint (BPoint where, uint16 &address) const;
	void DrawSelectedByteInfo (float x, float y);
	bool HoverAddressForPoint (BPoint where);
	bool ActiveInspectAddress (uint16 &address) const;
	void DrawInstructionTargetInfo (float x, float y);

	private:
	void DrawHeaderPanel();
	void DrawMemoryPanel();
	void DrawNoROMMessage(BRect panel);
	void DrawByteCell (float x, float y, uint16 address, uint8 value, bool isPC, 
						bool isPCOperand, bool isSP);
	private:
	void JumpToAddress (uint16 address);
	void ScrollLines (int32 lines);
	bool HasROMLoaded() const;
	uint16 ReadVector (uint16 address) const;
	const char* RegionLabel (uint16 address) const;
	const char* VectorLabel (uint16 address) const;
	void SetRegionBackgroundColor (uint16 address);
	
	private:
	bool CurrentInstructionTarget (uint16 &address) const;
	bool ParseOperandAddress (const BString &operand, uint16 &value, int32 &digits) const;
	uint16 ReadZeroPageVector (uint8 address) const;
	uint16 Read6502IndirectVector (uint16 address) const;
	
	
	private:
	PretendoWindow *fParent = nullptr;
	uint16 fBaseAddress = 0x0000;
	
	private:
	void LayoutScrollBar();
	void UpdateScrollBar();
	void ScrollBarChanged (float value);
	int32 VisibleMemoryRows() const;
	
	private:
	friend class CPUMemoryScrollBar;
	BScrollBar *fScrollBar = nullptr;
	bool fUpdatingScrollBar = false;
	
	bool fHasHoveredAddress = false;
	uint16 fHoveredAddress = 0x0000;

	bool fHasLockedAddress = false;
	uint16 fLockedAddress = 0x0000;
};

#endif	// _CPU_MEMORY_VIEW_H_

