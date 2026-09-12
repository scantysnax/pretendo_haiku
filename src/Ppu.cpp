// Ppu.cpp (fixed)
// Key fixes:
//  - NO namespace blocks inside functions
//  - internal state stays in the anonymous namespace
//  - execute_scanline_impl<T>() stays in the anonymous namespace (cpp-only)
//  - public non-template overloads execute_scanline(...) live in namespace nes::ppu
//  - debug helpers (scroll_state/vram_address/temp_address/fine_x) live in namespace nes::ppu
//  - removed all template explicit-instantiation junk / accidental calls

#include "Ppu.h"
#include "Apu.h"
#include "Cart.h"
#include "Compiler.h"
#include "Cpu.h"
#include "Mapper.h"
#include "Nes.h"

#include <algorithm>
#include <cstring>
#include <iostream>

// #define SPRITE_ZERO_HACK

namespace nes::ppu {

// These are meant to be externally visible toggles.
bool show_sprites  = true;
bool system_paused = false;

} // namespace nes::ppu


namespace {

// PPUSTATUS ($2002) register state.
union ppu_status_t {
	uint8_t raw;

	// Bit 5: sprite overflow flag.
	BitField<uint8_t, 5> overflow;

	// Bit 6: sprite 0 hit flag.
	BitField<uint8_t, 6> sprite0;

	// Bit 7: vertical blank flag.
	BitField<uint8_t, 7> vblank;
};


// PPUCTRL ($2000) register state.
union ppu_ctrl_t {
	uint8_t raw;

	// Bits 0-1: base nametable selection.
	BitField<uint8_t, 0, 2> nametable;

	// Bit 2: VRAM address increment mode.
	BitField<uint8_t, 2> address_increment;

	// Bit 3: sprite pattern-table selection.
	BitField<uint8_t, 3> sprite_pattern_table;

	// Bit 4: background pattern-table selection.
	BitField<uint8_t, 4> background_pattern_table;

	// Bit 5: selects 8x16 sprites when set.
	BitField<uint8_t, 5> large_sprites;

	// Bit 6: PPU master/slave selection.
	BitField<uint8_t, 6> master;

	// Bit 7: enables NMI generation on vertical blank.
	BitField<uint8_t, 7> nmi_on_vblank;
};


// PPUMASK ($2001) register state.
union ppu_mask_t {
	uint8_t raw;

	// Bit 0: grayscale/monochrome display mode.
	BitField<uint8_t, 0> monochrome;

	// Bits 1-2: left-edge background/sprite visibility controls.
	BitField<uint8_t, 1> background_clipping;
	BitField<uint8_t, 2> sprite_clipping;

	// Bits 3-4: background and sprite rendering enables.
	BitField<uint8_t, 3> background_visible;
	BitField<uint8_t, 4> sprites_visible;

	// Bits 5-7: color-emphasis controls.
	BitField<uint8_t, 5, 3> intensity;

	// Meta-field covering both rendering-enable bits.
	BitField<uint8_t, 3, 2> screen_enabled;
};


// PPU timing constants.
constexpr auto kCyclesPerScanline = 341u;
constexpr auto kCPUAlignment      = 0u;

// PPUSTATUS ($2002) bit masks.
constexpr uint8_t kOverflowStatus	= 0b0010'0000;
constexpr uint8_t kSprite0Status  	= 0b0100'0000;
constexpr uint8_t kVBlankStatus	   	= 0b1000'0000;

// Sprite/OAM attribute bit masks.
constexpr uint8_t kOAMColor    = 0b0000'0011;

// Internal flag used to track sprite 0.
constexpr uint8_t kOAMZero     = 0b0001'0000;

constexpr uint8_t kOAMPriority = 0b0010'0000;
constexpr uint8_t kOAMHFlip    = 0b0100'0000;
constexpr uint8_t kOAMVFlip    = 0b1000'0000;


// Pattern-table plane selectors used during tile fetch/decode.
struct pattern_0_t {
	static constexpr int index      = 0;
	static constexpr uint8_t offset = 0;
};

struct pattern_1_t {
	static constexpr int index      = 1;
	static constexpr uint8_t offset = 8;
};

// Sprite-height helpers used when applying vertical tile flipping.
struct size_8px_t {
	static constexpr int flip_mask = 0b0000'0111;
};

struct size_16px_t {
	static constexpr int flip_mask = 0b0000'1111;
};


// Lookup table that reverses the bit order of every possible 8-bit value.
// Used when horizontally flipping pattern data without reversing bits at runtime.
const uint8_t reverse_bits[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};


// Initial PPU palette RAM contents used on power-up/hard reset.
const uint8_t powerup_palette[32] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08
};


// -----------------------------------------------------------------------------
// attribute_bits
//
// Extracts the two-bit background palette attribute selected by the current
// nametable VRAM address from an attribute-table byte.
//
// Parameters:
//   vram_address - Current nametable VRAM address.
//   attr_byte    - Attribute-table byte.
//
// Returns:
//   Two-bit palette attribute.
// -----------------------------------------------------------------------------
constexpr uint8_t 
attribute_bits (uint_least16_t vram_address, uint8_t attr_byte)
{
	return (attr_byte >> (((vram_address & 0x40) >> 4) | (vram_address & 0x02))) & 0x03;
}


// -----------------------------------------------------------------------------
// attribute_address
//
// Calculates the attribute-table address corresponding to a nametable VRAM
// address.
//
// Parameters:
//   vram_address - Current nametable VRAM address.
//
// Returns:
//   PPU attribute-table address.
// -----------------------------------------------------------------------------
constexpr uint_least16_t 
attribute_address (uint_least16_t vram_address)
{
	return 0x23c0 | (vram_address & 0x0c00) | ((vram_address >> 4) & 0x38) | ((vram_address >> 2) & 0x07);
}


// -----------------------------------------------------------------------------
// tile_address
//
// Calculates the nametable tile-index address corresponding to the current VRAM
// address.
//
// Parameters:
//   vram_address - Current PPU VRAM address.
//
// Returns:
//   Nametable tile-index address.
// -----------------------------------------------------------------------------
constexpr uint_least16_t
tile_address (uint_least16_t vram_address) 
{
	return 0x2000 | (vram_address & 0x0fff);
}


// Cached sprite metadata and the two decoded pattern-plane bytes used during rendering.
struct sprite_pattern_t {
	uint8_t x;
	uint8_t y;
	uint8_t index;
	uint8_t attr;
	uint8_t patterns[2];
};

//------------------------------------------------------------------------------
// Internal state (single definitions only)
//------------------------------------------------------------------------------
// Sprite pattern data for the sprites selected for the current scanline.
sprite_pattern_t sprite_patterns_[8];
uint8_t current_sprite_index_ = 0;

// Primary OAM and current OAM address ($2003).
uint8_t sprite_ram_[0x100] = {};
uint8_t sprite_address_    = 0; // OAMADDR

// Temporary sprite-evaluation/rendering state for the current scanline.
uint8_t  sprite_data_[32]       = {};
uint8_t  sprite_data_index_     = 0;
uint8_t  left_most_sprite_x_    = 0xff;
uint8_t  sprite_read_buffer_    = 0;
uint8_t  sprite_read_index_     = 0;
bool     current_is_sprite_0    = false;
uint8_t  visible_sprite_count_  = 0;


// Sprite evaluation state machine used while scanning OAM for visible sprites.
enum sprite_eval_state  {
	STATE_1_Y,
	STATE_1_I,
	STATE_1_A,
	STATE_1_X,
	STATE_3,
	STATE_4
} sprite_eval_state_ = STATE_1_Y;


// Palette RAM and global PPU timing state.
uint8_t        palette_[0x20]               = {};
uint64_t       ppu_cycle_                   = 0;
uint64_t       ppu_read_2002_cycle_         = 0;

