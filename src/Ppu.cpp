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

union Status {
	uint8_t raw;
	BitField<uint8_t, 5> overflow;
	BitField<uint8_t, 6> sprite0;
	BitField<uint8_t, 7> vblank;
};

union Control {
	uint8_t raw;
	BitField<uint8_t, 0, 2> nametable;
	BitField<uint8_t, 2> address_increment;
	BitField<uint8_t, 3> sprite_pattern_table;
	BitField<uint8_t, 4> background_pattern_table;
	BitField<uint8_t, 5> large_sprites;
	BitField<uint8_t, 6> master;
	BitField<uint8_t, 7> nmi_on_vblank;
};

union Mask {
	uint8_t raw;
	BitField<uint8_t, 0> monochrome;
	BitField<uint8_t, 1> background_clipping;
	BitField<uint8_t, 2> sprite_clipping;
	BitField<uint8_t, 3> background_visible;
	BitField<uint8_t, 4> sprites_visible;
	BitField<uint8_t, 5, 3> intensity;

	// meta-fields which don't occupy any space :-)
	BitField<uint8_t, 3, 2> screen_enabled;
};

constexpr auto CyclesPerScanline = 341u;
constexpr auto CpuAlignment      = 0u;

constexpr uint8_t StatusOverflow = 0b0010'0000;
constexpr uint8_t StatusSprite0  = 0b0100'0000;
constexpr uint8_t StatusVBlank   = 0b1000'0000;

constexpr uint8_t OamColor    = 0b0000'0011;
constexpr uint8_t OamZero     = 0b0001'0000; // internal flag
constexpr uint8_t OamPriority = 0b0010'0000;
constexpr uint8_t OamHFlip    = 0b0100'0000;
constexpr uint8_t OamVFlip    = 0b1000'0000;

struct pattern_0 {
	static constexpr int index      = 0;
	static constexpr uint8_t offset = 0;
};

struct pattern_1 {
	static constexpr int index      = 1;
	static constexpr uint8_t offset = 8;
};

struct size_8px {
	static constexpr int flip_mask = 0b0000'0111;
};

struct size_16px {
	static constexpr int flip_mask = 0b0000'1111;
};

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
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

const uint8_t powerup_palette[32] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08};

//------------------------------------------------------------------------------
// Name: attribute_bits
//------------------------------------------------------------------------------
constexpr uint8_t attribute_bits(uint_least16_t vram_address, uint8_t attr_byte) {
	return (attr_byte >> (((vram_address & 0x40) >> 4) | (vram_address & 0x02))) & 0x03;
}

//------------------------------------------------------------------------------
// Name: attribute_address
//------------------------------------------------------------------------------
constexpr uint_least16_t attribute_address(uint_least16_t vram_address) {
	return 0x23c0 | (vram_address & 0x0c00) | ((vram_address >> 4) & 0x38) | ((vram_address >> 2) & 0x07);
}

//------------------------------------------------------------------------------
// Name: tile_address
//------------------------------------------------------------------------------
constexpr uint_least16_t tile_address(uint_least16_t vram_address) {
	return 0x2000 | (vram_address & 0x0fff);
}

struct SpritePattern {
	uint8_t x;
	uint8_t y;
	uint8_t index;
	uint8_t attr;
	uint8_t patterns[2];
};

//------------------------------------------------------------------------------
// Internal state (single definitions only)
//------------------------------------------------------------------------------
SpritePattern sprite_patterns_[8];
uint8_t current_sprite_index_ = 0;

uint8_t sprite_ram_[0x100] = {};
uint8_t sprite_address_    = 0; // OAMADDR

uint8_t  sprite_data_[32]       = {};
uint8_t  sprite_data_index_     = 0;
uint8_t  left_most_sprite_x_    = 0xff;
uint8_t  sprite_read_buffer_    = 0;
uint8_t  sprite_read_index_     = 0;
bool     current_is_sprite_0    = false;
uint8_t  visible_sprite_count_  = 0;

enum SpriteEvalState {
	STATE_1_Y,
	STATE_1_I,
	STATE_1_A,
	STATE_1_X,
	STATE_3,
	STATE_4
} sprite_eval_state_ = STATE_1_Y;

