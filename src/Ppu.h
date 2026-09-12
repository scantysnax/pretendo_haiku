
#ifndef _PPU_H_
#define _PPU_H_

#include <cstdint>

#include "Reset.h"


namespace nes::ppu {


// Scanline-type tags used to select the appropriate PPU rendering path.
struct scanline_vblank {};
struct scanline_prerender {};
struct scanline_postrender {};

struct scanline_render {
	explicit scanline_render(uint32_t* p)
		: buffer(p)
	{
	}

	// Destination pixel buffer for a visible rendered scanline.
	uint32_t* const buffer;
};

// Snapshot of the PPU scrolling/address state.
struct scroll_state_t {
	// Current VRAM address ("v").
	uint32_t v;

	// Temporary VRAM address ("t").
	uint32_t t;

	// Fine X scroll ("x").
	uint8_t x;

	// Current PPU control-register value.
	uint8_t ctrl;
};


// Records one debugger-visible write to a PPU register or address.
struct ppu_write_log_entry_t {
	// Video frame in which the write occurred.
	uint64_t frame;

	// PPU timing position of the write.
	uint16_t dot;
	uint16_t scanline;

	// Written PPU register/address and value.
	uint16_t address;
	uint8_t value;

	// Monotonically increasing write sequence number.
	uint8_t write_index;

	/*
	 * $2005/$2006 shared write-latch state at the time of the write.
	 *
	 * 0 = first write
	 * 1 = second write
	 *
	 * This field is meaningful for PPUSCROLL and PPUADDR writes.
	 */
	uint8_t write_latch;
};


// Resets PPU state according to the requested reset type.
void reset(nes::Reset reset_type);

// CPU writes to PPU registers.
void write2000 (uint8_t);
void write2001 (uint8_t);
void write2002 (uint8_t);
void write2003 (uint8_t);
void write2004 (uint8_t);
void write2005 (uint8_t);
void write2006 (uint8_t);
void write2007 (uint8_t);

// CPU write to the OAM DMA register.
void write4014 (uint8_t);

// CPU reads from readable PPU registers.
uint8_t read2002();
uint8_t read2004();
uint8_t read2007();

// Handles reads from the mirrored $2000-$2007 PPU register range.
uint8_t read200x();


/*
 * Public scanline-execution interface.
 *
 * The templated implementation remains private to Ppu.cpp; callers use these
 * overloads so template details do not leak into the public API.
 */
bool execute_scanline (const scanline_vblank& target);
bool execute_scanline (const scanline_prerender& target);
bool execute_scanline (const scanline_postrender& target);
bool execute_scanline (const scanline_render& target);


// Returns a snapshot of the current PPU scrolling/address state.
scroll_state_t scroll_state();


// Current PPU register/address state.
uint16_t vram_address();
uint16_t temp_address();
uint8_t  fine_x();
uint8_t  ppuctrl();
uint8_t  ppumask();
uint8_t  ppustatus();


// Current PPU timing position.
uint16_t ppu_dot();
uint16_t ppu_scanline();
uint64_t ppu_frame_counter();


// Low-level PPU timing counters.
uint64_t       cycle_count();
uint_least16_t hpos();
uint_least16_t vpos();


// Direct debugger access to PPU palette RAM.
uint8_t palette_ram (uint32_t address);
void    set_palette_ram (uint32_t address, uint8_t data);


// Direct debugger access to primary OAM and its current address register.
uint8_t oam_ram (uint32_t address);
uint8_t oam_addr();


// Maximum number of PPU register writes retained in the debugger history.
constexpr uint32_t PPU_WRITE_LOG_CAPACITY = 256;


// Records one PPU register write in the rolling debugger history.
void log_ppu_write (uint16_t address, uint8_t value);


// Returns the number of currently retained PPU write-log entries.
uint32_t ppu_write_log_count();


// Returns one retained write-log entry by chronological index.
ppu_write_log_entry_t ppu_write_log_entry (uint32_t index);

// Clears all retained PPU write-log history.
void clear_ppu_write_log();

// Copies retained PPU writes into the supplied buffer, oldest to newest.
uint32_t ppu_write_log_snapshot (ppu_write_log_entry_t* entries, uint32_t capacity);

// Side-effect-free debugger read from PPU address space.
uint8_t debug_read_ppu_memory (uint16_t address);

// Advances the PPU by one dot for debugger stepping.
void debug_step_dot();

// Global debugger/display control flags.
extern bool show_sprites;
extern bool system_paused;


}


#endif	// _PPU_H_


