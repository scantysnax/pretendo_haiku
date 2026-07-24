
#ifndef _CPU_DISASM_H_
#define _CPU_DISASM_H_

#include <String.h>


// -----------------------------------------------------------------------------
// CPUDisasmLine
//
// One decoded 6502 disassembly line.  The line contains the CPU address,
// instruction bytes, instruction length, mnemonic, operand text, and a combined
// display string suitable for drawing in the debugger.
// -----------------------------------------------------------------------------
struct CPUDisasmLine {
	uint16 address = 0;
	uint8 bytes[3] = {0, 0, 0};
	uint8 length = 1;

	BString mnemonic;
	BString operand;
	BString text;
};


CPUDisasmLine DisassembleCPU (uint16 address);


#endif // _CPU_DISASM_H_
