#ifndef _STACK_VIEW_H_
#define _STACK_VIEW_H_

#include <View.h>

#include <cmath>

#include "Bus.h"
#include "Cart.h"
#include "Cpu.h"
#include "DebugHelpers.h"
#include "PretendoWindow.h"


// -----------------------------------------------------------------------------
// StackView
//
// Displays CPU stack page RAM, $0100-$01FF, as a 16x16 byte grid with live/
// frozen snapshots, changed-byte highlighting, SP highlighting, selected-byte
// inspection, stack activity history, stack-depth warnings, and heuristic
// call-stack reconstruction.
// -----------------------------------------------------------------------------
class StackView : public BView
{
	public:
			StackView(BRect frame, PretendoWindow* parent);
	virtual ~StackView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void MouseDown(BPoint where);
	virtual void Pulse();

	private:
	enum StackActivityType {
		STACK_ACTIVITY_NONE = 0,

		STACK_ACTIVITY_PUSH,
		STACK_ACTIVITY_POP,

		STACK_ACTIVITY_PHA,
		STACK_ACTIVITY_PHP,
		STACK_ACTIVITY_PLA,
		STACK_ACTIVITY_PLP,
		STACK_ACTIVITY_JSR,
		STACK_ACTIVITY_RTS,
		STACK_ACTIVITY_RTI,

		STACK_ACTIVITY_IRQ,
		STACK_ACTIVITY_NMI,
		STACK_ACTIVITY_BRK,
		STACK_ACTIVITY_INTERRUPT,

		STACK_ACTIVITY_WRAP_DOWN,
		STACK_ACTIVITY_WRAP_UP
	};

	private:
	struct StackActivity {
		StackActivityType type = STACK_ACTIVITY_NONE;

		uint8 oldSP = 0xff;
		uint8 newSP = 0xff;

		uint16 firstAddress = 0x0100;
		uint16 lastAddress = 0x0100;

		uint8 value = 0;
		uint16 count = 0;

		uint16 instructionAddress = 0;
		uint8 opcode = 0;
		uint16 targetAddress = 0;
		uint16 resumeAddress = 0;

		uint8 resultA = 0;
		uint8 resultP = 0;

		uint64 sequence = 0;
	};
	
	enum CallStackConfidence {
		CALL_STACK_CONFIDENCE_MEDIUM = 0,
		CALL_STACK_CONFIDENCE_HIGH
	};

	struct CallStackCandidate {
		uint16 lowByteAddress = 0x0100;
		uint16 highByteAddress = 0x0101;

		uint16 rawReturnAddress = 0;
		uint16 resumeAddress = 0;
		uint16 callSiteAddress = 0;

		CallStackConfidence confidence = CALL_STACK_CONFIDENCE_MEDIUM;
	};

	static const int32 kStackHistoryCapacity = 8;
	static const int32 kCallStackCandidateCapacity = 16;
	static const int32 kVisibleBottomPanelLines = 8;

	private:
	void DrawHeaderUI();
	void DrawStackSummaryPanel();
	void DrawStackGrid();
	void DrawSelectedBytePanel();
	void DrawStackHistoryPanel();
	void DrawPossibleCallStackPanel();
	void DrawNoROMMessage(BRect panel);

	private:
	void CaptureStackSnapshot();
	void RecordStackActivity(uint8 oldSP, uint8 newSP);
	void ClearStackHistory();
	void UpdateStackHighWater (uint8 sp);
	void CaptureInstructionStackHistory();
	void RecordInstructionStackActivity(const nes::cpu::cpu_trace_entry_t& entry, 
										const nes::cpu::cpu_trace_entry_t& nextEntry);
	void RecordInterruptStackActivity(const nes::cpu::cpu_trace_entry_t& entry, const nes::cpu::cpu_trace_entry_t& nextEntry);
	int32 BuildPossibleCallStack(CallStackCandidate* candidates, int32 capacity) const;
	

	bool HasROMLoaded() const;
	bool AddressForPoint(BPoint where, uint16& address) const;

	void MoveSelection(int32 delta);

	int32 ChangedByteCount() const;

	uint8 StackPointer() const;
	uint16 StackPointerAddress() const;

	const char* StackWarningText() const;

	private:
	PretendoWindow* fParent = nullptr;

	private:
	bool fFreezeUpdates = false;
	bool fFollowStackPointer = false;
	bool fShowStackHistory = false;
	bool fShowPossibleCallStack = false;
	bool fHaveSnapshot = false;

	uint8 fBytes[0x100] = {};
	uint8 fPreviousBytes[0x100] = {};
	uint8 fChangeAge[0x100] = {};
	bool fChanged[0x100] = {};

	private:
	bool fHasPreviousStackPointer = false;
	uint8 fPreviousStackPointer = 0xff;
	
	bool fHaveStackHighWater = false;
	uint8 fStackBaselinePointer = 0xff;
	uint8 fLowestStackPointer = 0xff;
	uint16 fPeakStackDepth = 0;
	
	bool fStackWrapDetected = false;
	uint8 fStackWrapWarningAge = 0;

	StackActivity fStackHistory[kStackHistoryCapacity] = {};
	int32 fStackHistoryCount = 0;
	int32 fStackHistoryNext = 0;
	uint64 fStackActivitySequence = 0;

	private:
	bool fHasSelectedAddress = false;
	uint16 fSelectedAddress = 0x01ff;
	
	bool fHaveProcessedTraceCycle = false;
	uint64 fLastProcessedTraceCycle = 0;
};


#endif // _STACK_VIEW_H_