uint8_t        palette_[0x20]               = {};
uint64_t       ppu_cycle_                   = 0;
uint64_t       ppu_read_2002_cycle_         = 0;
uint_least16_t next_ppu_fetch_address_      = 0;
uint_least16_t pattern_queue_[2]            = {};
uint_least16_t attribute_queue_[2]          = {};
uint_least16_t nametable_                   = 0; // loopy t
uint_least16_t vram_address_                = 0; // loopy v
uint_least16_t hpos_                        = 0; // pixel counter
uint_least16_t vpos_                        = 0; // scanline counter
uint8_t        next_pattern_[2]             = {};
uint_least16_t latch_                       = 0;
uint8_t        next_attribute_              = 0;
uint8_t        next_tile_index_             = 0;
Control        ppu_control_                 = {0};
Mask           ppu_mask_                    = {0};
uint8_t        register_2007_buffer_        = 0;
Status         status_                      = {0};
uint8_t        tile_offset_                 = 0; // loopy x
uint8_t        monochrome_mask_             = 0xff;

bool odd_frame_   = false;
bool write_latch_ = false;
bool write_block_ = false;

//------------------------------------------------------------------------------
// Name: sprite_pattern_table
//------------------------------------------------------------------------------
uint_least16_t sprite_pattern_table() {
	return ppu_control_.sprite_pattern_table ? 0x1000 : 0x0000;
}

//------------------------------------------------------------------------------
// Name: background_pattern_table
//------------------------------------------------------------------------------
uint_least16_t background_pattern_table() {
	return ppu_control_.background_pattern_table ? 0x1000 : 0x0000;
}

//------------------------------------------------------------------------------
// Sprite pattern address helpers
//------------------------------------------------------------------------------
template <class Pattern>
constexpr uint_least16_t sprite_pattern_address(uint8_t index, uint8_t sprite_line, const size_8px &) {
	return (sprite_pattern_table() | (index << 4) | Pattern::offset | sprite_line) & 0xffff;
}

template <class Pattern>
constexpr uint_least16_t sprite_pattern_address(uint8_t index, uint8_t sprite_line, const size_16px &) {
	return (((index & 1) << 12) | ((index & 0xfe) << 4) | Pattern::offset | (sprite_line & 7) |
	        ((sprite_line & 0x08) << 1)) &
	       0xffff;
}

template <class Size, class Pattern>
constexpr uint_least16_t sprite_pattern_address(uint8_t index, uint8_t sprite_line) {
	return sprite_pattern_address<Pattern>(index, sprite_line, Size());
}

//------------------------------------------------------------------------------
// Name: render_blank_pixel
//------------------------------------------------------------------------------
uint8_t render_blank_pixel() {
	if (UNLIKELY((vram_address_ & 0x3f00) == 0x3f00)) {
		return palette_[vram_address_ & 0x1f] & monochrome_mask_;
	}
	return palette_[0x00] & monochrome_mask_;
}

