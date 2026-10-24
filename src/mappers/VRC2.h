
#ifndef _VRC2_H_
#define _VRC2_H_

#include "Mapper.h"


// -----------------------------------------------------------------------------
// vrc2_debug_state_t
//
// Debugger-facing snapshot of VRC2-specific mapper state.
//
// Generic resolved PRG/CHR mappings and nametable mirroring are already exposed
// through Mapper::debug_state().  This structure contains the raw VRC2 CHR
// registers, CHR shift mode, and one-bit $6000 latch state.
// -----------------------------------------------------------------------------
struct vrc2_debug_state_t {
	uint8_t chr[8] = {};

	uint8_t latch = 0;
	int chr_shift = 0;
	
	uint64_t latch_write_count = 0;
	uint64_t latch_change_count = 0;
};


class VRC2 : public Mapper
{
	public:
	explicit VRC2(int chr_shift);

	public:
	std::string name() const override;

	public:
	uint8_t read_6(uint_least16_t address) override;

	public:
	void write_6(uint_least16_t address, uint8_t value) override;
	void write_8(uint_least16_t address, uint8_t value) override;
	void write_9(uint_least16_t address, uint8_t value) override;
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_b(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_d(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	
	public:
	vrc2_debug_state_t debug_mapper_state() const;
	
	private:
	uint8_t chr_[8] = {};
	uint8_t latch_  = 0;
	int chr_shift_  = 0;

	uint64_t latch_write_count_ = 0;
	uint64_t latch_change_count_ = 0;
};

#endif	// _VRC2_H_
