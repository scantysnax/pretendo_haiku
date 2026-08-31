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
	bool IsPPURegisterWrite (const cpu_disasm_line_t &line) const;
	bool IsOAMDMAWrite (const cpu_disasm_line_t &line) const;
	bool IsStoreInstruction (const cpu_disasm_line_t &line) const;
	bool IsAPUOrControllerRegister (const cpu_disasm_line_t &line) const;
	bool IsControlFlowInstruction (const cpu_disasm_line_t &line) const;
	bool IsLoadInstruction (const cpu_disasm_line_t &line) const;
	bool IsConditionalBranchInstruction (const cpu_disasm_line_t &line) const;
	bool IsUndocumentedInstruction (const cpu_disasm_line_t &line) const;
	const char *HardwareLabelForOperand (const cpu_disasm_line_t &line) const;
	const char *CPUIdiomCommentForLine (const cpu_disasm_line_t &line) const;
	void BuildCommentForLine (const cpu_disasm_line_t &line, BString &comment) const;
	bool BranchTakenForLine (const cpu_disasm_line_t &line) const;
	bool AddressForPoint (BPoint where, uint16 &address);
	
	private:
	bool ParseOperandAddress (const cpu_disasm_line_t &line, uint16& address) const;
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
	void CaptureFrozenSnapshot();
	cpu_disasm_line_t DisassembleAddress (uint16 address) const;
	
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
	bool fHaveFrozenSnapshot = false;
	uint8 fFrozenA = 0x00;
	uint8 fFrozenX = 0x00;
	uint8 fFrozenY = 0x00;
	uint8 fFrozenS = 0x00;
	uint8 fFrozenP = 0x00;
	uint8 fFrozenMemory[0x10000] = {};
	
	private:
	bool fHasSelectedAddress = false;
	uint16 fSelectedAddress = 0x0000;
};


#endif // _CPU_DISASM_VIEW_H_