// Background fetch pipeline and loopy scrolling registers.
uint_least16_t next_ppu_fetch_address_      = 0;
uint_least16_t pattern_queue_[2]            = {};
uint_least16_t attribute_queue_[2]          = {};
uint_least16_t nametable_                   = 0; // loopy t
uint_least16_t vram_address_                = 0; // loopy v
uint_least16_t hpos_                        = 0; // pixel/dot counter
uint_least16_t vpos_                        = 0; // scanline counter
uint8_t        next_pattern_[2]             = {};
uint_least16_t latch_                       = 0;
uint8_t        next_attribute_              = 0;
uint8_t        next_tile_index_             = 0;

// PPU register state.
ppu_ctrl_t    	ppu_control_                = {0};
ppu_mask_t     	ppu_mask_                   = {0};
uint8_t        	register_2007_buffer_       = 0;
ppu_status_t	status_                     = {0};
uint8_t        tile_offset_                 = 0; // loopy x
uint8_t        	monochrome_mask_            = 0xff;

// Frame tracking and resumable CPU-slot execution state.
static uint64_t frame_counter_              = 0;
static bool     sPendingCpuSlot             = false;

// Frame parity and PPU register write sequencing state.
bool odd_frame_   = false;
bool write_latch_ = false;
bool write_block_ = false;


// Circular log of recent CPU writes to PPU registers for debugger inspection.
static nes::ppu::ppu_write_log_entry_t write_log_[nes::ppu::PPU_WRITE_LOG_CAPACITY];
static uint32_t write_log_next_ = 0;
static uint32_t write_log_count_ = 0;
static uint8_t  write_log_write_index_ = 0;


// -----------------------------------------------------------------------------
// sprite_pattern_table
//
// Returns the base address of the sprite pattern table selected by PPUCTRL.
//
// Parameters:
//   None.
//
// Returns:
//   Sprite pattern-table base address.
// -----------------------------------------------------------------------------
uint_least16_t
sprite_pattern_table()
{
	return ppu_control_.sprite_pattern_table ? 0x1000 : 0x0000;
}


// -----------------------------------------------------------------------------
// background_pattern_table
//
// Returns the base address of the background pattern table selected by PPUCTRL.
//
// Parameters:
//   None.
//
// Returns:
//   Background pattern-table base address.
// -----------------------------------------------------------------------------
uint_least16_t 
background_pattern_table()
{
	return ppu_control_.background_pattern_table ? 0x1000 : 0x0000;
}


//------------------------------------------------------------------------------
// Sprite pattern address helpers
//------------------------------------------------------------------------------
template <class Pattern>
// -----------------------------------------------------------------------------
// sprite_pattern_address
//
// Calculates a CHR address for the requested sprite pattern plane, sprite
// size, tile index, and line.
//
// Parameters:
//   index       - Sprite tile index.
//   sprite_line - Row within the sprite.
//
// Returns:
//   PPU CHR address for the requested sprite pattern byte.
// -----------------------------------------------------------------------------
constexpr uint_least16_t 
sprite_pattern_address (uint8_t index, uint8_t sprite_line, const size_8px_t &) 
{
	return (sprite_pattern_table() | (index << 4) | Pattern::offset | sprite_line) & 0xffff;
}


// -----------------------------------------------------------------------------
// sprite_pattern_address
//
// Calculates a CHR address for the requested sprite pattern plane, sprite
// size, tile index, and line.
//
// Parameters:
//   index       - Sprite tile index.
//   sprite_line - Row within the sprite.
//
// Returns:
//   PPU CHR address for the requested sprite pattern byte.
// -----------------------------------------------------------------------------
template <class Pattern>
constexpr uint_least16_t 
sprite_pattern_address (uint8_t index, uint8_t sprite_line, const size_16px_t &) 
{
	return (((index & 1) << 12) | ((index & 0xfe) << 4) | Pattern::offset | (sprite_line & 7) |
	        ((sprite_line & 0x08) << 1)) & 0xffff;
}


// -----------------------------------------------------------------------------
// sprite_pattern_address
//
// Calculates a CHR address for the requested sprite pattern plane, sprite
// size, tile index, and line.
//
// Parameters:
//   index       - Sprite tile index.
//   sprite_line - Row within the sprite.
//
// Returns:
//   PPU CHR address for the requested sprite pattern byte.
// -----------------------------------------------------------------------------
template <class Size, class Pattern>
constexpr uint_least16_t 
sprite_pattern_address (uint8_t index, uint8_t sprite_line)
{
	return sprite_pattern_address<Pattern>(index, sprite_line, Size());
}


// -----------------------------------------------------------------------------
// render_blank_pixel
//
// Returns the palette value produced while normal rendering is disabled.
//
// Parameters:
//   None.
//
// Returns:
//   Current blank-rendering palette value.
// -----------------------------------------------------------------------------
uint8_t 
render_blank_pixel()
{
	if (UNLIKELY((vram_address_ & 0x3f00) == 0x3f00)) {
		return palette_[vram_address_ & 0x1f] & monochrome_mask_;
	}
	return palette_[0x00] & monochrome_mask_;
}


// -----------------------------------------------------------------------------
// select_bg_pixel
//
// Selects the background pixel for the requested horizontal position using the
// background pattern and attribute shift registers.
//
// Parameters:
//   index - Horizontal pixel index.
//
// Returns:
//   Encoded background pixel value.
// -----------------------------------------------------------------------------
uint8_t
select_bg_pixel(uint_least16_t index)
{
	if (LIKELY(index >= 8 || ppu_mask_.background_clipping) && ppu_mask_.background_visible) {
		const uint_least16_t mask = (0x8000 >> tile_offset_);

		return (((pattern_queue_[0] & mask) >> (15 - tile_offset_)) |
		        ((pattern_queue_[1] & mask) >> (14 - tile_offset_)) |
		        ((attribute_queue_[0] & mask) >> (13 - tile_offset_)) |
		        ((attribute_queue_[1] & mask) >> (12 - tile_offset_))) &
		       0xff;
	}

	return 0x00;
}


