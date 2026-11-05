
#ifndef _MAPPER_005_H_
#define _MAPPER_005_H_

#include "BitField.h"
#include "Mapper.h"


struct mmc5_debug_state_t {
	uint8_t prg_mode = 0;
	uint8_t chr_mode = 0;

	uint8_t prg_bank[5] = {};

	uint8_t bg_chr_bank[8] = {};
	uint8_t sp_chr_bank[8] = {};

	uint16_t bg_char_upper = 0;
	bool last_chr_write_bg = false;

	uint8_t prg_ram_protect1 = 0;
	uint8_t prg_ram_protect2 = 0;

	uint8_t mirroring_mode = 0;
	uint8_t exram_mode = 0;

	uint8_t fill_mode_tile = 0;
	uint8_t fill_mode_attr = 0;

	uint8_t vertical_split_mode = 0;
	uint8_t vertical_split_scroll = 0;
	uint8_t vertical_split_bank = 0;

	bool large_sprites = false;
	uint16_t fetch_count = 0;

	bool irq_enabled = false;
	uint8_t irq_counter = 0;
	uint8_t irq_target = 0;
	bool irq_in_frame = false;
	bool irq_pending = false;

	uint8_t multiplier_1 = 0;
	uint8_t multiplier_2 = 0;
	uint16_t multiplier_result = 0;
};


// -----------------------------------------------------------------------------
// mmc5_pulse_channel_t
//
// Stores the internal state of one MMC5 pulse-audio channel.
//
// The two MMC5 pulse channels use the same basic duty, envelope, timer, and
// length-counter organization as the native NES pulse channels.  The fields
// here are kept mapper-local so MMC5 audio can be clocked independently from
// the native APU.
//
// Muting affects only the final expansion-audio mix and does not stop channel
// state from advancing.
// -----------------------------------------------------------------------------
struct mmc5_pulse_channel_t {
	uint8_t duty = 0;
	uint8_t volume = 0;

	bool constant_volume = false;
	bool length_halt = false;
	bool enabled = false;
	bool muted = false;

	uint16_t period = 0;
	uint16_t timer = 0;

	uint8_t sequence_step = 0;
	uint8_t length_counter = 0;

	bool envelope_start = false;
	uint8_t envelope_divider = 0;
	uint8_t envelope_decay = 0;
};


// -----------------------------------------------------------------------------
// mmc5_pcm_channel_t
//
// Stores the internal state of the MMC5 PCM audio channel.
//
// The channel supports direct PCM output and the MMC5 read-mode/IRQ control
// state.  Muting affects only the final expansion-audio mix.
// -----------------------------------------------------------------------------
struct mmc5_pcm_channel_t {
	uint8_t value = 0;

	bool read_mode = false;
	bool irq_enabled = false;
	bool irq_pending = false;

	bool enabled = true;
	bool muted = false;
};


class Mapper5 final : public Mapper
{
	private:
	union irq_status_t {
		uint8_t raw;
		BitField<uint8_t, 6> in_frame;
		BitField<uint8_t, 7> pending;
	};

	public:
	Mapper5();

	public:
	std::string name() const override;

	public:
	void write_2 (uint_least16_t address, uint8_t value) override;
	void write_3 (uint_least16_t address, uint8_t value) override;
	void write_5 (uint_least16_t address, uint8_t value) override;
	void write_6 (uint_least16_t address, uint8_t value) override;
	void write_7 (uint_least16_t address, uint8_t value) override;
	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;

	public:
	uint8_t read_5 (uint_least16_t address) override;
	uint8_t read_6 (uint_least16_t address) override;
	uint8_t read_7 (uint_least16_t address) override;
	uint8_t read_8 (uint_least16_t address) override;
	uint8_t read_9 (uint_least16_t address) override;
	uint8_t read_a (uint_least16_t address) override;
	uint8_t read_b (uint_least16_t address) override;
	uint8_t read_c (uint_least16_t address) override;
	uint8_t read_d (uint_least16_t address) override;

	public:
	uint8_t read_vram (uint_least16_t address) override;
	void write_vram (uint_least16_t address, uint8_t value) override;
	void set_ppu_fetch_type (PPUFetchType type) override
	{
		ppu_fetch_type_ = type;
	}

