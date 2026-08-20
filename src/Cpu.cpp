
#ifdef _MSC_VER
#pragma warning(disable : 4127)
#endif

#include "Cpu.h"
#include "Bus.h"
#include "Cart.h"
#include "Compiler.h"
#include "Mapper.h"
#include "Nes.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#define LAST_CYCLE                                         \
	do {                                                   \
		rst_executing_ = false;                            \
		nmi_executing_ = false;                            \
		irq_executing_ = false;                            \
                                                           \
		if (rst_asserted_) {                               \
			rst_executing_ = true;                         \
		} else if (nmi_asserted_) {                        \
			nmi_executing_ = true;                         \
		} else if (irq_asserted_ && ((P & I_MASK) == 0)) { \
			irq_executing_ = true;                         \
		}                                                  \
		nmi_asserted_ = false;                             \
		rst_asserted_ = false;                             \
	} while (0)

#define OPCODE_COMPLETE \
	do {                \
		cycle_ = -1;    \
		return;         \
	} while (0)

namespace nes::cpu {

// public registers
register16 PC = {};
uint8_t A     = 0;
uint8_t X     = 0;
uint8_t Y     = 0;
uint8_t S     = 0;
uint8_t P     = I_MASK | R_MASK;

namespace {

constexpr uint16_t NmiVectorAddress = 0xfffa;
constexpr uint16_t RstVectorAddress = 0xfffc;
constexpr uint16_t IrqVectorAddress = 0xfffe;
constexpr uint16_t StackAddress     = 0x0100;

constexpr uint8_t flag_table_[256] = {
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};

uint8_t irq_sources_ = 0x00;

// internal registers
uint16_t instruction_ = 0;
int cycle_            = 0;


// Debug trace / breakpoint state.
bool sExecutedInstructionAddress[0x10000] = {};
bool sExecuteBreakpointAddress[0x10000] = {};

bool sDebugBreakpointHit = false;
uint16_t sDebugBreakpointHitAddress = 0x0000;
uint16_t sDebugCurrentInstructionAddress = 0x0000;
DebugBreakReason sDebugBreakReason = DEBUG_BREAK_NONE;

bool sDebugSkipBreakpointOnce = false;
uint32_t sExecuteBreakpointHitCount[0x10000] = {};


// Stack break conditions.
bool sDebugStackSPBreakEnabled = false;
uint8_t sDebugStackSPBreakThreshold = 0x20;
bool sDebugStackSPBreakArmed = true;
bool sDebugStackSPBreakReady = false;
bool sDebugStackWrapBreakEnabled = false;
bool sDebugHavePreviousBoundaryS = false;
uint8_t sDebugPreviousBoundaryS = 0xff;
uint8_t sDebugPreviousBoundaryOpcode = 0x00;
bool sDebugPreviousBoundaryWasInterrupt = false;
/*
 * SP values associated with the most recent stack breakpoint.
 */
uint8_t sDebugStackBreakOldS = 0xff;
uint8_t sDebugStackBreakNewS = 0xff;

// debug CPU execution trace
cpu_trace_entry_t sCPUTraceEntries[CPU_TRACE_CAPACITY] = {};
uint32_t sCPUTraceNext = 0;
uint32_t sCPUTraceCount = 0;
static void record_cpu_trace_entry(uint16_t address);

// watchpoints
static bool sDebugReadWatchpointAddress[0x10000] = {};
static bool sDebugWriteWatchpointAddress[0x10000] = {};

static uint32_t sDebugReadWatchpointHitCount[0x10000] = {};
static uint32_t sDebugWriteWatchpointHitCount[0x10000] = {};

static uint16_t sDebugMemoryBreakAddress = 0x0000;


// internal registers (which get trashed by instructions)
register16 effective_address_ = {};
register16 data16_            = {};
register16 old_pc_            = {};
register16 new_pc_            = {};
uint8_t data8_                = {};

bool irq_asserted_  = false;
bool nmi_asserted_  = false;
bool rst_asserted_  = false;
bool irq_executing_ = false;
bool nmi_executing_ = false;
bool rst_executing_ = true;

dma_handler_t spr_dma_handler_         = nullptr;
uint_least16_t spr_dma_source_address_ = 0;
uint_least16_t spr_dma_count_          = 0;
uint8_t spr_dma_byte_                  = 0;
uint8_t spr_dma_delay_                 = 0;

dma_handler_t dmc_dma_handler_         = nullptr;
uint_least16_t dmc_dma_source_address_ = 0;
uint_least16_t dmc_dma_count_          = 0;
uint8_t dmc_dma_byte_                  = 0;
uint8_t dmc_dma_delay_                 = 0;

// stats
uint64_t executed_cycles_ = 1; // NOTE(eteran): 1 instead of 0 makes 4.irq_and_dma.nes pass...

// eli - made this useful
[[noreturn]] void jam_handler() {
	std::cerr
		<< "CPU JAM abort"
		<< " PC=$" << std::hex << static_cast<unsigned>(PC.raw)
		<< " instruction=$" << static_cast<unsigned>(instruction_)
		<< " cycle=" << std::dec << cycle_
		<< " A=$" << std::hex << static_cast<unsigned>(A)
		<< " X=$" << static_cast<unsigned>(X)
		<< " Y=$" << static_cast<unsigned>(Y)
		<< " S=$" << static_cast<unsigned>(S)
		<< " P=$" << static_cast<unsigned>(P)
		<< " cycles=" << std::dec << executed_cycles_
		<< std::endl;

	abort();
}


// -----------------------------------------------------------------------------
// record_cpu_trace_entry
//
// Records the CPU state at the start of an instruction.  The entry stores the
// instruction address, opcode bytes, current registers, and cycle count.  The
// trace buffer is circular, keeping the most recent CPU_TRACE_CAPACITY entries.
//
// Parameters:
//   address - CPU address of the instruction about to execute.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
static void
record_cpu_trace_entry(uint16_t address)
{
	cpu_trace_entry_t& entry = sCPUTraceEntries[sCPUTraceNext];

	entry.cycle = executed_cycles_;
	entry.pc = address;

	entry.bytes[0] = nes::bus::debug_read_memory(address);
	entry.bytes[1] = nes::bus::debug_read_memory(static_cast<uint16_t>(address + 1));
	entry.bytes[2] = nes::bus::debug_read_memory(static_cast<uint16_t>(address + 2));

	entry.length = 1;

	entry.a = A;
	entry.x = X;
	entry.y = Y;
	entry.s = S;
	entry.p = P;

	sCPUTraceNext = (sCPUTraceNext + 1) % CPU_TRACE_CAPACITY;

	if (sCPUTraceCount < CPU_TRACE_CAPACITY) {
		sCPUTraceCount++;
	}
}


// -----------------------------------------------------------------------------
// check_debug_stack_break
//
// Checks debugger stack break conditions at a CPU instruction boundary.
//
// Stack-wrap detection compares S at consecutive instruction boundaries.
// A large transition normally indicates modulo-256 stack wrapping.
//
// Direct assignments to S, such as TXS, are excluded from wrap detection
// because they can legitimately move S by a large amount without performing
// stack push/pop activity.
//
// Interrupt entry is not excluded because it legitimately modifies the stack
// and may itself cause the hardware stack pointer to wrap.
//
// The SP-threshold breakpoint remains a one-shot downward-crossing condition.
//
// Parameters:
//   None.
//
// Returns:
//   true if a stack break condition was triggered.
// -----------------------------------------------------------------------------
static bool
check_debug_stack_break()
{
	if (sDebugStackWrapBreakEnabled && sDebugHavePreviousBoundaryS) {
		const int32_t delta = static_cast<int32_t>(S) - static_cast<int32_t>(sDebugPreviousBoundaryS);
		const bool largeTransition = delta < -128 || delta > 128;

		/*
		 * TXS ($9A) directly loads S from X.  Such a change is not a
		 * stack wrap even if the numerical SP difference is large.
		 *
		 * $9B (XAS/TAS) can also directly alter S on unofficial-opcode
		 * code paths, so exclude it as well.
		 *
		 * Do not apply these opcode exclusions when the previous CPU
		 * operation was actually interrupt entry.
		 */
		const bool directSPAssignment = !sDebugPreviousBoundaryWasInterrupt
				&& (sDebugPreviousBoundaryOpcode == 0x9a || sDebugPreviousBoundaryOpcode == 0x9b);

		if (largeTransition && !directSPAssignment) {
			sDebugStackBreakOldS = sDebugPreviousBoundaryS;
			sDebugStackBreakNewS = S;

			sDebugBreakpointHit = true;
			sDebugBreakpointHitAddress = PC.raw;
			sDebugBreakReason = DEBUG_BREAK_STACK_WRAP;

			return true;
		}
	}

	/*
	 * Wait until S has first moved above the SP-break threshold.
	 */
	if (sDebugStackSPBreakEnabled && sDebugStackSPBreakArmed && !sDebugStackSPBreakReady) {
		if (S > sDebugStackSPBreakThreshold) {
			sDebugStackSPBreakReady = true;
		}

		return false;
	}

	/*
	 * Once ready, trigger only on a downward crossing through the
	 * configured SP-break threshold.
	 */
	if (sDebugStackSPBreakEnabled && sDebugStackSPBreakArmed && sDebugStackSPBreakReady
		&& sDebugHavePreviousBoundaryS && sDebugPreviousBoundaryS > sDebugStackSPBreakThreshold
		&& S <= sDebugStackSPBreakThreshold) {
		sDebugStackBreakOldS = sDebugPreviousBoundaryS;
		sDebugStackBreakNewS = S;

		sDebugBreakpointHit = true;
		sDebugBreakpointHitAddress = PC.raw;
		sDebugBreakReason = DEBUG_BREAK_STACK_SP;

		sDebugStackSPBreakArmed = false;
		sDebugStackSPBreakReady = false;

		return true;
	}

	return false;
}


/**
 * @brief sync_handler
 */
void sync_handler() {
	return nes::cart.mapper()->cpu_sync();
}

/**
 * @brief set_flag - sets the given flag to true
 */
template <uint8_t M>
void set_flag() {
	P |= M;
}

/**
 * @brief clear_flag - sets the given flag to false
 */
template <uint8_t M>
void clear_flag() {
	P &= ~M;
}

/**
 * @brief set_flag_condition - sets the given flag based on the given condition
 * @param cond
 */
template <uint8_t M>
void set_flag_condition(bool cond) {
	if (cond) {
		set_flag<M>();
	} else {
		clear_flag<M>();
	}
}

/**
 * @brief update_nz_flags - sets the Negative and Zero flags based on the value given
 * @param value
 */
void update_nz_flags(uint8_t value) {

	// basically no bits set = 0x02
	// high bit set = 0x80
	// else = 0x00

	P &= ~(N_MASK | Z_MASK);
	P |= flag_table_[value];
}

// opcode implementation
#include "memory.h"
#include "opcodes.h"

void cycle_0(uint8_t next_op) {
	// first cycle is always instruction fetch
	// or do we force an interrupt?

	if (rst_executing_) {
		instruction_ = 0x100;
	} else if (nmi_executing_) {
		instruction_ = 0x101;
	} else if (irq_executing_) {
		instruction_ = 0x102;
	} else {
		instruction_ = next_op;
		++PC.raw;
	}
}

#include "address_modes.h"

/**
 * @brief execute_opcode
 */
void execute_opcode() {

	using fptr_t = void (*)();

	// instruction dispatch table
	static const fptr_t table[] = {
		opcode_brk::execute,
		indexed_indirect<opcode_ora>::execute,
		opcode_jam::execute,
		indexed_indirect<opcode_slo>::execute,
		zero_page<opcode_nop>::execute,
		zero_page<opcode_ora>::execute,
		zero_page<opcode_asl>::execute,
		zero_page<opcode_slo>::execute,
		stack<opcode_php>::execute,
		immediate<opcode_ora>::execute,
		accumulator<opcode_asl>::execute,
		immediate<opcode_aac>::execute,
		absolute<opcode_nop>::execute,
		absolute<opcode_ora>::execute,
		absolute<opcode_asl>::execute,
		absolute<opcode_slo>::execute,
		relative<opcode_bpl>::execute,
		indirect_indexed<opcode_ora>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_slo>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_ora>::execute,
		zero_page_x<opcode_asl>::execute,
		zero_page_x<opcode_slo>::execute,
		implied<opcode_clc>::execute,
		absolute_y<opcode_ora>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_slo>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_ora>::execute,
		absolute_x<opcode_asl>::execute,
		absolute_x<opcode_slo>::execute,
		opcode_jsr::execute,
		indexed_indirect<opcode_and>::execute,
		opcode_jam::execute,
		indexed_indirect<opcode_rla>::execute,
		zero_page<opcode_bit>::execute,
		zero_page<opcode_and>::execute,
		zero_page<opcode_rol>::execute,
		zero_page<opcode_rla>::execute,
		stack<opcode_plp>::execute,
		immediate<opcode_and>::execute,
		accumulator<opcode_rol>::execute,
		immediate<opcode_aac>::execute,
		absolute<opcode_bit>::execute,
		absolute<opcode_and>::execute,
		absolute<opcode_rol>::execute,
		absolute<opcode_rla>::execute,
		relative<opcode_bmi>::execute,
		indirect_indexed<opcode_and>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_rla>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_and>::execute,
		zero_page_x<opcode_rol>::execute,
		zero_page_x<opcode_rla>::execute,
		implied<opcode_sec>::execute,
		absolute_y<opcode_and>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_rla>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_and>::execute,
		absolute_x<opcode_rol>::execute,
		absolute_x<opcode_rla>::execute,
		opcode_rti::execute,
		indexed_indirect<opcode_eor>::execute,
		opcode_jam::execute,
		indexed_indirect<opcode_sre>::execute,
		zero_page<opcode_nop>::execute,
		zero_page<opcode_eor>::execute,
		zero_page<opcode_lsr>::execute,
		zero_page<opcode_sre>::execute,
		stack<opcode_pha>::execute,
		immediate<opcode_eor>::execute,
		accumulator<opcode_lsr>::execute,
		immediate<opcode_asr>::execute,
		absolute<opcode_jmp>::execute,
		absolute<opcode_eor>::execute,
		absolute<opcode_lsr>::execute,
		absolute<opcode_sre>::execute,
		relative<opcode_bvc>::execute,
		indirect_indexed<opcode_eor>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_sre>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_eor>::execute,
		zero_page_x<opcode_lsr>::execute,
		zero_page_x<opcode_sre>::execute,
		implied<opcode_cli>::execute,
		absolute_y<opcode_eor>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_sre>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_eor>::execute,
		absolute_x<opcode_lsr>::execute,
		absolute_x<opcode_sre>::execute,
		opcode_rts::execute,
		indexed_indirect<opcode_adc>::execute,
		opcode_jam::execute,
		indexed_indirect<opcode_rra>::execute,
		zero_page<opcode_nop>::execute,
		zero_page<opcode_adc>::execute,
		zero_page<opcode_ror>::execute,
		zero_page<opcode_rra>::execute,
		stack<opcode_pla>::execute,
		immediate<opcode_adc>::execute,
		accumulator<opcode_ror>::execute,
		immediate<opcode_arr>::execute,
		indirect<opcode_jmp>::execute,
		absolute<opcode_adc>::execute,
		absolute<opcode_ror>::execute,
		absolute<opcode_rra>::execute,
		relative<opcode_bvs>::execute,
		indirect_indexed<opcode_adc>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_rra>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_adc>::execute,
		zero_page_x<opcode_ror>::execute,
		zero_page_x<opcode_rra>::execute,
		implied<opcode_sei>::execute,
		absolute_y<opcode_adc>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_rra>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_adc>::execute,
		absolute_x<opcode_ror>::execute,
		absolute_x<opcode_rra>::execute,
		immediate<opcode_nop>::execute,
		indexed_indirect<opcode_sta>::execute,
		immediate<opcode_nop>::execute,
		indexed_indirect<opcode_aax>::execute,
		zero_page<opcode_sty>::execute,
		zero_page<opcode_sta>::execute,
		zero_page<opcode_stx>::execute,
		zero_page<opcode_aax>::execute,
		implied<opcode_dey>::execute,
		immediate<opcode_nop>::execute,
		implied<opcode_txa>::execute,
		immediate<opcode_xaa>::execute,
		absolute<opcode_sty>::execute,
		absolute<opcode_sta>::execute,
		absolute<opcode_stx>::execute,
		absolute<opcode_aax>::execute,
		relative<opcode_bcc>::execute,
		indirect_indexed<opcode_sta>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_axa>::execute,
		zero_page_x<opcode_sty>::execute,
		zero_page_x<opcode_sta>::execute,
		zero_page_y<opcode_stx>::execute,
		zero_page_y<opcode_aax>::execute,
		implied<opcode_tya>::execute,
		absolute_y<opcode_sta>::execute,
		implied<opcode_txs>::execute,
		absolute_y<opcode_xas>::execute,
		absolute_x<opcode_sya>::execute,
		absolute_x<opcode_sta>::execute,
		absolute_y<opcode_sxa>::execute,
		absolute_y<opcode_axa>::execute,
		immediate<opcode_ldy>::execute,
		indexed_indirect<opcode_lda>::execute,
		immediate<opcode_ldx>::execute,
		indexed_indirect<opcode_lax>::execute,
		zero_page<opcode_ldy>::execute,
		zero_page<opcode_lda>::execute,
		zero_page<opcode_ldx>::execute,
		zero_page<opcode_lax>::execute,
		implied<opcode_tay>::execute,
		immediate<opcode_lda>::execute,
		implied<opcode_tax>::execute,
		immediate<opcode_lax>::execute,
		absolute<opcode_ldy>::execute,
		absolute<opcode_lda>::execute,
		absolute<opcode_ldx>::execute,
		absolute<opcode_lax>::execute,
		relative<opcode_bcs>::execute,
		indirect_indexed<opcode_lda>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_lax>::execute,
		zero_page_x<opcode_ldy>::execute,
		zero_page_x<opcode_lda>::execute,
		zero_page_y<opcode_ldx>::execute,
		zero_page_y<opcode_lax>::execute,
		implied<opcode_clv>::execute,
		absolute_y<opcode_lda>::execute,
		implied<opcode_tsx>::execute,
		absolute_y<opcode_lar>::execute,
		absolute_x<opcode_ldy>::execute,
		absolute_x<opcode_lda>::execute,
		absolute_y<opcode_ldx>::execute,
		absolute_y<opcode_lax>::execute,
		immediate<opcode_cpy>::execute,
		indexed_indirect<opcode_cmp>::execute,
		immediate<opcode_nop>::execute,
		indexed_indirect<opcode_dcp>::execute,
		zero_page<opcode_cpy>::execute,
		zero_page<opcode_cmp>::execute,
		zero_page<opcode_dec>::execute,
		zero_page<opcode_dcp>::execute,
		implied<opcode_iny>::execute,
		immediate<opcode_cmp>::execute,
		implied<opcode_dex>::execute,
		immediate<opcode_axs>::execute,
		absolute<opcode_cpy>::execute,
		absolute<opcode_cmp>::execute,
		absolute<opcode_dec>::execute,
		absolute<opcode_dcp>::execute,
		relative<opcode_bne>::execute,
		indirect_indexed<opcode_cmp>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_dcp>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_cmp>::execute,
		zero_page_x<opcode_dec>::execute,
		zero_page_x<opcode_dcp>::execute,
		implied<opcode_cld>::execute,
		absolute_y<opcode_cmp>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_dcp>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_cmp>::execute,
		absolute_x<opcode_dec>::execute,
		absolute_x<opcode_dcp>::execute,
		immediate<opcode_cpx>::execute,
		indexed_indirect<opcode_sbc>::execute,
		immediate<opcode_nop>::execute,
		indexed_indirect<opcode_isc>::execute,
		zero_page<opcode_cpx>::execute,
		zero_page<opcode_sbc>::execute,
		zero_page<opcode_inc>::execute,
		zero_page<opcode_isc>::execute,
		implied<opcode_inx>::execute,
		immediate<opcode_sbc>::execute,
		implied<opcode_nop>::execute,
		immediate<opcode_sbc>::execute,
		absolute<opcode_cpx>::execute,
		absolute<opcode_sbc>::execute,
		absolute<opcode_inc>::execute,
		absolute<opcode_isc>::execute,
		relative<opcode_beq>::execute,
		indirect_indexed<opcode_sbc>::execute,
		opcode_jam::execute,
		indirect_indexed<opcode_isc>::execute,
		zero_page_x<opcode_nop>::execute,
		zero_page_x<opcode_sbc>::execute,
		zero_page_x<opcode_inc>::execute,
		zero_page_x<opcode_isc>::execute,
		implied<opcode_sed>::execute,
		absolute_y<opcode_sbc>::execute,
		implied<opcode_nop>::execute,
		absolute_y<opcode_isc>::execute,
		absolute_x<opcode_nop>::execute,
		absolute_x<opcode_sbc>::execute,
		absolute_x<opcode_inc>::execute,
		absolute_x<opcode_isc>::execute,

		opcode_rst::execute,
		opcode_nmi::execute,
		opcode_irq::execute,
	};

	assert(instruction_ <= 0x102);
	table[instruction_]();
}

/**
 * @brief clock - steps the emulation 1 cycle
 */
void clock() {

	if (UNLIKELY(dmc_dma_count_)) {

		if (dmc_dma_delay_ != 0) {
			read_byte(dmc_dma_source_address_);
			--dmc_dma_delay_;
			return;
		}

		// the count will always be initially even (we multiply by 2)
		// so, this forces us to idle for 1 cycle if
		// the CPU starts DMA on an odd cycle
		// after that, they should stay in sync
		if ((dmc_dma_count_ & 1) != (executed_cycles_ & 1)) {
			read_byte(dmc_dma_source_address_);
			return;
		}

		if ((dmc_dma_count_ & 1) == 0) {
			// read cycle
			dmc_dma_byte_ = read_byte(dmc_dma_source_address_++);
		} else {
			// write cycle
			(*dmc_dma_handler_)(dmc_dma_byte_);
		}

		--dmc_dma_count_;

	} else if (UNLIKELY(spr_dma_count_)) {

		if (spr_dma_delay_ != 0) {
			read_byte(spr_dma_source_address_);
			--spr_dma_delay_;
			return;
		}

		// the count will always be initially even (we multiply by 2)
		// so, this forces us to idle for 1 cycle if
		// the CPU starts DMA on an odd cycle
		// after that, they should stay in sync
		if ((spr_dma_count_ & 1) != (executed_cycles_ & 1)) {
			read_byte(spr_dma_source_address_);
			return;
		}

		if ((executed_cycles_ & 1) == 0) {
			// read cycle
			spr_dma_byte_ = read_byte(spr_dma_source_address_++);
		} else {
			// write cycle
			(*spr_dma_handler_)(spr_dma_byte_);
		}

		--spr_dma_count_;
	} else {
		assert(cycle_ < 10);

		if (cycle_ == 0) {
			const uint8_t next_op = read_byte(PC.raw);
			cycle_0(next_op);
		} else {
			// execute the current part of the instruction
			execute_opcode();
		}

		++cycle_;
	}
}

}


// -----------------------------------------------------------------------------
// nes::cpu::tick
//
// Advances the CPU by one cycle.
//
// At instruction boundaries, debugger break conditions are checked before the
// next CPU operation executes.
//
// After breakpoint checks pass, debugger state records the current instruction
// address, stack pointer, and operation that is about to execute.
//
// The saved instruction address remains valid throughout the instruction and
// allows memory READ/WRITE watchpoints to report the instruction responsible
// for the access rather than the already-advanced PC.
//
// On the next instruction boundary, the saved stack information allows
// stack-wrap detection to determine whether a large S transition came from
// ordinary stack activity or from a direct stack-pointer assignment such as
// TXS.
//
// Interrupt-entry state is recorded separately so a coincidental opcode byte at
// PC cannot cause an interrupt-induced stack wrap to be mistaken for TXS/XAS.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
tick()
{
	if (cycle_ == 0) {
		if (sDebugBreakpointHit) {
			return;
		}

		bool skipBreakChecks = false;

		if (sDebugSkipBreakpointOnce) {
			sDebugSkipBreakpointOnce = false;
			skipBreakChecks = true;
		}

		if (!skipBreakChecks) {
			if (check_debug_stack_break()) {
				return;
			}

			if (sExecuteBreakpointAddress[PC.raw]) {
				sDebugBreakpointHit = true;
				sDebugBreakpointHitAddress = PC.raw;
				sDebugBreakReason = DEBUG_BREAK_EXECUTE;

				sExecuteBreakpointHitCount[PC.raw]++;

				return;
			}
		}

		/*
		 * Preserve the starting address of the instruction that is about
		 * to execute. PC may advance during opcode/address processing, but
		 * memory watchpoints need the original instruction address.
		 */
		sDebugCurrentInstructionAddress = PC.raw;

		sDebugPreviousBoundaryS = S;

		sDebugPreviousBoundaryWasInterrupt
			= nmi_executing_
				|| irq_executing_;

		sDebugPreviousBoundaryOpcode
			= nes::bus::debug_read_memory(
				PC.raw
			);

		sDebugHavePreviousBoundaryS = true;

		sExecutedInstructionAddress[PC.raw] = true;
		record_cpu_trace_entry(PC.raw);
	}

	clock();
	sync_handler();
	++executed_cycles_;
}


/**
 * @brief nmi
 */
void nmi() {
	nmi_asserted_ = true;
}


// -----------------------------------------------------------------------------
// nes::cpu::reset
//
// Requests a CPU reset and resets debugger state that depends on observing
// consecutive instruction boundaries.
//
// Stack-break configuration itself is preserved across reset.  Boundary-history
// state used by SP-threshold and stack-wrap detection is cleared so stale
// pre-reset CPU state cannot participate in a later breakpoint decision.
//
// If the SP-threshold breakpoint is enabled, it is rearmed but starts in the
// not-ready state.  The CPU must subsequently observe S above the threshold
// before a later downward crossing may trigger it.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
reset()
{
	if (!rst_executing_) {
		rst_asserted_ = true;
		irq_asserted_ = false;
		nmi_asserted_ = false;
		executed_cycles_ = 1;
	}

	/*
	 * Discard instruction-boundary history used by stack break detection.
	 */
	sDebugHavePreviousBoundaryS = false;
	sDebugPreviousBoundaryS = 0xff;
	sDebugPreviousBoundaryOpcode = 0x00;
	sDebugPreviousBoundaryWasInterrupt = false;

	/*
	 * Preserve the user's configured SP breakpoint, but begin a fresh
	 * one-shot detection cycle after reset.
	 */
	if (sDebugStackSPBreakEnabled) {
		sDebugStackSPBreakArmed = true;
		sDebugStackSPBreakReady = false;
	} else {
		sDebugStackSPBreakArmed = false;
		sDebugStackSPBreakReady = false;
	}

	debug_clear_instruction_trace();
}


/**
 * @brief clear_nmi
 */
void clear_nmi() {
	nmi_asserted_ = false;
}

// -----------------------------------------------------------------------------
// nes::cpu::stop
//
// Stops CPU execution and resets the processor's runtime state.
//
// CPU registers, interrupt state, DMA state, instruction-cycle state, and the
// debugger's current-instruction address are returned to their initial values.
//
// The current debugger breakpoint-hit latch is also cleared so stale break
// state cannot survive into a later CPU session.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
stop()
{
	A = 0;
	X = 0;
	Y = 0;
	S = 0;
	P = I_MASK | R_MASK;

	executed_cycles_ = 1;
	irq_asserted_    = false;
	nmi_asserted_    = false;
	rst_asserted_    = false;
	irq_executing_   = false;
	nmi_executing_   = false;
	rst_executing_   = true;
	cycle_           = 0;

	spr_dma_handler_        = nullptr;
	spr_dma_source_address_ = 0;
	spr_dma_count_          = 0;
	spr_dma_delay_          = 0;

	dmc_dma_handler_        = nullptr;
	dmc_dma_source_address_ = 0;
	dmc_dma_count_          = 0;
	dmc_dma_delay_          = 0;

	sDebugCurrentInstructionAddress = 0x0000;

	debug_clear_breakpoint_hit();
}


/**
 * @brief reset
 * @param reset_type
 */
void reset(Reset reset_type) {

	if (reset_type == Reset::Hard) {
		stop();
		nes::bus::trash_ram();
	}

	reset();
	std::cout << "CPU reset complete" << std::endl;
}

/**
 * @brief irq
 * @param source
 */
void irq(IrqSource source) {

	irq_sources_ |= source;

	if (irq_sources_) {
		irq_asserted_ = true;
	}
}

/**
 * @brief clear_irq
 * @param source
 */
void clear_irq(IrqSource source) {

	irq_sources_ &= ~source;

	if (!irq_sources_) {
		irq_asserted_ = false;
	}
}

/**
 * @brief schedule_spr_dma
 * @param dma_handler
 * @param source_address
 * @param count
 */
void schedule_spr_dma(dma_handler_t dma_handler, uint_least16_t source_address, uint_least16_t count) {
	spr_dma_handler_        = dma_handler;
	spr_dma_source_address_ = source_address;
	spr_dma_count_          = count * 2;
	spr_dma_delay_          = 1;
}

/**
 * @brief schedule_dmc_dma
 * @param dma_handler
 * @param source_address
 * @param count
 */
void schedule_dmc_dma(dma_handler_t dma_handler, uint_least16_t source_address, uint_least16_t count) {
	dmc_dma_handler_        = dma_handler;
	dmc_dma_source_address_ = source_address;
	dmc_dma_count_          = count * 2;
	dmc_dma_delay_          = 0;
}

/**
 * @brief cycle_count
 * @return
 */
uint64_t cycle_count() {
	return executed_cycles_;
}


cpu_state_t
debug_cpu_state() {
	cpu_state_t state{};

	state.pc = PC.raw;
	state.a = A;
	state.x = X;
	state.y = Y;
	state.s = S;
	state.p = P;
	state.instruction = instruction_;
	state.cycle = cycle_;
	state.executed_cycles = executed_cycles_;

	return state;
}


bool
debug_instruction_boundary()
{
	return cycle_ == 0;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_instruction_was_executed
//
// Returns whether the supplied CPU address has been observed as the start of an
// executed instruction since the trace was last cleared.
//
// Parameters:
//   address - CPU address to query.
//
// Returns:
//   true if the address has been executed as an instruction start.
// -----------------------------------------------------------------------------
bool
debug_instruction_was_executed(uint16_t address)
{
	return sExecutedInstructionAddress[address];
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_instruction_trace
//
// Clears the executed-instruction address trace.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_instruction_trace()
{
	std::fill(sExecutedInstructionAddress, (sExecutedInstructionAddress + 0x10000), false);
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_add_execute_breakpoint
//
// Enables an execute breakpoint at a CPU address.  The breakpoint is checked
// when the CPU is about to fetch an opcode at instruction cycle 0.
//
// Parameters:
//   address - CPU address to break on when used as an instruction start.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_add_execute_breakpoint(uint16_t address)
{
	sExecuteBreakpointAddress[address] = true;
}


void
debug_remove_execute_breakpoint(uint16_t address)
{
	sExecuteBreakpointAddress[address] = false;
	sExecuteBreakpointHitCount[address] = 0;

	if ((sDebugBreakpointHit && sDebugBreakpointHitAddress) == address) {
		debug_clear_breakpoint_hit();
	}
}


void
debug_clear_execute_breakpoints()
{
	std::fill(sExecuteBreakpointAddress, (sExecuteBreakpointAddress + 0x10000), false);

	debug_clear_breakpoint_hit();
	debug_clear_breakpoint_hit_counts();
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_has_execute_breakpoint
//
// Returns whether an execute breakpoint is enabled at a CPU address.
//
// Parameters:
//   address - CPU address to query.
//
// Returns:
//   true if an execute breakpoint exists at address.
// -----------------------------------------------------------------------------
bool
debug_has_execute_breakpoint(uint16_t address)
{
	return sExecuteBreakpointAddress[address];
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_breakpoint_hit
//
// Returns whether the CPU has reached an enabled execute breakpoint since the
// hit state was last cleared.
//
// Parameters:
//   None.
//
// Returns:
//   true if an execute breakpoint has been hit.
// -----------------------------------------------------------------------------
bool
debug_breakpoint_hit()
{
	return sDebugBreakpointHit;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_breakpoint_hit_address
//
// Returns the CPU address of the most recent execute breakpoint hit.
//
// Parameters:
//   None.
//
// Returns:
//   CPU address for the most recent breakpoint hit.
// -----------------------------------------------------------------------------
uint16_t
debug_breakpoint_hit_address()
{
	return sDebugBreakpointHitAddress;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_break_reason
//
// Returns the reason associated with the currently latched debugger breakpoint.
//
// Parameters:
//   None.
//
// Returns:
//   DebugBreakReason describing why execution stopped.
// -----------------------------------------------------------------------------
DebugBreakReason
debug_break_reason()
{
	return sDebugBreakReason;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_breakpoint_hit
//
// Clears the current debugger breakpoint-hit latch and its associated state.
//
// This does not remove execute BreakPoints, memory watchpoints, or configured
// stack BreakPoint conditions.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_breakpoint_hit()
{
	sDebugBreakpointHit = false;
	sDebugBreakpointHitAddress = 0x0000;
	sDebugMemoryBreakAddress = 0x0000;
	sDebugBreakReason = DEBUG_BREAK_NONE;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_skip_breakpoint_once
//
// Skips execute-breakpoint checking for the next instruction boundary.  This is
// used when resuming from a breakpoint so execution can advance past the
// breakpoint instruction instead of immediately re-triggering the same hit.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_skip_breakpoint_once()
{
	sDebugSkipBreakpointOnce = true;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_resume_past_breakpoint
//
// Clears the currently latched debugger stop and prepares CPU execution to
// continue.
//
// Execute and stack BreakPoints occur at an instruction boundary.  Resuming from
// those conditions skips debugger break checks once so execution can move past
// the condition without immediately re-triggering it.
//
// Memory READ/WRITE watchpoints occur during an instruction after execution has
// already begun.  Their instruction has therefore already advanced far enough
// to perform the watched access, so the following instruction boundary must not
// be skipped.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_resume_past_breakpoint()
{
	if (!sDebugBreakpointHit) {
		return;
	}

	switch (sDebugBreakReason) {
		case DEBUG_BREAK_EXECUTE:
		case DEBUG_BREAK_STACK_SP:
		case DEBUG_BREAK_STACK_WRAP:
			/*
			 * These conditions stop at an instruction boundary.  Skip the
			 * debugger checks once so execution can move beyond the condition
			 * that caused the stop.
			 */
			sDebugSkipBreakpointOnce = true;
			break;

		case DEBUG_BREAK_MEMORY_READ:
		case DEBUG_BREAK_MEMORY_WRITE:
			/*
			 * A memory watchpoint fired during an instruction.  Do not skip
			 * the next instruction boundary; debugger instruction tracking
			 * must immediately resume there.
			 */
			sDebugSkipBreakpointOnce = false;
			break;

		case DEBUG_BREAK_NONE:
		default:
			sDebugSkipBreakpointOnce = false;
			break;
	}

	debug_clear_breakpoint_hit();
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_breakpoint_hit_count
//
// Returns the number of times an execute breakpoint has been hit at the supplied
// CPU address since the count was last cleared.
//
// Parameters:
//   address - CPU address to query.
//
// Returns:
//   Number of breakpoint hits recorded for address.
// -----------------------------------------------------------------------------
uint32_t
debug_breakpoint_hit_count(uint16_t address)
{
	return sExecuteBreakpointHitCount[address];
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_breakpoint_hit_counts
//
// Clears all execute-breakpoint hit counters.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_breakpoint_hit_counts()
{
	std::fill(sExecuteBreakpointHitCount, (sExecuteBreakpointHitCount + 0x10000), 0);
}

// -----------------------------------------------------------------------------
// nes::cpu::debug_cpu_trace_count
//
// Returns the number of valid entries currently stored in the CPU execution
// trace buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Number of valid trace entries.
// -----------------------------------------------------------------------------
uint32_t
debug_cpu_trace_count()
{
	return sCPUTraceCount;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_cpu_trace_capacity
//
// Returns the maximum number of entries the CPU execution trace buffer can hold.
//
// Parameters:
//   None.
//
// Returns:
//   Trace buffer capacity.
// -----------------------------------------------------------------------------
uint32_t
debug_cpu_trace_capacity()
{
	return CPU_TRACE_CAPACITY;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_cpu_trace_entry
//
// Reads one CPU execution trace entry by chronological index.  Index 0 is the
// oldest valid entry currently retained.  The newest entry is at
// debug_cpu_trace_count() - 1.
//
// Parameters:
//   index - Chronological trace index to read.
//   entry - Receives the trace entry.
//
// Returns:
//   true if the entry was read.
// -----------------------------------------------------------------------------
bool
debug_cpu_trace_entry(uint32_t index, cpu_trace_entry_t& entry)
{
	if (index >= sCPUTraceCount) {
		return false;
	}

	uint32_t physicalIndex = 0;

	if (sCPUTraceCount < CPU_TRACE_CAPACITY) {
		physicalIndex = index;
	} else {
		physicalIndex = (sCPUTraceNext + index) % CPU_TRACE_CAPACITY;
	}

	entry = sCPUTraceEntries[physicalIndex];
	return true;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_cpu_trace
//
// Clears the CPU execution trace buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_cpu_trace()
{
	sCPUTraceNext = 0;
	sCPUTraceCount = 0;

	for (uint32_t i = 0; i < CPU_TRACE_CAPACITY; i++) {
		sCPUTraceEntries[i] = cpu_trace_entry_t();
	}
}


uint8_t debug_s() {
	return S;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_break_old_s
//
// Returns the stack-pointer value immediately before the most recent stack
// breakpoint transition.
//
// Parameters:
//   None.
//
// Returns:
//   Previous stack-pointer value.
// -----------------------------------------------------------------------------
uint8_t
debug_stack_break_old_s()
{
	return sDebugStackBreakOldS;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_break_new_s
//
// Returns the stack-pointer value at the instruction boundary where the most
// recent stack breakpoint fired.
//
// Parameters:
//   None.
//
// Returns:
//   Stack-pointer value that caused the breakpoint.
// -----------------------------------------------------------------------------
uint8_t
debug_stack_break_new_s()
{
	return sDebugStackBreakNewS;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_set_stack_sp_break
//
// Enables or disables the stack-pointer threshold breakpoint and configures its
// threshold.
//
// Enabling the breakpoint always starts a fresh armed cycle.  It is initially
// not ready to fire; the CPU must subsequently observe S above the threshold
// before a later downward crossing can trigger the breakpoint.
//
// Parameters:
//   enabled   - true to enable SP-threshold breaking.
//   threshold - Stack-pointer threshold to watch.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_set_stack_sp_break(bool enabled, uint8_t threshold)
{
	sDebugStackSPBreakEnabled = enabled;
	sDebugStackSPBreakThreshold = threshold;
	sDebugStackSPBreakArmed = enabled;

	/*
	 * A newly enabled or reconfigured breakpoint must first observe
	 * the stack above the threshold before it can trigger.
	 */
	sDebugStackSPBreakReady = false;

	/*
	 * Discard the previous SP boundary sample so stack state observed
	 * before this configuration change cannot participate in the next
	 * threshold-crossing test.
	 */
	sDebugHavePreviousBoundaryS = false;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_sp_break_enabled
//
// Returns whether stack-pointer threshold breaking is currently enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true if the SP-threshold break condition is enabled.
// -----------------------------------------------------------------------------
bool
debug_stack_sp_break_enabled()
{
	return sDebugStackSPBreakEnabled;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_sp_break_threshold
//
// Returns the currently configured stack-pointer break threshold.
//
// Parameters:
//   None.
//
// Returns:
//   Current SP threshold.
// -----------------------------------------------------------------------------
uint8_t
debug_stack_sp_break_threshold()
{
	return sDebugStackSPBreakThreshold;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_set_stack_wrap_break
//
// Enables or disables debugger breaking when a large stack-pointer transition
// indicates a possible stack wrap.
//
// Parameters:
//   enabled - true to enable stack-wrap breaking.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_set_stack_wrap_break(bool enabled)
{
	sDebugStackWrapBreakEnabled = enabled;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_wrap_break_enabled
//
// Returns whether stack-wrap debugger breaking is currently enabled.
//
// Parameters:
//   None.
//
// Returns:
//   true if stack-wrap breaking is enabled.
// -----------------------------------------------------------------------------
bool
debug_stack_wrap_break_enabled()
{
	return sDebugStackWrapBreakEnabled;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_stack_sp_break_armed
//
// Returns whether the one-shot stack-pointer threshold breakpoint is currently
// armed and waiting to fire.
//
// Parameters:
//   None.
//
// Returns:
//   true if the SP-threshold breakpoint is armed.
// -----------------------------------------------------------------------------
bool
debug_stack_sp_break_armed()
{
	return sDebugStackSPBreakArmed;
}

// -----------------------------------------------------------------------------
// nes::cpu::debug_add_read_watchpoint
//
// Enables a CPU-memory read watchpoint at an address.
//
// Parameters:
//   address - CPU address whose reads should stop execution.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_add_read_watchpoint(uint16_t address)
{
	sDebugReadWatchpointAddress[address] = true;
	sDebugReadWatchpointHitCount[address] = 0;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_remove_read_watchpoint
//
// Removes a CPU-memory read watchpoint.
//
// Parameters:
//   address - CPU address whose read watchpoint should be removed.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_remove_read_watchpoint(uint16_t address)
{
	sDebugReadWatchpointAddress[address] = false;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_read_watchpoints
//
// Removes all CPU-memory read watchpoints.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_read_watchpoints()
{
	std::fill(sDebugReadWatchpointAddress, (sDebugReadWatchpointAddress + 0x10000), false);
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_has_read_watchpoint
//
// Returns whether a CPU-memory read watchpoint exists at an address.
//
// Parameters:
//   address - CPU address to query.
//
// Returns:
//   true if reads from the address should break execution.
// -----------------------------------------------------------------------------
bool
debug_has_read_watchpoint(uint16_t address)
{
	return sDebugReadWatchpointAddress[address];
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_add_write_watchpoint
//
// Enables a CPU-memory write watchpoint at an address.
//
// Parameters:
//   address - CPU address whose writes should stop execution.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_add_write_watchpoint(uint16_t address)
{
	sDebugWriteWatchpointAddress[address] = true;
	sDebugWriteWatchpointHitCount[address] = 0;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_remove_write_watchpoint
//
// Removes a CPU-memory write watchpoint.
//
// Parameters:
//   address - CPU address whose write watchpoint should be removed.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_remove_write_watchpoint(uint16_t address)
{
	sDebugWriteWatchpointAddress[address] = false;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_clear_write_watchpoints
//
// Removes all CPU-memory write watchpoints.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_write_watchpoints()
{
	std::fill(sDebugWriteWatchpointAddress, (sDebugWriteWatchpointAddress + 0x10000), false);
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_has_write_watchpoint
//
// Returns whether a CPU-memory write watchpoint exists at an address.
//
// Parameters:
//   address - CPU address to query.
//
// Returns:
//   true if writes to the address should break execution.
// -----------------------------------------------------------------------------
bool
debug_has_write_watchpoint(uint16_t address)
{
	return sDebugWriteWatchpointAddress[address];
}


// -----------------------------------------------------------------------------
// debug_read_watchpoint_hit_count
//
// Returns the number of times the specified READ watchpoint has been hit.
//
// Parameters:
//   address - CPU memory address to query.
//
// Returns:
//   Number of recorded READ-watchpoint hits for the address.
// -----------------------------------------------------------------------------
uint32_t
debug_read_watchpoint_hit_count(uint16_t address)
{
	return sDebugReadWatchpointHitCount[address];
}


// -----------------------------------------------------------------------------
// debug_write_watchpoint_hit_count
//
// Returns the number of times the specified WRITE watchpoint has been hit.
//
// Parameters:
//   address - CPU memory address to query.
//
// Returns:
//   Number of recorded WRITE-watchpoint hits for the address.
// -----------------------------------------------------------------------------
uint32_t
debug_write_watchpoint_hit_count(uint16_t address)
{
	return sDebugWriteWatchpointHitCount[address];
}


// -----------------------------------------------------------------------------
// debug_clear_read_watchpoint_hit_count
//
// Clears the accumulated READ-watchpoint hit count for one address.
//
// Parameters:
//   address - CPU memory address whose counter should be cleared.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_read_watchpoint_hit_count(uint16_t address)
{
	sDebugReadWatchpointHitCount[address] = 0;
}


// -----------------------------------------------------------------------------
// debug_clear_write_watchpoint_hit_count
//
// Clears the accumulated WRITE-watchpoint hit count for one address.
//
// Parameters:
//   address - CPU memory address whose counter should be cleared.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_write_watchpoint_hit_count(uint16_t address)
{
	sDebugWriteWatchpointHitCount[address] = 0;
}


// -----------------------------------------------------------------------------
// debug_clear_all_read_watchpoint_hit_counts
//
// Clears the accumulated hit counts for all CPU READ watchpoint addresses.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_all_read_watchpoint_hit_counts()
{
	for (uint32_t address = 0x0000;
		address <= 0xffff;
		address++) {

		sDebugReadWatchpointHitCount[address] = 0;
	}
}


// -----------------------------------------------------------------------------
// debug_clear_all_write_watchpoint_hit_counts
//
// Clears the accumulated hit counts for all CPU WRITE watchpoint addresses.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_clear_all_write_watchpoint_hit_counts()
{
	for (uint32_t address = 0x0000;
		address <= 0xffff;
		address++) {

		sDebugWriteWatchpointHitCount[address] = 0;
	}
}


// -----------------------------------------------------------------------------
// debug_check_memory_read
//
// Checks whether a CPU memory READ should trigger a debugger watchpoint.
//
// If a READ watchpoint exists for the supplied address, its hit counter is
// incremented and the debugger break state is updated.
//
// The breakpoint hit address records the starting PC of the instruction that
// performed the memory access rather than the CPU's already-advanced PC.
//
// Parameters:
//   address - CPU memory address being read.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_check_memory_read(uint16_t address)
{
	if (!sDebugReadWatchpointAddress[address]) {
		return;
	}

	sDebugReadWatchpointHitCount[address]++;

	sDebugBreakpointHit = true;
	sDebugBreakpointHitAddress = sDebugCurrentInstructionAddress;
	sDebugMemoryBreakAddress = address;
	sDebugBreakReason = DEBUG_BREAK_MEMORY_READ;
}


// -----------------------------------------------------------------------------
// debug_check_memory_write
//
// Checks whether a CPU memory WRITE should trigger a debugger watchpoint.
//
// If a WRITE watchpoint exists for the supplied address, its hit counter is
// incremented and the debugger break state is updated.
//
// The breakpoint hit address records the starting PC of the instruction that
// performed the memory access rather than the CPU's already-advanced PC.
//
// Parameters:
//   address - CPU memory address being written.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_check_memory_write(uint16_t address)
{
	if (!sDebugWriteWatchpointAddress[address]) {
		return;
	}

	sDebugWriteWatchpointHitCount[address]++;

	sDebugBreakpointHit = true;
	sDebugBreakpointHitAddress = sDebugCurrentInstructionAddress;
	sDebugMemoryBreakAddress = address;
	sDebugBreakReason = DEBUG_BREAK_MEMORY_WRITE;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_memory_break_address
//sf
// Returns the CPU-memory address responsible for the current memory
// read/write debugger break.
//
// Parameters:
//   None.
//
// Returns:
//   CPU address associated with the most recent memory watchpoint hit.
// -----------------------------------------------------------------------------
uint16_t
debug_memory_break_address()
{
	return sDebugMemoryBreakAddress;
}


// -----------------------------------------------------------------------------
// nes::cpu::debug_reset_in_progress
//
// Returns whether the CPU is currently processing or waiting to process a
// reset sequence.
//
// This allows debugger views to distinguish a valid runtime PC from the
// temporary PC value present before the reset vector has been loaded.
//
// Parameters:
//   None.
//
// Returns:
//   true if reset is asserted or currently executing.
// -----------------------------------------------------------------------------
bool
debug_reset_in_progress()
{
	return rst_asserted_ || rst_executing_;
}


}