// -----------------------------------------------------------------------------
// select_pixel
//
// Combines background and sprite data for the requested horizontal position,
// including sprite priority, clipping, and sprite-zero-hit behavior.
//
// Parameters:
//   index - Horizontal pixel index.
//
// Returns:
//   Encoded final pixel value.
// -----------------------------------------------------------------------------
uint8_t 
select_pixel (uint_least16_t index)
{
	const uint8_t pixel = select_bg_pixel(index);

	if (LIKELY(index >= 8 || ppu_mask_.sprite_clipping) && ppu_mask_.sprites_visible) {

		for (uint8_t spr = 0; spr != visible_sprite_count_; ++spr) {
			const sprite_pattern_t &sprite = sprite_patterns_[spr];
			const uint16_t x_offset     = index - sprite.x;

			if (x_offset >= 8) {
				continue;
			}

			const uint8_t p0     = sprite.patterns[0];
			const uint8_t p1     = sprite.patterns[1];
			const uint16_t shift = 7 - x_offset;

			const uint8_t sprite_pixel = ((p0 >> shift) & 0x01) | (((p1 >> shift) << 1) & 0x02);

			if ((sprite_pixel & 0x03) == 0) {
				continue;
			}

#ifndef SPRITE_ZERO_HACK
			if ((sprite.attr & kOAMZero) && (index < 255) && (pixel & 0x03)) {
#else
			if ((sprite.attr & kOAMZero) && (index < 255)) {
#endif
				status_.sprite0 = true;
			}

			if (UNLIKELY(!nes::ppu::show_sprites)) {
				return pixel;
			}

			if ((((sprite.attr & kOAMPriority) == 0) || ((pixel & 0x03) == 0))) {
				return (0x10 | sprite_pixel | ((sprite.attr & kOAMColor) << 2)) & 0xff;
			}

			return pixel;
		}
	}

	return pixel;
}

// -----------------------------------------------------------------------------
// clock_x
//
// Advances the coarse horizontal loopy scroll position by one tile, including
// nametable wrapping at the right edge.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_x()
{
	if (UNLIKELY((vram_address_ & 0x1f) == 0x1f)) {
		vram_address_ ^= 0x41f;
	} else {
		++vram_address_;
	}
}


// -----------------------------------------------------------------------------
// clock_y
//
// Advances the fine/coarse vertical loopy scroll position according to NES PPU
// scrolling rules.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_y()
{
	if (UNLIKELY((vram_address_ & 0x7000) == 0x7000)) {

		vram_address_ &= 0x0fff;

		switch (vram_address_ & 0x03e0) {
		case 0x03a0:
			vram_address_ &= ~0x03e0;
			vram_address_ ^= 0x0800;
			break;
		case 0x03e0:
			vram_address_ &= ~0x03e0;
			break;
		default:
			vram_address_ += 0x20;
			break;
		}
	} else {
		vram_address_ += 0x1000;
	}
}


// -----------------------------------------------------------------------------
// open_background_pattern
//
// Prepares the mapper-visible CHR address for the next background pattern
// fetch.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
template <class Pattern>
void
open_background_pattern()
{
	const uint8_t tile_line = (vram_address_ & 0x7000) >> 12;
	next_ppu_fetch_address_ =
		(background_pattern_table() | (next_tile_index_ << 4) | Pattern::offset | tile_line) & 0xffff;
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// read_background_pattern
//
// Reads the currently opened background pattern byte into the pending pattern
// latch.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
template <class Pattern>
void
read_background_pattern() 
{
	next_pattern_[Pattern::index] = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// open_background_attribute
//
// Prepares the mapper-visible attribute-table address for the next background
// attribute fetch.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
open_background_attribute()
{
	next_ppu_fetch_address_ = attribute_address(vram_address_);
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// read_background_attribute
//
// Reads the current attribute byte and extracts the palette bits for the
// current background tile.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
read_background_attribute()
{
	const uint8_t attr_byte = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
	next_attribute_         = attribute_bits(vram_address_, attr_byte);
}


// -----------------------------------------------------------------------------
// open_tile_index
//
// Prepares the mapper-visible nametable address for the next tile-index fetch.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
open_tile_index()
{
	next_ppu_fetch_address_ = tile_address(vram_address_);
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// read_tile_index
//
// Reads the currently opened nametable byte into the pending tile-index latch.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
read_tile_index()
{
	next_tile_index_ = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// sprite_in_range
//
// Tests whether the supplied sprite Y coordinate intersects the scanline being
// evaluated.
//
// Parameters:
//   y - Sprite OAM Y coordinate.
//
// Returns:
//   true if the sprite intersects the current scanline.
// -----------------------------------------------------------------------------
bool
sprite_in_range (uint8_t y) {
	const uint_least16_t sprite_line = (vpos_ - 1) - y;
	return ppu_control_.large_sprites ? (sprite_line < 16) : (sprite_line < 8);
}


// Accessors for the four bytes of a sprite entry in the temporary sprite buffer.
uint8_t &sprite_y (uint8_t index)     { return sprite_data_[index * 4 + 0]; }
uint8_t &sprite_index (uint8_t index) { return sprite_data_[index * 4 + 1]; }
uint8_t &sprite_attr (uint8_t index)  { return sprite_data_[index * 4 + 2]; }
uint8_t &sprite_x (uint8_t index)     { return sprite_data_[index * 4 + 3]; }


// -----------------------------------------------------------------------------
// evaluate_sprites_even
//
// Performs the even-dot half of secondary-OAM clearing and sprite evaluation
// for the next scanline.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
evaluate_sprites_even()
{
	if (hpos_ <= 64) {
		sprite_data_[(hpos_ >> 1) - 1] = sprite_read_buffer_;

		if (UNLIKELY(hpos_ == 0)) {
			left_most_sprite_x_ = 0xff;
		}
	} else if (hpos_ <= 256) {
		switch (sprite_eval_state_) {
		case STATE_1_Y:
			if (sprite_data_index_ < 8) {
				if (sprite_in_range(sprite_read_buffer_)) {
					sprite_y(sprite_data_index_) = static_cast<uint8_t>((vpos_ - 1) - sprite_read_buffer_);
					sprite_eval_state_           = STATE_1_I;
					++sprite_read_index_;
					break;
				} else {
					sprite_read_index_ += 4;
					current_is_sprite_0 = false;

					if ((sprite_read_index_ & 0xfc) == 0x00) {
						sprite_eval_state_ = STATE_4;
						break;
					}

					if (sprite_data_index_ < 8) {
						sprite_eval_state_ = STATE_1_Y;
						break;
					}

					sprite_eval_state_ = STATE_3;
					break;
				}
			}
			break;

		case STATE_1_I:
			sprite_index(sprite_data_index_) = sprite_read_buffer_;
			sprite_eval_state_               = STATE_1_A;
			++sprite_read_index_;
			break;

		case STATE_1_A:
			sprite_attr(sprite_data_index_) = sprite_read_buffer_ & 0xe3;
			if (current_is_sprite_0) {
				sprite_attr(sprite_data_index_) |= kOAMZero;
			}
			sprite_eval_state_ = STATE_1_X;
			++sprite_read_index_;
			break;

		case STATE_1_X:
			sprite_x(sprite_data_index_) = sprite_read_buffer_;
			left_most_sprite_x_          = std::min(left_most_sprite_x_, sprite_x(sprite_data_index_));
			++sprite_data_index_;
			++sprite_read_index_;
			current_is_sprite_0 = false;

			if ((sprite_read_index_ & 0xfc) == 0x00) {
				sprite_eval_state_ = STATE_4;
				break;
			}

			if (sprite_data_index_ < 8) {
				sprite_eval_state_ = STATE_1_Y;
				break;
			}

			sprite_eval_state_ = STATE_3;
			break;

		case STATE_3: {
			if (sprite_in_range(sprite_read_buffer_)) {
				status_.overflow = true;
				++sprite_read_index_;
			} else {
				sprite_read_index_ = (sprite_read_index_ & 0x03) | (((sprite_read_index_ & 0xfc) + 4) & 0xfc);
				sprite_read_index_ = (sprite_read_index_ & 0xfc) | (((sprite_read_index_ & 0x03) + 1) & 0x03);
			}

			if ((sprite_read_index_ & 0xfc) == 0x00) {
				sprite_eval_state_ = STATE_4;
			}
		} break;

		case STATE_4:
			break;
		}

		if (hpos_ == 256) {
			visible_sprite_count_ = sprite_data_index_;
		}
	}
}


// -----------------------------------------------------------------------------
// evaluate_sprites_odd
//
// Performs the odd-dot half of sprite evaluation by reading primary OAM into
// the sprite-evaluation buffer.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
evaluate_sprites_odd()
{
	if (hpos_ < 64) {
		sprite_read_buffer_ = 0xff;
	} else if (hpos_ == 65) {
		sprite_read_index_  = sprite_address_;
		current_is_sprite_0 = true;
		sprite_eval_state_  = STATE_1_Y;
		sprite_data_index_  = 0;
		sprite_read_buffer_ = sprite_ram_[sprite_read_index_];
	} else if (hpos_ < 256) {
		sprite_read_buffer_ = sprite_ram_[sprite_read_index_];
	}
}


// -----------------------------------------------------------------------------
// enter_vblank
//
// Sets the vblank status flag unless a precisely timed PPUSTATUS read suppresses
// the transition.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
enter_vblank()
{
	if (UNLIKELY(ppu_cycle_ != (ppu_read_2002_cycle_ + 1))) {
		status_.vblank = true;
	}
}


// -----------------------------------------------------------------------------
// open_sprite_pattern
//
// Prepares the mapper-visible CHR address for one sprite pattern-plane fetch,
// including sprite size and vertical-flip handling.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
template <class Size, class Pattern>
void
open_sprite_pattern()
{
	current_sprite_index_ = ((hpos_ - 1) >> 3) & 0x07;
	sprite_pattern_t &sprite = sprite_patterns_[current_sprite_index_];

	sprite.y = sprite_y(current_sprite_index_);

	if (sprite.y != 0xff) {

		sprite.x     = sprite_x(current_sprite_index_);
		sprite.attr  = sprite_attr(current_sprite_index_);
		sprite.index = sprite_index(current_sprite_index_);

		if (sprite.attr & kOAMVFlip) {
			sprite.y ^= Size::flip_mask;
		}

		next_ppu_fetch_address_ = sprite_pattern_address<Size, Pattern>(sprite.index, sprite.y);
	} else {
		next_ppu_fetch_address_ = sprite_pattern_address<Size, Pattern>(0xff, 0xff);
	}

	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}


// -----------------------------------------------------------------------------
// read_sprite_pattern
//
// Reads one sprite pattern plane and applies horizontal bit reversal when the
// sprite is horizontally flipped.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
template <class Size, class Pattern>
void
read_sprite_pattern()
{
	uint8_t pattern = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);

	sprite_pattern_t &sprite = sprite_patterns_[current_sprite_index_];

	if (sprite.attr & kOAMHFlip) {
		pattern = reverse_bits[pattern];
	}

	sprite_patterns_[current_sprite_index_].patterns[Pattern::index] = pattern;
}


// -----------------------------------------------------------------------------
// render_pixel
//
// Renders the current visible pixel, advances the background shift registers, and
// resolves the selected palette entry.
//
// Parameters:
//   None.
//
// Returns:
//   Current NES palette value.
// -----------------------------------------------------------------------------
uint8_t 
render_pixel()
{
	const uint8_t pixel = select_pixel(hpos_ - 1);

	pattern_queue_[0] <<= 1;
	pattern_queue_[1] <<= 1;
	attribute_queue_[0] <<= 1;
	attribute_queue_[1] <<= 1;

	const uint8_t mask = ((pixel & 0x01) | ((pixel & 0x02) >> 1)) * 0xff;
	return palette_[pixel & mask] & monochrome_mask_;
}


// -----------------------------------------------------------------------------
// update_shift_registers_render
//
// Loads newly fetched background pattern and attribute data into the rendering
// shift registers.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
update_shift_registers_render() {
	pattern_queue_[0] |= next_pattern_[0];
	pattern_queue_[1] |= next_pattern_[1];
	attribute_queue_[0] |= ((next_attribute_ >> 0) & 0x01) * 0xff;
	attribute_queue_[1] |= ((next_attribute_ >> 1) & 0x01) * 0xff;
}


// -----------------------------------------------------------------------------
// update_shift_registers_idle
//
// Advances the background shift registers by one tile and loads newly fetched
// pattern and attribute data.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
update_shift_registers_idle() {
	pattern_queue_[0] <<= 8;
	pattern_queue_[1] <<= 8;
	attribute_queue_[0] <<= 8;
	attribute_queue_[1] <<= 8;
	update_shift_registers_render();
}


// -----------------------------------------------------------------------------
// update_x_scroll
//
// Copies the horizontal scroll bits from the temporary loopy register into the
// current VRAM address.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
update_x_scroll()
{
	vram_address_ = (vram_address_ & ~0b00000100'00011111) | (nametable_ & 0b00000100'00011111);
}


// -----------------------------------------------------------------------------
// update_sprite_registers
//
// Resets OAMADDR during the PPU sprite-fetch portion of a rendering scanline.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
update_sprite_registers() 
{
	sprite_address_ = 0;
}


// -----------------------------------------------------------------------------
// update_vram_address
//
// Copies the vertical scroll bits from the temporary loopy register into the
// current VRAM address.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
update_vram_address()
{
	vram_address_ = (vram_address_ & ~0b01111011'11100000) | (nametable_ & 0b01111011'11100000);
}


// -----------------------------------------------------------------------------
// rendering
//
// Reports whether the PPU is currently within the prerender/visible rendering
// scanline range.
//
// Parameters:
//   None.
//
// Returns:
//   true while the PPU is in the rendering range.
// -----------------------------------------------------------------------------
bool 
rendering()
{
	return vpos_ <= 240;
}


// -----------------------------------------------------------------------------
// increment_vram_address
//
// Advances the current VRAM address using rendering scroll rules or the
// PPUCTRL-selected linear increment when rendering is inactive.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
increment_vram_address()
{
	if (rendering() && ppu_mask_.screen_enabled) {
		if (ppu_control_.address_increment) {
			clock_y();
		} else {
			clock_x();
		}
	} else {
		vram_address_ += ppu_control_.address_increment ? 32 : 1;
	}
}


//------------------------------------------------------------------------------
// clock_ppu overloads
//------------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// clock_ppu
//
// Executes one PPU dot for the supplied scanline type, performing the rendering,
// fetch, scroll, sprite, or vblank work appropriate to that scanline.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_ppu (const nes::ppu::scanline_prerender &)
{
	if (UNLIKELY(hpos_ == 0)) {
		status_.sprite0  = 0;
		status_.overflow = 0;
	} else if (UNLIKELY(hpos_ == 1)) {
		status_.vblank = 0;
		write_block_   = false;
	}

	if (LIKELY(ppu_mask_.screen_enabled)) {
		if (hpos_ < 1) {
			// idle
		} else if (hpos_ < 257) {
			switch (hpos_ & 0x07) {
			case 1: evaluate_sprites_odd();  open_tile_index(); break;
			case 2: evaluate_sprites_even(); read_tile_index(); break;
			case 3: evaluate_sprites_odd();  open_background_attribute(); break;
			case 4: evaluate_sprites_even(); read_background_attribute(); break;
			case 5: evaluate_sprites_odd();  open_background_pattern<pattern_0_t>(); break;
			case 6: evaluate_sprites_even(); read_background_pattern<pattern_0_t>(); break;
			case 7: evaluate_sprites_odd();  open_background_pattern<pattern_1_t>(); break;
			case 0:
				evaluate_sprites_even();
				read_background_pattern<pattern_1_t>();
				update_shift_registers_idle();
				clock_x();
				break;
			}

			if (UNLIKELY(hpos_ == 256)) {
				clock_y();
			}
		} else if (hpos_ < 281) {

			if (UNLIKELY(hpos_ == 257)) {
				update_x_scroll();
			}

			update_sprite_registers();

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			}
		} else if (hpos_ < 305) {

			update_vram_address();
			update_sprite_registers();

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			}
		} else if (hpos_ < 321) {

			update_sprite_registers();

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			}
		} else if (hpos_ < 337) {
			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5: open_background_pattern<pattern_0_t>(); break;
			case 6: read_background_pattern<pattern_0_t>(); break;
			case 7: open_background_pattern<pattern_1_t>(); break;
			case 0:
				read_background_pattern<pattern_1_t>();
				update_shift_registers_idle();
				clock_x();
				break;
			}
		} else {
			switch (hpos_) {
			case 337: open_tile_index(); break;
			case 338: read_tile_index(); break;
			case 339:
				open_tile_index();
				if (odd_frame_) {
					++hpos_;
				}
				break;
			case 340: read_tile_index(); break;
			default: abort();
			}
		}
	}
}


// -----------------------------------------------------------------------------
// clock_ppu
//
// Executes one PPU dot for the supplied scanline type, performing the rendering,
// fetch, scroll, sprite, or vblank work appropriate to that scanline.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
clock_ppu (const nes::ppu::scanline_render &target) 
{
	if (UNLIKELY(!ppu_mask_.screen_enabled)) {

		if (hpos_ < 1) {
			// idle
		} else if (hpos_ < 257) {
			target.buffer[hpos_ - 1] = render_blank_pixel();
			target.buffer[hpos_ - 1] |= (ppu_mask_.intensity << 6);
		} else {
			// idle
		}
	} else {

		if (hpos_ < 1) {
			if (UNLIKELY(vpos_ == 1 && odd_frame_)) {
				read_tile_index();
			}
		} else if (hpos_ < 257) {

			target.buffer[hpos_ - 1] = render_pixel();
			target.buffer[hpos_ - 1] |= (ppu_mask_.intensity << 6);

			switch (hpos_ & 0x07) {
			case 1: evaluate_sprites_odd();  open_tile_index(); break;
			case 2: evaluate_sprites_even(); read_tile_index(); break;
			case 3: evaluate_sprites_odd();  open_background_attribute(); break;
			case 4: evaluate_sprites_even(); read_background_attribute(); break;
			case 5: evaluate_sprites_odd();  open_background_pattern<pattern_0_t>(); break;
			case 6: evaluate_sprites_even(); read_background_pattern<pattern_0_t>(); break;
			case 7: evaluate_sprites_odd();  open_background_pattern<pattern_1_t>(); break;
			case 0:
				evaluate_sprites_even();
				read_background_pattern<pattern_1_t>();
				update_shift_registers_render();
				clock_x();
				break;
			}

			if (UNLIKELY(hpos_ == 256)) {
				clock_y();
			}
		} else if (hpos_ < 321) {

			if (UNLIKELY(hpos_ == 257)) {
				update_x_scroll();
			}

			update_sprite_registers();

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_0_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_0_t>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           open_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px_t, pattern_1_t>();
				else                           read_sprite_pattern<size_8px_t,  pattern_1_t>();
				break;
			}
		} else if (hpos_ < 337) {

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5: open_background_pattern<pattern_0_t>(); break;
			case 6: read_background_pattern<pattern_0_t>(); break;
			case 7: open_background_pattern<pattern_1_t>(); break;
			case 0:
				read_background_pattern<pattern_1_t>();
				update_shift_registers_idle();
				clock_x();
				break;
			}
		} else {
			switch (hpos_) {
			case 337: open_tile_index(); break;
			case 338: read_tile_index(); break;
			case 339: open_tile_index(); break;
			case 340: read_tile_index(); break;
			default: abort();
			}
		}
	}
}


