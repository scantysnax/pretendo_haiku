#ifndef PPU_20080314_H_
#define PPU_20080314_H_

#include "Reset.h"
#include <cstdint>

namespace nes::ppu {

struct scanline_vblank {};
struct scanline_prerender {};
struct scanline_postrender {};
struct scanline_render {
    explicit scanline_render(uint32_t* p) : buffer(p) {}
    uint32_t* const buffer;
};

struct scroll_state_t {
    uint32_t v;
    uint32_t t;
    uint8_t x;
    uint8_t ctrl;
};


struct ppu_write_log_entry_t {
	uint64_t frame;
	uint16_t dot;
	uint16_t scanline;
	uint16_t address;
	uint8_t value;
	uint8_t write_index;
};

constexpr uint32_t PPU_WRITE_LOG_CAPACITY = 256;


void reset(nes::Reset reset_type);

void write2000(uint8_t);
void write2001(uint8_t);
void write2002(uint8_t);
void write2003(uint8_t);
void write2004(uint8_t);
void write2005(uint8_t);
void write2006(uint8_t);
void write2007(uint8_t);
void write4014(uint8_t);

uint8_t read2002();
uint8_t read2004();
uint8_t read2007();
uint8_t read200x();

/*
 * IMPORTANT:
 * execute_scanline is NOT a template in the public API anymore.
 * (template implementation stays in Ppu.cpp)
 */
void execute_scanline(const scanline_vblank& target);
void execute_scanline(const scanline_prerender& target);
void execute_scanline(const scanline_postrender& target);
void execute_scanline(const scanline_render& target);

// debug/introspection helpers
scroll_state_t scroll_state();
uint16_t vram_address();
uint16_t temp_address();
uint8_t  fine_x();
uint8_t  ppuctrl();
uint8_t  ppumask();
uint8_t  ppustatus();
uint16_t ppu_dot();
uint16_t ppu_scanline();
uint64_t ppu_frame_counter();


uint64_t       cycle_count();
uint_least16_t hpos();
uint_least16_t vpos();

uint8_t palette_ram(uint32_t address);
void    set_palette_ram(uint32_t address, uint8_t data);

uint8_t oam_ram(uint32_t address);
uint8_t oamaddr();

void log_ppu_write(uint16_t address, uint8_t value);
uint32_t ppu_write_log_count();
ppu_write_log_entry_t ppu_write_log_entry(uint32_t index);
void clear_ppu_write_log();

extern bool show_sprites;
extern bool system_paused;

} // namespace nes::ppu

#endif
