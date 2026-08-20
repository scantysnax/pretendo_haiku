
#include "CPUDisasm.h"


enum AddressMode {
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


struct OpcodeInfo {
	const char *mnemonic;
	AddressMode mode;
};


static const OpcodeInfo kOpcodeTable[256] = {
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


static uint8
DebugRead (uint16 address)
{
	return nes::bus::debug_read_memory(address);
}


static uint16
ReadOp16 (uint16 address)
{
	uint16 lo = DebugRead(address + 1);
	uint16 hi = DebugRead(address + 2);

	return lo | (hi << 8);
}


static uint8
InstructionLength (AddressMode mode)
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


static void
FormatOperand (uint16 address, AddressMode mode, BString &operand)
{
	operand.SetTo("");

	uint8 op8 = DebugRead(address + 1);
	uint16 op16 = ReadOp16(address);

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


static void
FormatBytes (const CPUDisasmLine &line, BString &bytes)
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


CPUDisasmLine
DisassembleCPU (uint16 address)
{
	CPUDisasmLine line;

	line.address = address;

	uint8 opcode = DebugRead(address);
	const OpcodeInfo &info = kOpcodeTable[opcode];

	line.length = InstructionLength(info.mode);
	line.bytes[0] = opcode;
	line.bytes[1] = DebugRead(address + 1);
	line.bytes[2] = DebugRead(address + 2);

	line.mnemonic.SetTo(info.mnemonic);
	FormatOperand(address, info.mode, line.operand);

	BString bytes;

	FormatBytes(line, bytes);

	if (line.operand.Length() > 0) {
		line.text.SetToFormat(
			"$%04X:  %-8s  %s %s",
			line.address,
			bytes.String(),
			line.mnemonic.String(),
			line.operand.String()
		);
	} else {
		line.text.SetToFormat(
			"$%04X:  %-8s  %s",
			line.address,
			bytes.String(),
			line.mnemonic.String()
		);
	}

	return line;
}