// -----------------------------------------------------------------------------
// clock_ppu
//
// Executes one PPU dot for the supplied scanline type, performing the rendering,
// fetch, scroll, sprite, or vblank work appropriate to that scanline.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
clock_ppu (const nes::ppu::scanline_postrender &) 
{
	// no-op
}


// -----------------------------------------------------------------------------
// clock_ppu
//
// Executes one PPU dot for the supplied scanline type, performing the rendering,
// fetch, scroll, sprite, or vblank work appropriate to that scanline.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clock_ppu (const nes::ppu::scanline_vblank &)
{
	// eli: really?
	// vpos_ == 242 is treated as "line 241 in theory"
	if (UNLIKELY(vpos_ == 242)) {
		switch (hpos_) {
		case 1:
			enter_vblank();
			break;
		case 3:
			if (ppu_control_.nmi_on_vblank && status_.vblank) {
				nes::cpu::nmi();
			}
			break;
		}
	}
}


// -----------------------------------------------------------------------------
// start_frame
//
// Begins a new PPU frame by advancing the frame counter, resetting the scanline
// position, and notifying the APU frame hook.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
start_frame()
{
	frame_counter_++;
	
	vpos_ = 0;
	nes::apu::start_frame();
}


// -----------------------------------------------------------------------------
// end_frame
//
// Completes the current PPU frame, toggles odd/even frame state, advances the
// I/O-latch decay value, and notifies the active mapper.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
end_frame()
{
	odd_frame_ = !odd_frame_;

	latch_ += 0x100;
	if (latch_ > 0x3c00) {
		latch_ = 0;
	}

	nes::cart.mapper()->ppu_end_frame();
}


