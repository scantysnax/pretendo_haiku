
#include "CPUDisasm.h"


// -----------------------------------------------------------------------------
// address_mode
//
// Identifies the 6502 addressing mode used by an opcode.
//
// The selected mode determines both the instruction length and how the operand
// bytes are formatted for debugger disassembly output.
//
// -----------------------------------------------------------------------------
enum address_mode {
	AM_IMP,
	AM_ACC,
	AM_REL,
	AM_IMM,
	AM_ABS,
	AM_ABS_X,
	AM_ABS_Y,
	AM_ZERO,
	AM_ZERO_X,
	AM_ZERO_Y,
	AM_IND,
	AM_IND_X,
	AM_IND_Y
};


// -----------------------------------------------------------------------------
// opcode_info_t
//
// Describes one 6502 opcode for the debugger disassembler.
//
// Each opcode maps to a mnemonic string and an addressing mode.  Together these
// determine the displayed instruction text and encoded instruction length.
//
// Members:
//   mnemonic - Instruction mnemonic.
//   mode     - Addressing mode used by the opcode.
//
// -----------------------------------------------------------------------------
struct opcode_info_t {
	const char *mnemonic;
	address_mode mode;
};


// -----------------------------------------------------------------------------
// kOpcodeTable
//
// Complete 256-entry 6502 opcode description table used by the debugger
// disassembler.
//
// The table includes both documented and undocumented opcodes.  Each byte value
// maps directly to its mnemonic and addressing mode so instruction decoding does
// not require a large switch statement.
//
// -----------------------------------------------------------------------------
static const opcode_info_t kOpcodeTable[256] = {
	{ "brk", AM_IMP },    { "ora", AM_IND_X },  { "jam", AM_IMP },    { "slo", AM_IND_X },
	{ "nop", AM_ZERO },   { "ora", AM_ZERO },   { "asl", AM_ZERO },   { "slo", AM_ZERO },
	{ "php", AM_IMP },    { "ora", AM_IMM },    { "asl", AM_ACC },    { "anc", AM_IMM },
	{ "nop", AM_ABS },    { "ora", AM_ABS },    { "asl", AM_ABS },    { "slo", AM_ABS },

	{ "bpl", AM_REL },    { "ora", AM_IND_Y },  { "jam", AM_IMP },    { "slo", AM_IND_Y },
	{ "nop", AM_ZERO_X }, { "ora", AM_ZERO_X }, { "asl", AM_ZERO_X }, { "slo", AM_ZERO_X },
	{ "clc", AM_IMP },    { "ora", AM_ABS_Y },  { "nop", AM_IMP },    { "slo", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "ora", AM_ABS_X },  { "asl", AM_ABS_X },  { "slo", AM_ABS_X },

	{ "jsr", AM_ABS },    { "and", AM_IND_X },  { "jam", AM_IMP },    { "rla", AM_IND_X },
	{ "bit", AM_ZERO },   { "and", AM_ZERO },   { "rol", AM_ZERO },   { "rla", AM_ZERO },
	{ "plp", AM_IMP },    { "and", AM_IMM },    { "rol", AM_ACC },    { "anc", AM_IMM },
	{ "bit", AM_ABS },    { "and", AM_ABS },    { "rol", AM_ABS },    { "rla", AM_ABS },

	{ "bmi", AM_REL },    { "and", AM_IND_Y },  { "jam", AM_IMP },    { "rla", AM_IND_Y },
	{ "nop", AM_IMP },    { "and", AM_ZERO_X }, { "rol", AM_ZERO_X }, { "rla", AM_ZERO_X },
	{ "sec", AM_IMP },    { "and", AM_ABS_Y },  { "nop", AM_IMP },    { "rla", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "and", AM_ABS_X },  { "rol", AM_ABS_X },  { "rla", AM_ABS_X },

	{ "rti", AM_IMP },    { "eor", AM_IND_X },  { "jam", AM_IMP },    { "sre", AM_IND_X },
	{ "nop", AM_ZERO },   { "eor", AM_ZERO },   { "lsr", AM_ZERO },   { "sre", AM_ZERO },
	{ "pha", AM_IMP },    { "eor", AM_IMM },    { "lsr", AM_ACC },    { "asr", AM_IMM },
	{ "jmp", AM_ABS },    { "eor", AM_ABS },    { "lsr", AM_ABS },    { "sre", AM_ABS },

	{ "bvc", AM_REL },    { "eor", AM_IND_Y },  { "jam", AM_IMP },    { "sre", AM_IND_Y },
	{ "nop", AM_ZERO_X }, { "eor", AM_ZERO_X }, { "lsr", AM_ZERO_X }, { "sre", AM_ZERO_X },
	{ "cli", AM_IMP },    { "eor", AM_ABS_Y },  { "nop", AM_IMP },    { "sre", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "eor", AM_ABS_X },  { "lsr", AM_ABS_X },  { "sre", AM_ABS_X },

	{ "rts", AM_IMP },    { "adc", AM_IND_X },  { "jam", AM_IMP },    { "rra", AM_IND_X },
	{ "nop", AM_ZERO },   { "adc", AM_ZERO },   { "ror", AM_ZERO },   { "rra", AM_ZERO },
	{ "pla", AM_IMP },    { "adc", AM_IMM },    { "ror", AM_ACC },    { "arr", AM_IMM },
	{ "jmp", AM_IND },    { "adc", AM_ABS },    { "ror", AM_ABS },    { "rra", AM_ABS },

	{ "bvs", AM_REL },    { "adc", AM_IND_Y },  { "jam", AM_IMP },    { "rra", AM_IND_Y },
	{ "nop", AM_ZERO_X }, { "adc", AM_ZERO_X }, { "ror", AM_ZERO_X }, { "rra", AM_ZERO_X },
	{ "sei", AM_IMP },    { "adc", AM_ABS_Y },  { "nop", AM_IMP },    { "rra", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "adc", AM_ABS_X },  { "ror", AM_ABS_X },  { "rra", AM_ABS_X },

	{ "nop", AM_IMM },    { "sta", AM_IND_X },  { "nop", AM_IMM },    { "sax", AM_IND_X },
	{ "sty", AM_ZERO },   { "sta", AM_ZERO },   { "stx", AM_ZERO },   { "sax", AM_ZERO },
	{ "dey", AM_IMP },    { "nop", AM_IMM },    { "txa", AM_IMP },    { "ane", AM_IMM },
	{ "sty", AM_ABS },    { "sta", AM_ABS },    { "stx", AM_ABS },    { "sax", AM_ABS },

	{ "bcc", AM_REL },    { "sta", AM_IND_Y },  { "jam", AM_IMP },    { "sha", AM_IND_Y },
	{ "sty", AM_ZERO_X }, { "sta", AM_ZERO_X }, { "stx", AM_ZERO_Y }, { "sax", AM_ZERO_Y },
	{ "tya", AM_IMP },    { "sta", AM_ABS_Y },  { "txs", AM_IMP },    { "shs", AM_ABS_Y },
	{ "shy", AM_ABS_X },  { "sta", AM_ABS_X },  { "shx", AM_ABS_Y },  { "sha", AM_ABS_Y },

	{ "ldy", AM_IMM },    { "lda", AM_IND_X },  { "ldx", AM_IMM },    { "lax", AM_IND_X },
	{ "ldy", AM_ZERO },   { "lda", AM_ZERO },   { "ldx", AM_ZERO },   { "lax", AM_ZERO },
	{ "tay", AM_IMP },    { "lda", AM_IMM },    { "tax", AM_IMP },    { "lxa", AM_IMM },
	{ "ldy", AM_ABS },    { "lda", AM_ABS },    { "ldx", AM_ABS },    { "lax", AM_ABS },

	{ "bcs", AM_REL },    { "lda", AM_IND_Y },  { "jam", AM_IMP },    { "lax", AM_IND_Y },
	{ "ldy", AM_ZERO_X }, { "lda", AM_ZERO_X }, { "ldx", AM_ZERO_Y }, { "lax", AM_ZERO_Y },
	{ "clv", AM_IMP },    { "lda", AM_ABS_Y },  { "tsx", AM_IMP },    { "las", AM_ABS_Y },
	{ "ldy", AM_ABS_X },  { "lda", AM_ABS_X },  { "ldx", AM_ABS_Y },  { "lax", AM_ABS_Y },

	{ "cpy", AM_IMM },    { "cmp", AM_IND_X },  { "nop", AM_IMM },    { "dcp", AM_IND_X },
	{ "cpy", AM_ZERO },   { "cmp", AM_ZERO },   { "dec", AM_ZERO },   { "dcp", AM_ZERO },
	{ "iny", AM_IMP },    { "cmp", AM_IMM },    { "dex", AM_IMP },    { "sbx", AM_IMM },
	{ "cpy", AM_ABS },    { "cmp", AM_ABS },    { "dec", AM_ABS },    { "dcp", AM_ABS },

	{ "bne", AM_REL },    { "cmp", AM_IND_Y },  { "jam", AM_IMP },    { "dcp", AM_IND_Y },
	{ "nop", AM_ZERO_X }, { "cmp", AM_ZERO_X }, { "dec", AM_ZERO_X }, { "dcp", AM_ZERO_X },
	{ "cld", AM_IMP },    { "cmp", AM_ABS_Y },  { "nop", AM_IMP },    { "dcp", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "cmp", AM_ABS_X },  { "dec", AM_ABS_X },  { "dcp", AM_ABS_X },

	{ "cpx", AM_IMM },    { "sbc", AM_IND_X },  { "nop", AM_IMM },    { "isb", AM_IND_X },
	{ "cpx", AM_ZERO },   { "sbc", AM_ZERO },   { "inc", AM_ZERO },   { "isb", AM_ZERO },
	{ "inx", AM_IMP },    { "sbc", AM_IMM },    { "nop", AM_IMP },    { "sbc", AM_IMM },
	{ "cpx", AM_ABS },    { "sbc", AM_ABS },    { "inc", AM_ABS },    { "isb", AM_ABS },

	{ "beq", AM_REL },    { "sbc", AM_IND_Y },  { "jam", AM_IMP },    { "isb", AM_IND_Y },
	{ "nop", AM_ZERO_X }, { "sbc", AM_ZERO_X }, { "inc", AM_ZERO_X }, { "isb", AM_ZERO_X },
	{ "sed", AM_IMP },    { "sbc", AM_ABS_Y },  { "nop", AM_IMP },    { "isb", AM_ABS_Y },
	{ "nop", AM_ABS_X },  { "sbc", AM_ABS_X },  { "inc", AM_ABS_X },  { "isb", AM_ABS_X }
};