	public:
	void vram_change_hook (uint_least16_t vram_address) override;
	void ppu_end_frame() override;
	void cpu_sync() override;

	private:
	void clock_irq();
	void write_handler (uint_least16_t address, uint8_t value);
	uint8_t read_handler (uint_least16_t address);
	void update_prg_mapping();
	void update_irq_line();
	uint8_t read_pcm_memory (uint_least16_t address);
	
	public:
	void debug_update_chr_mapping();
	
	private:
	void clock_audio_pulse (mmc5_pulse_channel_t &channel);
	uint8_t audio_pulse_output (const mmc5_pulse_channel_t &channel) const;
	void clock_audio_frame();
	void clock_audio_envelope (mmc5_pulse_channel_t &channel);
	void clock_audio_length (mmc5_pulse_channel_t &channel);
	
	public:
	size_t audio_channel_count() const override;
	const char *audio_channel_name (size_t index) const override;
	bool audio_channel_muted (size_t index) const override;
	void set_audio_channel_muted (size_t index, bool muted) override;
	uint8_t audio_output() const override;
	
	mapper_audio_scope_sample_t debug_audio_scope_sample() const override;
	mapper_audio_debug_state_t debug_audio_state() const override;
	
	private:
	void update_vertical_split (uint_least16_t address);

	private:
	uint8_t prg_ram_[2][0x8000]    		 = {};
	uint8_t exram_[0x400]          		 = {};
	uint8_t bg_chr_banks_[8]       		 = {};
	uint8_t sp_chr_banks_[8]       		 = {};
	uint_least16_t prev_vram_address_[2] = {0xffff, 0xffff};
	uint8_t *prg_ram_banks_[0x10]  		 = {};

	private:
	bool irq_enabled_              		= false;
	bool large_sprites_            		= false;
	uint16_t bg_char_upper_        		= 0x00;
	uint16_t fetch_count_          		= 0;
	PPUFetchType ppu_fetch_type_   		= PPUFetchType::NONE;
	uint8_t chr_mode_              		= 0;
	uint8_t exram_mode_            		= 0;
	uint8_t fill_mode_attr_        		= 0;
	uint8_t fill_mode_tile_        		= 0;
	uint8_t irq_counter_           		= 0;
	irq_status_t irq_status_       		= {0};
	uint8_t irq_target_            		= 0;
	uint8_t mirroring_mode_        		= 0;
	uint8_t multiplier_1_          		= 0;
	uint8_t multiplier_2_          		= 0;
	uint8_t prg_mode_              		= 0x03;
	uint8_t prg_ram_protect1_      		= 0;
	uint8_t prg_ram_protect2_      		= 0;
	uint8_t vertical_split_mode_   		= 0;
	uint8_t vertical_split_scroll_ 		= 0;
	uint8_t vertical_split_bank_   		= 0;
	bool vertical_split_active_	   		= false;
	uint8_t vertical_split_tile_number_ = 0;
	uint8_t vertical_split_y_ 			= 0;
	uint16_t vertical_split_tile_ 		= 0;
	uint8_t extended_attribute_    		= 0;
	
	enum {
		CHR_BANK_A,
		CHR_BANK_B
	} last_chr_write_ = CHR_BANK_A;
	
	public:
	mmc5_debug_state_t debug_state() const;
	
	private:
	// Initial PRG bank selections.
	//
	// $5113 selects PRG-RAM at $6000-$7FFF.
	// $5114-$5117 default to ROM selections until programmed by the game.
	//
	// Bit 7 selects PRG-ROM for the switchable banks.
	// Bank $7F resolves through the cartridge's PRG-ROM mask.
	uint8_t prg_bank_[5] = {
    	0x00,
    	0xff,
    	0xff,
    	0xff,
    	0xff
	};

	mmc5_pulse_channel_t pulse_[2];
	mmc5_pcm_channel_t pcm_;

	bool audio_half_clock_ = false;
	uint16_t audio_frame_counter_ = 7424;
};


#endif	// _MAPPER_005_H_