// -----------------------------------------------------------------------------
// execute_scanline_impl
//
// Executes or resumes one PPU scanline.
//
// Unlike the original implementation, hpos_ is not reset when this function is
// entered.  This allows an interrupted scanline to resume at the exact PPU dot
// where a CPU debugger BreakPoint stopped execution.
//
// Because clock_ppu() executes before the CPU/APU slot for a given timing point,
// an execute BreakPoint may occur after the PPU dot has already completed but
// before the CPU has consumed its corresponding cycle.  sPendingCpuSlot records
// that condition so the CPU/APU slot can be completed after debugger resume
// without clocking the same PPU dot twice.
//
// Parameters:
//   target - PPU scanline rendering target.
//
// Returns:
//   true  - The complete scanline finished.
//   false - Execution stopped before the scanline finished.
// -----------------------------------------------------------------------------
template <class T>
bool
execute_scanline_impl (const T &target)
{
	if (nes::ppu::system_paused) {
		return false;
	}

	/*
	 * If a BreakPoint stopped us after clock_ppu() but before the CPU
	 * consumed this timing slot, finish that CPU/APU slot first.
	 *
	 * Do NOT clock the PPU again here.
	 */
	if (sPendingCpuSlot) {
		const uint64_t beforeCycles = nes::cpu::cycle_count();

		nes::cpu::exec<1>();

		const uint64_t afterCycles = nes::cpu::cycle_count();

		/*
		 * The CPU still refused to advance. Leave the timing slot
		 * pending and remain at exactly the same PPU position.
		 */
		if (afterCycles == beforeCycles) {
			return false;
		}

		/*
		 * The CPU consumed the pending cycle. The APU must consume its
		 * corresponding cycle as well.
		 */
		nes::apu::exec<1>();

		sPendingCpuSlot = false;

		/*
		 * Complete the PPU timing position whose clock_ppu() call was
		 * performed before the debugger stop.
		 */
		++hpos_;
		++ppu_cycle_;

		if (hpos_ >= kCyclesPerScanline) {
			hpos_ = 0;
			++vpos_;

			return true;
		}

		/*
		 * A memory/stack BreakPoint could theoretically have become
		 * latched while the pending CPU cycle itself executed.
		 */
		if (nes::cpu::debug_breakpoint_hit()) {
			return false;
		}
	}

	/*
	 * Frame-boundary PPU work must happen only once, at the genuine
	 * beginning of a scanline.
	 */
	if (hpos_ == 0) {
		if (UNLIKELY(vpos_ == 262)) {
			start_frame();
		} else if (UNLIKELY(vpos_ == 241)) {
			end_frame();
		}
	}

	while (hpos_ < kCyclesPerScanline) {
		/*
		 * First execute the PPU portion of this dot, preserving the
		 * emulator's existing PPU-before-CPU ordering.
		 */
		clock_ppu(target);

		if ((ppu_cycle_ % 3) == kCPUAlignment) {
			const uint64_t beforeCycles = nes::cpu::cycle_count();

			nes::cpu::exec<1>();

			const uint64_t afterCycles = nes::cpu::cycle_count();

			/*
			 * An execute BreakPoint is checked at CPU instruction
			 * cycle zero before the CPU actually clocks. Therefore
			 * the PPU dot above has happened but this CPU cycle has
			 * not.
			 *
			 * Preserve the exact timing position and return.
			 */
			if (afterCycles == beforeCycles) {
				sPendingCpuSlot = true;

				return false;
			}

			/*
			 * Keep the APU synchronized with every CPU cycle that
			 * genuinely completed.
			 */
			nes::apu::exec<1>();
		}

		/*
		 * This complete PPU timing point has now been consumed.
		 */
		++hpos_;
		++ppu_cycle_;

		/*
		 * READ/WRITE/stack conditions may latch while the CPU cycle
		 * itself executes. In that case the CPU slot did complete, so
		 * no pending-slot state is required.
		 */
		if (nes::cpu::debug_breakpoint_hit()) {
			return false;
		}
	}

	/*
	 * Normalize to the start of the next scanline.
	 */
	hpos_ = 0;
	++vpos_;

	return true;
}


} // end anonymous namespace

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
namespace nes::ppu {

// -----------------------------------------------------------------------------
// nes::ppu::reset
//
// Resets the PPU using the requested reset type.
// A hard reset also clears OAM, secondary OAM, sprite pattern state, and
// restores the power-up palette.
//
// Parameters:
//   reset_type - Type of reset to perform.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
reset (nes::Reset reset_type)
{
	if (reset_type == Reset::Hard) {
		std::fill_n(sprite_ram_, 0x0100, 0);
		std::fill_n(sprite_data_, 32, 0xff);
		std::copy(std::begin(powerup_palette), std::end(powerup_palette), palette_);

		for (int i = 0; i < 8; ++i) {
			sprite_patterns_[i].patterns[0] = 0;
			sprite_patterns_[i].patterns[1] = 0;
			sprite_patterns_[i].x           = 0;
			sprite_patterns_[i].y           = 0;
			sprite_patterns_[i].index       = 0;
			sprite_patterns_[i].attr        = 0;
		}
	}

	attribute_queue_[0]   = 0;
	attribute_queue_[1]   = 0;
	hpos_                 = 0;
	latch_                = 0;
	nametable_            = 0x0000;
	next_attribute_       = 0;
	next_pattern_[0]      = 0;
	next_pattern_[1]      = 0;
	next_tile_index_      = 0;
	odd_frame_            = false;
	pattern_queue_[0]     = 0;
	pattern_queue_[1]     = 0;
	ppu_cycle_            = 0;
	ppu_control_.raw      = 0;
	ppu_mask_.raw         = 0;
	register_2007_buffer_ = 0;
	sprite_address_       = 0;
	sprite_data_index_    = 0;
	left_most_sprite_x_   = 0xff;
	status_.raw           = 0;
	tile_offset_          = 0;
	vpos_                 = 0;
	vram_address_         = 0x0000;
	write_latch_          = false;
	write_block_          = true;
	monochrome_mask_      = 0xff;
	frame_counter_		  = 0;
	sPendingCpuSlot = false;
	std::cout << "PPU reset complete" << std::endl;
}


// -----------------------------------------------------------------------------
// nes::ppu::write2000
//
// Writes PPUCTRL, updating control state, nametable selection, and vblank NMI
// behavior.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2000 (uint8_t value)
{
	log_ppu_write(0x2000, value);
	
	latch_ = value;

	if (write_block_) {
		return;
	}

	const ppu_ctrl_t prev_control = ppu_control_;
	ppu_control_.raw           = value;

	nametable_ &= 0b1111001111111111;
	nametable_ |= ((value & 0b00000011) << 10);

	if (prev_control.nmi_on_vblank && !ppu_control_.nmi_on_vblank) {
		cpu::clear_nmi();
	} else if (!prev_control.nmi_on_vblank && ppu_control_.nmi_on_vblank && status_.vblank && hpos_ != 0) {
		cpu::nmi();
	}
}


// -----------------------------------------------------------------------------
// nes::ppu::write2001
//
// Writes PPUMASK, updating rendering enables, clipping, emphasis, and
// monochrome state.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2001 (uint8_t value)
{
	log_ppu_write(0x2001, value);
	
	latch_ = value;

	if (write_block_) {
		return;
	}

	ppu_mask_.raw     = value;
	monochrome_mask_  = (ppu_mask_.monochrome) ? 0x30 : 0xff;
}


// -----------------------------------------------------------------------------
// nes::ppu::write2002
//
// Updates the PPU I/O latch for a CPU write directed at the normally read-only
// PPUSTATUS address.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2002 (uint8_t value)
{ 
	log_ppu_write(0x2002, value);
	
	latch_ = value; 
}


// -----------------------------------------------------------------------------
// nes::ppu::write2003
//
// Writes OAMADDR and updates the PPU I/O latch.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2003 (uint8_t value)
{
	log_ppu_write(0x2003, value);
	
	latch_          = value;
	sprite_address_ = value;
}

// -----------------------------------------------------------------------------
// nes::ppu::write2004
//
// Writes one OAM byte at OAMADDR and advances OAMADDR.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
write2004 (uint8_t value)
{
	latch_ = value;
	sprite_ram_[sprite_address_++] = value;
}


// -----------------------------------------------------------------------------
// nes::ppu::write2005
//
// Processes one PPUSCROLL write using the shared first/second-write latch.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2005 (uint8_t value)
{
	log_ppu_write(0x2005, value);
	
	latch_ = value;

	if (write_block_) {
		return;
	}

	write_latch_ = !write_latch_;

	if (write_latch_) {
		nametable_ &= 0b1111111'11100000;
		nametable_ |= (value & 0b11111000) >> 3;
		tile_offset_ = value & 0x07;
	} else {
		nametable_ &= ~0b01110011'11100000;
		nametable_ |= (value & 0b11111000) << 2;
		nametable_ |= (value & 0b00000111) << 12;
	}
}


// -----------------------------------------------------------------------------
// nes::ppu::write2006
//
// Processes one PPUADDR write using the shared first/second-write latch.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2006 (uint8_t value)
{
	log_ppu_write(0x2006, value);
	
	latch_ = value;

	if (write_block_) {
		return;
	}

	write_latch_ = !write_latch_;

	if (write_latch_) {
		nametable_ &= 0b00000000'11111111;
		nametable_ |= (value & 0b00111111) << 8;
	} else {
		nametable_ &= 0b01111111'00000000;
		nametable_ |= value;
		vram_address_ = nametable_;
		nes::cart.mapper()->vram_change_hook(vram_address_);
	}
}


// -----------------------------------------------------------------------------
// nes::ppu::write2007
//
// Writes PPUDATA to palette RAM or mapper VRAM at the current PPU address,
// then advances the VRAM address.
//
// Parameters:
//   value - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
write2007 (uint8_t value)
{
	log_ppu_write(0x2007, value);
	
	latch_ = value;

	const uint_least16_t temp_address = vram_address_ & 0b00111111'11111111;

	increment_vram_address();
	nes::cart.mapper()->vram_change_hook(vram_address_);

	if ((temp_address & 0b00111111'00000000) == 0b00111111'00000000) {

		const uint_least8_t palette_address = temp_address & 0x1f;
		palette_[palette_address]           = value & 0x3f;

		if ((palette_address & 0x03) == 0x00) {
			palette_[palette_address ^ 0x10] = value & 0x3f;
		}

	} else {
		nes::cart.mapper()->write_vram(temp_address, value);
	}
}


// -----------------------------------------------------------------------------
// nes::ppu::read200x
//
// Returns the current PPU I/O latch value for reads from write-only PPU
// registers.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPU I/O latch value.
// -----------------------------------------------------------------------------
uint8_t 
read200x() 
{ 
	return static_cast<uint8_t>(latch_);
}


// -----------------------------------------------------------------------------
// nes::ppu::read2002
//
// Reads PPUSTATUS, clears vblank, and resets the shared $2005/$2006 write
// latch.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPUSTATUS/open-bus value.
// -----------------------------------------------------------------------------
uint8_t
read2002()
{
	const uint8_t ret =
		((status_.raw & (kOverflowStatus | kSprite0Status | kVBlankStatus)) |
		 (latch_ & ~(kOverflowStatus | kSprite0Status | kVBlankStatus))) & 0xff;

	write_latch_ = false;
	status_.vblank = false;

	ppu_read_2002_cycle_ = ppu_cycle_;
	return ret;
}


// -----------------------------------------------------------------------------
// nes::ppu::read2004
//
// Reads OAMDATA according to the current OAM address when rendering permits
// direct OAM access.
//
// Parameters:
//   None.
//
// Returns:
//   Current OAMDATA value.
// -----------------------------------------------------------------------------
uint8_t
read2004()
{
	if (!rendering() || !ppu_mask_.screen_enabled) {
		switch (sprite_address_ & 0x03) {
		case 0x00:
		case 0x01:
		case 0x03:
			latch_ = sprite_ram_[sprite_address_] & 0xff;
			break;
		case 0x02:
			latch_ = sprite_ram_[sprite_address_] & 0xe3;
			break;
		}
		return latch_ & 0xff;
	}

	return 0x00;
}


// -----------------------------------------------------------------------------
// nes::ppu::read2007
//
// Reads PPUDATA using the PPU read-buffer rules, including immediate palette
// reads and automatic VRAM-address incrementing.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPUDATA value.
// -----------------------------------------------------------------------------
uint8_t
read2007()
{
	if (write_block_) {
		return 0x00;
	}

	const uint_least16_t temp_address = vram_address_ & 0b00111111'11111111;

	increment_vram_address();
	nes::cart.mapper()->vram_change_hook(vram_address_);

	const auto decay_value = static_cast<uint8_t>(latch_);

	latch_                = register_2007_buffer_;
	register_2007_buffer_ = nes::cart.mapper()->read_vram(temp_address);

	if ((temp_address & 0b00111111'00000000) == 0b00111111'00000000) {
		latch_ = palette_[temp_address & 0x1f] | (decay_value & 0xc0);
		if (UNLIKELY(ppu_mask_.monochrome)) {
			latch_ &= 0xf0;
		}
	}

	return latch_ & 0xff;
}


// -----------------------------------------------------------------------------
// nes::ppu::write4014
//
// Starts the 256-byte OAM DMA transfer selected by the value written to $4014.
//
// Parameters:
//   value - High byte of the CPU source address.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void 
write4014 (uint8_t value)
{
	log_ppu_write(0x4014, value);
	
	const auto sprite_addr = static_cast<uint_least16_t>(value << 8);
	cpu::schedule_spr_dma(write2004, sprite_addr, 256);
}


// -----------------------------------------------------------------------------
// Public resumable scanline entry points.
//
// Returns true only when the requested scanline has completely finished.
// -----------------------------------------------------------------------------
bool
execute_scanline (const scanline_vblank &target)
{
	return execute_scanline_impl(target);
}


// -----------------------------------------------------------------------------
// nes::ppu::execute_scanline
//
// Executes or resumes one PPU scanline using the resumable scanline engine.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   true if the scanline completed; false if debugger execution interrupted it.
// -----------------------------------------------------------------------------
bool
execute_scanline (const scanline_prerender &target)
{
	return execute_scanline_impl(target);
}


// -----------------------------------------------------------------------------
// nes::ppu::execute_scanline
//
// Executes or resumes one PPU scanline using the resumable scanline engine.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   true if the scanline completed; false if debugger execution interrupted it.
// -----------------------------------------------------------------------------
bool
execute_scanline (const scanline_postrender &target)
{
	return execute_scanline_impl(target);
}


// -----------------------------------------------------------------------------
// nes::ppu::execute_scanline
//
// Executes or resumes one PPU scanline using the resumable scanline engine.
//
// Parameters:
//   target - Scanline-type tag or visible rendering target.
//
// Returns:
//   true if the scanline completed; false if debugger execution interrupted it.
// -----------------------------------------------------------------------------
bool
execute_scanline (const scanline_render &target)
{
	return execute_scanline_impl(target);
}


//------------------------------------------------------------------------------
// Debug helpers
//------------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// nes::ppu::scroll_state
//
// Captures the current loopy scroll registers, fine-X value, and PPUCTRL state
// for debugger inspection.
//
// Parameters:
//   None.
//
// Returns:
//   Snapshot of the current PPU scroll state.
// -----------------------------------------------------------------------------
scroll_state_t
scroll_state()
{
	scroll_state_t s{};

	s.v = static_cast<uint16_t>(vram_address_);
	s.t = static_cast<uint16_t>(nametable_);
	s.x = tile_offset_;
	s.ctrl = ppu_control_.raw;

	return s;
}

// -----------------------------------------------------------------------------
// nes::ppu::vram_address
//
// Returns the current loopy-v PPU VRAM address.
//
// Parameters:
//   None.
//
// Returns:
//   Current VRAM address.
// -----------------------------------------------------------------------------
uint16_t
vram_address()
{ 
	return static_cast<uint16_t>(vram_address_);
}


// -----------------------------------------------------------------------------
// nes::ppu::temp_address
//
// Returns the current loopy-t temporary PPU VRAM address.
//
// Parameters:
//   None.
//
// Returns:
//   Current temporary VRAM address.
// -----------------------------------------------------------------------------
uint16_t 
temp_address()
{ 
	return static_cast<uint16_t>(nametable_);
}


// -----------------------------------------------------------------------------
// nes::ppu::fine_x
//
// Returns the current fine-X scroll value.
//
// Parameters:
//   None.
//
// Returns:
//   Current fine-X scroll value.
// -----------------------------------------------------------------------------
uint8_t
fine_x() 
{ 
	return tile_offset_; 
}


//------------------------------------------------------------------------------
// Misc getters
//------------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// nes::ppu::cycle_count
//
// Returns the total PPU cycle count.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPU cycle count.
// -----------------------------------------------------------------------------
uint64_t cycle_count() 
{ 
	return ppu_cycle_;
}


// -----------------------------------------------------------------------------
// nes::ppu::hpos
//
// Returns the current horizontal PPU dot position.
//
// Parameters:
//   None.
//
// Returns:
//   Current horizontal dot.
// -----------------------------------------------------------------------------
uint_least16_t 
hpos()  	
{ 
	return hpos_; 					  	
}


// -----------------------------------------------------------------------------
// nes::ppu::vpos
//
// Returns the current PPU scanline position.
//
// Parameters:
//   None.
//
// Returns:
//   Current scanline.
// -----------------------------------------------------------------------------
uint_least16_t 
vpos()
{ 
	return vpos_; 						
}


// -----------------------------------------------------------------------------
// nes::ppu::ppuctrl
//
// Returns the current raw PPUCTRL register value.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPUCTRL value.
// -----------------------------------------------------------------------------
uint8_t
ppuctrl()
{ 
	return ppu_control_.raw;
}


// -----------------------------------------------------------------------------
// nes::ppu::ppumask
//
// Returns the current raw PPUMASK register value.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPUMASK value.
// -----------------------------------------------------------------------------
uint8_t
ppumask()
{ 
	return ppu_mask_.raw;
}


// -----------------------------------------------------------------------------
// nes::ppu::ppustatus
//
// Returns the current raw PPUSTATUS register value.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPUSTATUS value.
// -----------------------------------------------------------------------------
uint8_t
ppustatus()
{ 
	return status_.raw;
}


// -----------------------------------------------------------------------------
// nes::ppu::ppu_dot
//
// Returns the current PPU dot position for debugger inspection.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPU dot.
// -----------------------------------------------------------------------------
uint16_t
ppu_dot()
{ 
	return static_cast<uint16_t>(hpos_);
}


// -----------------------------------------------------------------------------
// nes::ppu::ppu_scanline
//
// Returns the current PPU scanline number for debugger inspection.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPU scanline.
// -----------------------------------------------------------------------------
uint16_t 
ppu_scanline()
{ 
	return static_cast<uint16_t>(vpos_);

}


// -----------------------------------------------------------------------------
// nes::ppu::palette_ram
//
// Reads one byte from the internal 32-byte PPU palette RAM array.
//
// Parameters:
//   address - Palette RAM address.
//
// Returns:
//   Palette value at the requested address.
// -----------------------------------------------------------------------------
uint8_t
palette_ram (uint32_t address)
{
	return palette_[address & 0x1f];
}


// -----------------------------------------------------------------------------
// nes::ppu::set_palette_ram
//
// Writes one six-bit color value into internal PPU palette RAM.
//
// Parameters:
//   address - Palette RAM address.
//   data    - Palette value to store.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
set_palette_ram (uint32_t address, uint8_t data)
{
	palette_[address & 0x1f] = data & 0x3f;
}


// -----------------------------------------------------------------------------
// nes::ppu::oam_ram
//
// Reads one byte directly from primary OAM for debugger/display inspection.
//
// Parameters:
//   address - OAM byte address.
//
// Returns:
//   OAM byte at the requested address.
// -----------------------------------------------------------------------------
uint8_t
oam_ram (uint32_t address)
{
	return sprite_ram_[address & 0xff];
}


// -----------------------------------------------------------------------------
// nes::ppu::oam_addr
//
// Returns the current OAMADDR register value.
//
// Parameters:
//   None.
//
// Returns:
//   Current OAM address.
// -----------------------------------------------------------------------------
uint8_t
oam_addr()
{
	return sprite_address_;
}


// -----------------------------------------------------------------------------
// log_ppu_write
//
// Appends one CPU write to a PPU-facing register to the rolling write log.
//
// In addition to frame/render position and the written value, the current
// $2005/$2006 shared write-latch state is captured before the register handler
// changes it.  This allows debugger views to distinguish first and second
// PPUSCROLL/PPUADDR writes accurately.
//
// write_latch:
//   0 - first $2005/$2006 write
//   1 - second $2005/$2006 write
//
// Parameters:
//   address - CPU-visible PPU register address.
//   value   - Value written by the CPU.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
log_ppu_write (uint16_t address, uint8_t value)
{
	ppu_write_log_entry_t& entry = write_log_[write_log_next_];

	entry.frame = frame_counter_;
	entry.dot = static_cast<uint16_t>(hpos_);
	entry.scanline = static_cast<uint16_t>(vpos_);
	entry.address = address;
	entry.value = value;
	entry.write_index = write_log_write_index_++;

	/*
	 * log_ppu_write() is called before write2005()/write2006() toggle
	 * write_latch_, so this records the state that applies to the current
	 * write.
	 */
	entry.write_latch = write_latch_ ? 1 : 0;

	write_log_next_ = (write_log_next_ + 1) % PPU_WRITE_LOG_CAPACITY;

	if (write_log_count_ < PPU_WRITE_LOG_CAPACITY) {
		write_log_count_++;
	}
}


// -----------------------------------------------------------------------------
// nes::ppu::ppu_write_log_count
//
// Returns the number of valid entries currently stored in the rolling PPU
// register-write log.
//
// Parameters:
//   None.
//
// Returns:
//   Current PPU write-log entry count.
// -----------------------------------------------------------------------------
uint32_t
ppu_write_log_count()
{
	return write_log_count_;
}


// -----------------------------------------------------------------------------
// nes::ppu::ppu_write_log_entry
//
// Returns one logical PPU write-log entry in oldest-to-newest order.
//
// Parameters:
//   index - Logical write-log entry index.
//
// Returns:
//   Requested entry, or an empty entry if the index is invalid.
// -----------------------------------------------------------------------------
ppu_write_log_entry_t
ppu_write_log_entry (uint32_t index)
{
	ppu_write_log_entry_t empty{};

	if (index >= write_log_count_) {
		return empty;
	}

	uint32_t start = 0;

	if (write_log_count_ == PPU_WRITE_LOG_CAPACITY) {
		start = write_log_next_;
	}

	uint32_t physicalIndex = (start + index) % PPU_WRITE_LOG_CAPACITY;

	return write_log_[physicalIndex];
}


// -----------------------------------------------------------------------------
// nes::ppu::clear_ppu_write_log
//
// Clears the rolling PPU register-write log and resets its sequence state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
clear_ppu_write_log()
{
	write_log_next_ = 0;
	write_log_count_ = 0;
	write_log_write_index_ = 0;
}


// -----------------------------------------------------------------------------
// ppu_write_log_snapshot
//
// Copies the current logical PPU write log into caller-provided storage.
//
// The ring-buffer count and starting position are captured once before copying,
// so every entry in the returned snapshot uses the same logical ordering.
//
// Entries are returned oldest to newest:
//
//   entries[0]         = oldest captured write
//   entries[count - 1] = newest captured write
//
// Parameters:
//   entries  - Destination array.
//   capacity - Maximum number of entries that may be written.
//
// Returns:
//   Number of entries copied.
// -----------------------------------------------------------------------------
uint32_t
ppu_write_log_snapshot(ppu_write_log_entry_t *entries, uint32_t capacity)
{
	if (!entries || capacity == 0) {
		return 0;
	}

	const uint32_t available = write_log_count_;
	const uint32_t count = (available < capacity) ? available : capacity;

	if (count == 0) {
		return 0;
	}

	uint32_t start = 0;

	if (available == PPU_WRITE_LOG_CAPACITY) {
		start = write_log_next_;
	}

	/*
	 * If the destination is smaller than the current log, retain the
	 * newest entries rather than the oldest ones.
	 */
	if (count < available) {
		start = (start + (available - count)) % PPU_WRITE_LOG_CAPACITY;
	}

	for (uint32_t index = 0; index < count; index++) {
		const uint32_t physicalIndex = (start + index) % PPU_WRITE_LOG_CAPACITY;
		entries[index] = write_log_[physicalIndex];
	}

	return count;
}


// -----------------------------------------------------------------------------
// ppu_frame_counter
//
// Returns the current PPU frame counter value.
//
// Parameters:
//   None.
//
// Returns:
//   Number of PPU frames completed so far.
// -----------------------------------------------------------------------------
uint64_t
ppu_frame_counter()
{
	return frame_counter_;
}


// -----------------------------------------------------------------------------
// nes::ppu::debug_read_ppu_memory
//
// Reads PPU memory for debugger inspection without CPU-facing PPU register side
// effects.
//
// Parameters:
//   address - PPU address to inspect.
//
// Returns:
//   Debug-safe byte value from PPU memory.
// -----------------------------------------------------------------------------
uint8_t
debug_read_ppu_memory (uint16_t address)
{
	address &= 0x3fff;

	if (address >= 0x3f00) {
		uint8_t paletteAddress = address & 0x1f;

		if ((paletteAddress & 0x13) == 0x10)
			paletteAddress ^= 0x10;

		return palette_[paletteAddress] & 0x3f;
	}

	if (!nes::cart.mapper()) {
		return 0x00;
	}

	return nes::cart.mapper()->read_vram(address);
}

// -----------------------------------------------------------------------------
// nes::ppu::debug_step_dot
//
// Advances debugger execution by one PPU timing position.
//
// If normal execution stopped after clocking a PPU dot but before the matching
// CPU/APU slot could execute, the pending slot is completed first without
// clocking the PPU dot a second time.
//
// Visible rendering uses the same persistent scanline buffer as run_frame() so
// debugger stepping can safely occur in the middle of a visible scanline.
//
// system_paused is intentionally ignored because debugger stepping operates while
// normal emulator execution is paused.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_step_dot()
{
	if (sPendingCpuSlot) {
		const uint64_t beforeCycles = nes::cpu::cycle_count();

		nes::cpu::exec<1>();

		const uint64_t afterCycles = nes::cpu::cycle_count();

		if (afterCycles == beforeCycles) {
			return;
		}

		nes::apu::exec<1>();

		sPendingCpuSlot = false;

		++hpos_;
		++ppu_cycle_;

		if (hpos_ >= kCyclesPerScanline) {
			hpos_ = 0;
			++vpos_;
		}

		return;
	}

	if (hpos_ >= kCyclesPerScanline) {
		hpos_ = 0;
		++vpos_;
	}

	if (vpos_ > 262) {
		vpos_ = 0;
		hpos_ = 0;
	}

	if (hpos_ == 0) {
		if (UNLIKELY(vpos_ == 262)) {
			start_frame();
		} else if (UNLIKELY(vpos_ == 241)) {
			end_frame();
		}
	}

	if (vpos_ == 0) {
		clock_ppu(nes::ppu::scanline_prerender{});
	} else if (vpos_ >= 1 && vpos_ <= 240) {
		clock_ppu(nes::ppu::scanline_render(nes::frame_scanline_buffer()));
	} else if (vpos_ == 241) {
		clock_ppu(nes::ppu::scanline_postrender{});
	} else {
		clock_ppu(nes::ppu::scanline_vblank{});
	}

	if ((ppu_cycle_ % 3) == kCPUAlignment) {
		const uint64_t beforeCycles = nes::cpu::cycle_count();

		nes::cpu::exec<1>();

		const uint64_t afterCycles = nes::cpu::cycle_count();

		if (afterCycles == beforeCycles) {
			sPendingCpuSlot = true;
			return;
		}

		nes::apu::exec<1>();
	}

	++hpos_;
	++ppu_cycle_;

	if (hpos_ >= kCyclesPerScanline) {
		hpos_ = 0;
		++vpos_;
	}
}


} // namespace nes::ppu