// -----------------------------------------------------------------------------
// DebugRead
//
// Reads one byte from CPU-visible memory through the debugger-safe memory path.
//
// Debugger reads do not use the normal CPU access path, avoiding side effects
// and preventing debugger inspection from triggering memory watchpoints.
//
// Parameters:
//   address - CPU address to read.
//
// Returns:
//   Byte currently visible at the supplied CPU address.
// -----------------------------------------------------------------------------
static uint8
DebugRead (uint16 address)
{
	return nes::bus::debug_read_memory(address);
}


// -----------------------------------------------------------------------------
// InstructionLength
//
// Returns the encoded byte length of a 6502 instruction from its addressing
// mode.
//
// Parameters:
//   mode - Addressing mode of the instruction.
//
// Returns:
//   Instruction length in bytes: 1, 2, or 3.
// -----------------------------------------------------------------------------
static uint8
InstructionLength (address_mode mode)
{
	switch (mode) {
		case AM_IMP:
		case AM_ACC:
			return 1;

		case AM_REL:
		case AM_IMM:
		case AM_ZERO:
		case AM_ZERO_X:
		case AM_ZERO_Y:
		case AM_IND_X:
		case AM_IND_Y:
			return 2;

		case AM_ABS:
		case AM_ABS_X:
		case AM_ABS_Y:
		case AM_IND:
			return 3;
	}

	return 1;
}


