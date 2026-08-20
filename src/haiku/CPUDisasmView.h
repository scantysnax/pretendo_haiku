#ifndef _CPU_DISASM_VIEW_H_
#define _CPU_DISASM_VIEW_H_

#include <ScrollBar.h>
#include <String.h>
#include <View.h>

#include <cmath>

#include "CPUDisasm.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"

#include "Bus.h"
#include "Cart.h"
#include "Cpu.h"


struct CPUDisasmLine;

class BScrollBar;
class CPUDisasmScrollBar;
class PretendoWindow;


// -----------------------------------------------------------------------------
// CPUDisasmView
//
// Debugger view for inspecting live 6502 disassembly near the current program
// counter.  The view uses the side-effect-free CPU disassembler helper and
// highlights the instruction currently pointed to by PC.
// -----------------------------------------------------------------------------
class CPUDisasmView : public BView
{
	public:
			CPUDisasmView (BRect frame, PretendoWindow *parent);
	virtual ~CPUDisasmView();

	public:
	virtual void AttachedToWindow();
	virtual void Draw (BRect updateRect);
	virtual void KeyDown (const char *bytes, int32 numBytes);
	virtual void MouseDown (BPoint where);
	virtual void Pulse();
	virtual void FrameResized (float width, float height);
	
	public:
	void ResetView();
	void JumpToAddress (uint16 address);
	
	private:
	void DrawHeaderPanel();
	void DrawDisasmPanel();
	bool HasROMLoaded() const;
	void DrawNoROMMessage (BRect panel);
	void DrawRegisterSummary (BPoint where, uint8 a, uint8 x, uint8 y, uint8 s, uint8 p);
	void DrawInstructionLegend (BRect panel);
	
	private:
	void DrawDisasmLine (float y, uint16 address, bool active);
	void SetFollowPC (bool follow);
	
	private:
	void ScrollLines (int32 lines);
	bool IsPPURegisterWrite (const CPUDisasmLine &line) const;
	bool IsOAMDMAWrite (const CPUDisasmLine &line) const;
	bool IsStoreInstruction (const CPUDisasmLine &line) const;
	bool IsAPUOrControllerRegister (const CPUDisasmLine &line) const;
	bool IsControlFlowInstruction (const CPUDisasmLine &line) const;
	bool IsLoadInstruction (const CPUDisasmLine &line) const;
	bool IsUndocumentedInstruction (const CPUDisasmLine &line) const;
	const char *HardwareLabelForOperand (const CPUDisasmLine &line) const;
	const char *CPUIdiomCommentForLine (const CPUDisasmLine &line) const;
	void BuildCommentForLine (const CPUDisasmLine &line, BString &comment) const;
	bool BranchTakenForLine (const CPUDisasmLine &line) const;
	bool AddressForPoint (BPoint where, uint16 &address);
	
	private:
	bool ParseOperandAddress (const CPUDisasmLine &line, uint16& address) const;
	uint16 FindInstructionBefore (uint16 address) const;
	uint16 FindContextBase (uint16 pc, int32 linesBefore) const;
	
	private:
	uint16 ReadVector (uint16 address) const;
	void JumpToCurrentPC();
	void JumpToVector (uint16 vectorAddress);
	
	private:
	void LayoutScrollBar();
	void UpdateScrollBar();
	void ScrollBarChanged (float value);
	int32 VisibleDisasmRows() const;
	
	private:
	friend class CPUDisasmScrollBar;
	BScrollBar *fScrollBar = nullptr;
	bool fUpdatingScrollBar = false;

	private:
	PretendoWindow *fParent = nullptr;

	private:
	bool fFreezeUpdates = false;
	bool fFollowPC = true;
	uint16 fBaseAddress = 0x0000;
	uint16 fFrozenPC = 0x0000;

	bool fHasSelectedAddress = false;
	uint16 fSelectedAddress = 0x0000;
};


#endif // _CPU_DISASM_VIEW_H_

