
#ifndef CPU_20080314_H_
#define CPU_20080314_H_

#include "BitField.h"
#include "Reset.h"
#include <cstdint>

namespace nes::cpu {

using dma_handler_t = void (*)(uint8_t);

enum IrqSource : uint8_t {
	MAPPER_IRQ = 0x01,
	APU_IRQ    = 0x02,
	FDS_IRQ    = 0x04,
	ALL_IRQ    = 0xff
};

enum : uint8_t {
	C_MASK = 0x01,
	Z_MASK = 0x02,
	I_MASK = 0x04,
	D_MASK = 0x08,
	B_MASK = 0x10,
	R_MASK = 0x20, // antisocial flag... always 1
	V_MASK = 0x40,
	N_MASK = 0x80
};

union register16 {
	uint_least16_t raw;
	BitField<uint_least16_t, 0, 8> lo;
	BitField<uint_least16_t, 8, 8> hi;
};

// API
uint64_t cycle_count();
void clear_irq(IrqSource source);
void clear_nmi();
void irq(IrqSource source);
void schedule_spr_dma(dma_handler_t dma_handler, uint_least16_t source_address, uint_least16_t count);
void schedule_dmc_dma(dma_handler_t dma_handler, uint_least16_t source_address, uint_least16_t count);
void reset(Reset reset_type);
void reset();
void stop();
void nmi();
void tick();

// public registers
extern register16 PC;
extern uint8_t A;
extern uint8_t X;
extern uint8_t Y;
extern uint8_t S;
extern uint8_t P;

template <int Cycles>
void exec() {
	for (int i = 0; i < Cycles; ++i) {
		tick();
	}
}

struct cpu_state_t {
	uint16_t pc;
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t s;
	uint8_t p;
	uint16_t instruction;
	int cycle;
	uint64_t executed_cycles;
};


struct cpu_trace_entry_t {
	uint64_t cycle = 0;
	uint16_t pc = 0;
	uint8_t bytes[3] = {0, 0, 0};
	uint8_t length = 1;

	uint8_t a = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t s = 0;
	uint8_t p = 0;
};

enum : uint32_t {
	CPU_TRACE_CAPACITY = 2048
};


enum DebugBreakReason : uint8_t {
	DEBUG_BREAK_NONE = 0,
	DEBUG_BREAK_EXECUTE,
	DEBUG_BREAK_STACK_SP,
	DEBUG_BREAK_STACK_WRAP
};


uint8_t debug_s();


cpu_state_t debug_cpu_state();

bool debug_instruction_boundary();
bool debug_instruction_was_executed(uint16_t address);
void debug_clear_instruction_trace();

uint32_t debug_cpu_trace_count();
uint32_t debug_cpu_trace_capacity();
bool debug_cpu_trace_entry(uint32_t index, cpu_trace_entry_t& entry);
void debug_clear_cpu_trace();

void debug_add_execute_breakpoint(uint16_t address);
void debug_remove_execute_breakpoint(uint16_t address);
void debug_clear_execute_breakpoints();
bool debug_has_execute_breakpoint(uint16_t address);

bool debug_breakpoint_hit();
uint16_t debug_breakpoint_hit_address();
void debug_clear_breakpoint_hit();
void debug_skip_breakpoint_once();
void debug_resume_past_breakpoint();
uint32_t debug_breakpoint_hit_count(uint16_t address);
void debug_clear_breakpoint_hit_counts();

// stack stuff
DebugBreakReason debug_break_reason();
void debug_set_stack_sp_break(bool enabled, uint8_t threshold);
bool debug_stack_sp_break_enabled();
uint8_t debug_stack_sp_break_threshold();
void debug_set_stack_wrap_break(bool enabled);
bool debug_stack_wrap_break_enabled();
bool debug_stack_sp_break_armed();

uint8_t debug_stack_break_old_s();
uint8_t debug_stack_break_new_s();

}

#endif