// -----------------------------------------------------------------------------
// FormatOperand
//
// Formats the operand text for an instruction using already-supplied opcode
// bytes.
//
// Using captured bytes rather than rereading CPU memory allows historical CPU
// trace entries to remain correct even if mapper bank switching later changes
// the contents currently visible at the same CPU address.
//
// Parameters:
//   address - CPU address at which the instruction was executed.
//   mode    - Addressing mode for the instruction.
//   bytes   - Captured instruction bytes.
//   operand - Receives the formatted operand text.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
FormatOperand (uint16 address, address_mode mode, const uint8 bytes[3], BString &operand)
{
	operand.SetTo("");

	const uint8 op8 = bytes[1];
	const uint16 op16 = static_cast<uint16>(bytes[1] | (static_cast<uint16>(bytes[2]) << 8));

	switch (mode) {
		case AM_IMP:
			operand.SetTo("");
			break;

		case AM_ACC:
			operand.SetTo("A");
			break;

		case AM_REL:
		{
			int16 target = static_cast<int8>(op8);
			target += static_cast<int16>(address + 2);
			operand.SetToFormat("$%04X", static_cast<uint16>(target));
			break;
		}

		case AM_IMM:
			operand.SetToFormat("#$%02X", op8);
			break;

		case AM_ABS:
			operand.SetToFormat("$%04X", op16);
			break;

		case AM_ABS_X:
			operand.SetToFormat("$%04X,X", op16);
			break;

		case AM_ABS_Y:
			operand.SetToFormat("$%04X,Y", op16);
			break;

		case AM_ZERO:
			operand.SetToFormat("$%02X", op8);
			break;

		case AM_ZERO_X:
			operand.SetToFormat("$%02X,X", op8);
			break;

		case AM_ZERO_Y:
			operand.SetToFormat("$%02X,Y", op8);
			break;

		case AM_IND:
			operand.SetToFormat("($%04X)", op16);
			break;

		case AM_IND_X:
			operand.SetToFormat("($%02X,X)", op8);
			break;

		case AM_IND_Y:
			operand.SetToFormat("($%02X),Y", op8);
			break;
	}
}