//------------------------------------------------------------------------------
// Name: select_bg_pixel
//------------------------------------------------------------------------------
uint8_t select_bg_pixel(uint_least16_t index) {

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

//------------------------------------------------------------------------------
// Name: select_pixel
//------------------------------------------------------------------------------
uint8_t select_pixel(uint_least16_t index) {

	const uint8_t pixel = select_bg_pixel(index);

	if (LIKELY(index >= 8 || ppu_mask_.sprite_clipping) && ppu_mask_.sprites_visible) {

		for (uint8_t spr = 0; spr != visible_sprite_count_; ++spr) {

			const SpritePattern &sprite = sprite_patterns_[spr];
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
			if ((sprite.attr & OamZero) && (index < 255) && (pixel & 0x03)) {
#else
			if ((sprite.attr & OamZero) && (index < 255)) {
#endif
				status_.sprite0 = true;
			}

			if (UNLIKELY(!nes::ppu::show_sprites)) {
				return pixel;
			}

			if ((((sprite.attr & OamPriority) == 0) || ((pixel & 0x03) == 0))) {
				return (0x10 | sprite_pixel | ((sprite.attr & OamColor) << 2)) & 0xff;
			}

			return pixel;
		}
	}

	return pixel;
}

//------------------------------------------------------------------------------
// Name: clock_x
//------------------------------------------------------------------------------
void clock_x() {
	if (UNLIKELY((vram_address_ & 0x1f) == 0x1f)) {
		vram_address_ ^= 0x41f;
	} else {
		++vram_address_;
	}
}

//------------------------------------------------------------------------------
// Name: clock_y
//------------------------------------------------------------------------------
void clock_y() {
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

template <class Pattern>
void open_background_pattern() {
	const uint8_t tile_line = (vram_address_ & 0x7000) >> 12;
	next_ppu_fetch_address_ =
		(background_pattern_table() | (next_tile_index_ << 4) | Pattern::offset | tile_line) & 0xffff;
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}

template <class Pattern>
void read_background_pattern() {
	next_pattern_[Pattern::index] = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
}

void open_background_attribute() {
	next_ppu_fetch_address_ = attribute_address(vram_address_);
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}

void read_background_attribute() {
	const uint8_t attr_byte = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
	next_attribute_         = attribute_bits(vram_address_, attr_byte);
}

void open_tile_index() {
	next_ppu_fetch_address_ = tile_address(vram_address_);
	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}

void read_tile_index() {
	next_tile_index_ = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);
}

bool sprite_in_range(uint8_t y) {
	const uint_least16_t sprite_line = (vpos_ - 1) - y;
	return ppu_control_.large_sprites ? (sprite_line < 16) : (sprite_line < 8);
}

uint8_t &sprite_y(uint8_t index) { return sprite_data_[index * 4 + 0]; }
uint8_t &sprite_index(uint8_t index) { return sprite_data_[index * 4 + 1]; }
uint8_t &sprite_attr(uint8_t index) { return sprite_data_[index * 4 + 2]; }
uint8_t &sprite_x(uint8_t index) { return sprite_data_[index * 4 + 3]; }

void evaluate_sprites_even() {
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
				sprite_attr(sprite_data_index_) |= OamZero;
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

void evaluate_sprites_odd() {
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

void enter_vblank() {
	if (UNLIKELY(ppu_cycle_ != (ppu_read_2002_cycle_ + 1))) {
		status_.vblank = true;
	}
}

template <class Size, class Pattern>
void open_sprite_pattern() {

	current_sprite_index_ = ((hpos_ - 1) >> 3) & 0x07;
	SpritePattern &sprite = sprite_patterns_[current_sprite_index_];

	sprite.y = sprite_y(current_sprite_index_);

	if (sprite.y != 0xff) {

		sprite.x     = sprite_x(current_sprite_index_);
		sprite.attr  = sprite_attr(current_sprite_index_);
		sprite.index = sprite_index(current_sprite_index_);

		if (sprite.attr & OamVFlip) {
			sprite.y ^= Size::flip_mask;
		}

		next_ppu_fetch_address_ = sprite_pattern_address<Size, Pattern>(sprite.index, sprite.y);
	} else {
		next_ppu_fetch_address_ = sprite_pattern_address<Size, Pattern>(0xff, 0xff);
	}

	nes::cart.mapper()->vram_change_hook(next_ppu_fetch_address_);
}

template <class Size, class Pattern>
void read_sprite_pattern() {

	uint8_t pattern = nes::cart.mapper()->read_vram(next_ppu_fetch_address_);

	SpritePattern &sprite = sprite_patterns_[current_sprite_index_];

	if (sprite.attr & OamHFlip) {
		pattern = reverse_bits[pattern];
	}

	sprite_patterns_[current_sprite_index_].patterns[Pattern::index] = pattern;
}

uint8_t render_pixel() {

	const uint8_t pixel = select_pixel(hpos_ - 1);

	pattern_queue_[0] <<= 1;
	pattern_queue_[1] <<= 1;
	attribute_queue_[0] <<= 1;
	attribute_queue_[1] <<= 1;

	const uint8_t mask = ((pixel & 0x01) | ((pixel & 0x02) >> 1)) * 0xff;
	return palette_[pixel & mask] & monochrome_mask_;
}

void update_shift_registers_render() {
	pattern_queue_[0] |= next_pattern_[0];
	pattern_queue_[1] |= next_pattern_[1];
	attribute_queue_[0] |= ((next_attribute_ >> 0) & 0x01) * 0xff;
	attribute_queue_[1] |= ((next_attribute_ >> 1) & 0x01) * 0xff;
}

void update_shift_registers_idle() {
	pattern_queue_[0] <<= 8;
	pattern_queue_[1] <<= 8;
	attribute_queue_[0] <<= 8;
	attribute_queue_[1] <<= 8;
	update_shift_registers_render();
}

void update_x_scroll() {
	vram_address_ = (vram_address_ & ~0b00000100'00011111) | (nametable_ & 0b00000100'00011111);
}

void update_sprite_registers() {
	sprite_address_ = 0;
}

void update_vram_address() {
	vram_address_ = (vram_address_ & ~0b01111011'11100000) | (nametable_ & 0b01111011'11100000);
}

bool rendering() {
	return vpos_ <= 240;
}

void increment_vram_address() {
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
void clock_ppu(const nes::ppu::scanline_prerender &) {

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
			case 5: evaluate_sprites_odd();  open_background_pattern<pattern_0>(); break;
			case 6: evaluate_sprites_even(); read_background_pattern<pattern_0>(); break;
			case 7: evaluate_sprites_odd();  open_background_pattern<pattern_1>(); break;
			case 0:
				evaluate_sprites_even();
				read_background_pattern<pattern_1>();
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
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_0>();
				else                           open_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_0>();
				else                           read_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_1>();
				else                           open_sprite_pattern<size_8px,  pattern_1>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_1>();
				else                           read_sprite_pattern<size_8px,  pattern_1>();
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
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_0>();
				else                           open_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_0>();
				else                           read_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_1>();
				else                           open_sprite_pattern<size_8px,  pattern_1>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_1>();
				else                           read_sprite_pattern<size_8px,  pattern_1>();
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
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_0>();
				else                           open_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_0>();
				else                           read_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_1>();
				else                           open_sprite_pattern<size_8px,  pattern_1>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_1>();
				else                           read_sprite_pattern<size_8px,  pattern_1>();
				break;
			}
		} else if (hpos_ < 337) {
			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5: open_background_pattern<pattern_0>(); break;
			case 6: read_background_pattern<pattern_0>(); break;
			case 7: open_background_pattern<pattern_1>(); break;
			case 0:
				read_background_pattern<pattern_1>();
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

void clock_ppu(const nes::ppu::scanline_render &target) {

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
			case 5: evaluate_sprites_odd();  open_background_pattern<pattern_0>(); break;
			case 6: evaluate_sprites_even(); read_background_pattern<pattern_0>(); break;
			case 7: evaluate_sprites_odd();  open_background_pattern<pattern_1>(); break;
			case 0:
				evaluate_sprites_even();
				read_background_pattern<pattern_1>();
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
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_0>();
				else                           open_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 6:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_0>();
				else                           read_sprite_pattern<size_8px,  pattern_0>();
				break;
			case 7:
				if (ppu_control_.large_sprites) open_sprite_pattern<size_16px, pattern_1>();
				else                           open_sprite_pattern<size_8px,  pattern_1>();
				break;
			case 0:
				if (ppu_control_.large_sprites) read_sprite_pattern<size_16px, pattern_1>();
				else                           read_sprite_pattern<size_8px,  pattern_1>();
				break;
			}
		} else if (hpos_ < 337) {

			switch (hpos_ & 0x07) {
			case 1: open_tile_index(); break;
			case 2: read_tile_index(); break;
			case 3: open_background_attribute(); break;
			case 4: read_background_attribute(); break;
			case 5: open_background_pattern<pattern_0>(); break;
			case 6: read_background_pattern<pattern_0>(); break;
			case 7: open_background_pattern<pattern_1>(); break;
			case 0:
				read_background_pattern<pattern_1>();
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

void clock_ppu(const nes::ppu::scanline_postrender &) {
	// no-op
}

void clock_ppu(const nes::ppu::scanline_vblank &) {

	// You kept this offset in your original code:
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

void start_frame() {
	vpos_ = 0;
	nes::apu::start_frame();
}

void end_frame() {

	odd_frame_ = !odd_frame_;

	latch_ += 0x100;
	if (latch_ > 0x3c00) {
		latch_ = 0;
	}

	nes::cart.mapper()->ppu_end_frame();
}

//------------------------------------------------------------------------------
// execute_scanline_impl (cpp-only)
//------------------------------------------------------------------------------
template <class T>
void execute_scanline_impl(const T &target) {

	if (UNLIKELY(vpos_ == 262)) {
		start_frame();
	} else if (UNLIKELY(vpos_ == 241)) {
		end_frame();
	}

	if (LIKELY(!nes::ppu::system_paused)) {
		for (hpos_ = 0; hpos_ < CyclesPerScanline; ++hpos_, ++ppu_cycle_) {
			clock_ppu(target);
			if ((ppu_cycle_ % 3) == CpuAlignment) {
				nes::cpu::exec<1>();
				nes::apu::exec<1>();
			}
		}
		++vpos_;
	}
}

} // end anonymous namespace

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
namespace nes::ppu {

void reset(nes::Reset reset_type) {

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

	std::cout << "PPU reset complete" << std::endl;
}

void write2000(uint8_t value) {

	latch_ = value;

	if (write_block_) {
		return;
	}

	const Control prev_control = ppu_control_;
	ppu_control_.raw           = value;

	nametable_ &= 0b1111001111111111;
	nametable_ |= ((value & 0b00000011) << 10);

	if (prev_control.nmi_on_vblank && !ppu_control_.nmi_on_vblank) {
		cpu::clear_nmi();
	} else if (!prev_control.nmi_on_vblank && ppu_control_.nmi_on_vblank && status_.vblank && hpos_ != 0) {
		cpu::nmi();
	}
}

void write2001(uint8_t value) {
	latch_ = value;

	if (write_block_) {
		return;
	}

	ppu_mask_.raw     = value;
	monochrome_mask_  = (ppu_mask_.monochrome) ? 0x30 : 0xff;
}

void write2002(uint8_t value) { latch_ = value; }

void write2003(uint8_t value) {
	latch_          = value;
	sprite_address_ = value;
}

void write2004(uint8_t value) {
	latch_ = value;
	sprite_ram_[sprite_address_++] = value;
}

void write2005(uint8_t value) {
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

void write2006(uint8_t value) {
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

void write2007(uint8_t value) {
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

uint8_t read200x() { return static_cast<uint8_t>(latch_); }

uint8_t read2002() {

	const uint8_t ret =
		((status_.raw & (StatusOverflow | StatusSprite0 | StatusVBlank)) |
		 (latch_ & ~(StatusOverflow | StatusSprite0 | StatusVBlank))) &
		0xff;

	write_latch_ = false;
	status_.vblank = false;

	ppu_read_2002_cycle_ = ppu_cycle_;
	return ret;
}

uint8_t read2004() {

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

uint8_t read2007() {

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

void write4014(uint8_t value) {
	const auto sprite_addr = static_cast<uint_least16_t>(value << 8);
	cpu::schedule_spr_dma(write2004, sprite_addr, 256);
}

//------------------------------------------------------------------------------
// Public scanline entry points (non-template, so other TUs can call them)
//------------------------------------------------------------------------------
void execute_scanline(const scanline_vblank &t)      { execute_scanline_impl(t); }
void execute_scanline(const scanline_prerender &t)   { execute_scanline_impl(t); }
void execute_scanline(const scanline_postrender &t)  { execute_scanline_impl(t); }
void execute_scanline(const scanline_render &t)      { execute_scanline_impl(t); }

//------------------------------------------------------------------------------
// Debug helpers
//------------------------------------------------------------------------------
scroll_state_t scroll_state() {
	scroll_state_t s{};
	s.v = static_cast<uint16_t>(vram_address_);
	s.t = static_cast<uint16_t>(nametable_);
	s.x = tile_offset_;
	s.ctrl = ppu_control_.raw;
	return s;
}

uint16_t vram_address() { return static_cast<uint16_t>(vram_address_); }
uint16_t temp_address() { return static_cast<uint16_t>(nametable_); }
uint8_t  fine_x()       { return tile_offset_; }

//------------------------------------------------------------------------------
// Misc getters
//------------------------------------------------------------------------------
uint64_t cycle_count() { return ppu_cycle_; }
uint_least16_t hpos()  { return hpos_; }
uint_least16_t vpos()  { return vpos_; }

uint8_t ppuctrl() { return ppu_control_.raw;	}
uint8_t ppumask() { return ppu_mask_.raw; 		}

uint8_t palette_ram(uint32_t address) {
	return palette_[address & 0x1f];
}

void set_palette_ram(uint32_t address, uint8_t data) {
	palette_[address & 0x1f] = data & 0x3f;
}

} // namespace nes::ppu
