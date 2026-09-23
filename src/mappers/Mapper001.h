 
#ifndef MAPPER0001_20080314_H_
#define MAPPER0001_20080314_H_

#include "Mapper.h"


struct mmc1_debug_state_t {
	uint8_t shift_register = 0;
	uint8_t shift_count    = 0;

	uint8_t control        = 0;
	uint8_t chr_bank_0     = 0;
	uint8_t chr_bank_1     = 0;
	uint8_t prg_bank       = 0;

	uint8_t prg_mode       = 0;
	uint8_t chr_mode       = 0;

	bool prg_ram_enabled   = false;

	// Persistent serial-interface diagnostics.
	uint64_t serial_write_count    = 0;
	uint64_t register_commit_count = 0;
	uint64_t reset_count           = 0;

	// Last completed register transfer.
	bool have_last_commit = false;
	uint8_t last_register = 0;
	uint8_t last_value    = 0;

	// Five intermediate latch values from the most recently completed
	// serial transfer.
	bool have_last_transfer = false;
	uint8_t last_transfer[5] = {};
};


class Mapper1 final : public Mapper
{
	public:
	Mapper1();

	public:
	std::string name() const override;
	
	public:
	mmc1_debug_state_t debug_state() const;

	public:
	uint8_t read_6(uint_least16_t address) override;
	uint8_t read_7(uint_least16_t address) override;

	void write_6(uint_least16_t address, uint8_t value) override;
	void write_7(uint_least16_t address, uint8_t value) override;
	void write_8(uint_least16_t address, uint8_t value) override;
	void write_9(uint_least16_t address, uint8_t value) override;
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_b(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_d(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	void write_f(uint_least16_t address, uint8_t value) override;

	private:
	void write_handler(uint_least16_t address, uint8_t value);

	private:
	uint8_t chr_ram_[0x2000] = {};
	uint8_t regs_[4]         = {};
	uint64_t cpu_cycles_     = 0;
	uint8_t latch_           = 0;
	uint8_t write_counter_   = 0;
	uint8_t prg_ram_enable0_ = 0x10;
	uint8_t prg_ram_enable1_ = 0x10;

	private:
	uint64_t debug_serial_write_count_    = 0;
	uint64_t debug_register_commit_count_ = 0;
	uint64_t debug_reset_count_           = 0;

	private:
	bool debug_have_last_commit_ = false;
	uint8_t debug_last_register_ = 0;
	uint8_t debug_last_value_    = 0;

	private:
	uint8_t debug_shift_history_[5] = {};
	uint8_t debug_shift_history_count_ = 0;

	private:
	bool debug_have_last_transfer_ = false;
	uint8_t debug_last_transfer_[5] = {};

	private:
	MemoryMappedFile prg_ptr_;
};

#endif