// -----------------------------------------------------------------------------
// FormatBytes
//
// Formats the raw instruction bytes of a disassembled 6502 instruction into a
// fixed-width textual representation.
//
// Unused byte positions are filled with spaces so instruction text remains
// aligned regardless of whether the instruction is one, two, or three bytes
// long.
//
// Parameters:
//   line  - Disassembled instruction containing length and raw bytes.
//   bytes - Receives the formatted byte string.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
FormatBytes (const cpu_disasm_line_t &line, BString &bytes)
{
	bytes.SetTo("");

	BString s;

	for (uint8 i = 0; i < 3; i++) {
		if (i < line.length) {
			s.SetToFormat("%02X", line.bytes[i]);
		} else {
			s.SetTo("  ");
		}

		bytes.Append(s);

		if (i != 2) {
			bytes.Append(" ");
		}
	}
}


// -----------------------------------------------------------------------------
// DisassembleCPUBytes
//
// Disassembles a 6502 instruction from supplied instruction bytes.
//
// Unlike DisassembleCPU(), this function does not read CPU memory.  This makes
// it suitable for historical execution trace entries whose bytes were captured
// when the instruction actually executed and whose original mapper bank may no
// longer be visible.
//
// Parameters:
//   address - CPU address at which the instruction was executed.
//   bytes   - Three captured bytes beginning with the opcode.
//
// Returns:
//   Fully formatted disassembly information for the supplied instruction.
// -----------------------------------------------------------------------------
cpu_disasm_line_t
DisassembleCPUBytes (uint16 address, const uint8 bytes[3])
{
	cpu_disasm_line_t line;

	line.address = address;

	const uint8 opcode = bytes[0];
	const opcode_info_t &info = kOpcodeTable[opcode];

	line.length = InstructionLength(info.mode);

	line.bytes[0] = bytes[0];
	line.bytes[1] = bytes[1];
	line.bytes[2] = bytes[2];

	line.mnemonic.SetTo(info.mnemonic);

	FormatOperand(address, info.mode, bytes, line.operand);

	BString byteText;
	FormatBytes(line, byteText);

	if (line.operand.Length() > 0) {
		line.text.SetToFormat("$%04X:  %-8s  %s %s", line.address, byteText.String(),
								line.mnemonic.String(),
								line.operand.String());
	} else {
		line.text.SetToFormat("$%04X:  %-8s  %s", line.address, byteText.String(),
								line.mnemonic.String());
	}

	return line;
}


// -----------------------------------------------------------------------------
// DisassembleCPU
//
// Disassembles the instruction currently visible at a CPU memory address.
//
// The instruction bytes are read through the side-effect-free debugger memory
// path and then passed to DisassembleCPUBytes(), keeping all opcode decoding and
// formatting in one shared implementation.
//
// Parameters:
//   address - CPU address of the instruction to decode.
//
// Returns:
//   Fully formatted disassembly information for the current instruction.
// -----------------------------------------------------------------------------
cpu_disasm_line_t
DisassembleCPU (uint16 address)
{
	uint8 bytes[3];

	bytes[0] = DebugRead(address);
	bytes[1] = DebugRead(static_cast<uint16>(address + 1));
	bytes[2] = DebugRead(static_cast<uint16>(address + 2));

	return DisassembleCPUBytes(address, bytes);
}

