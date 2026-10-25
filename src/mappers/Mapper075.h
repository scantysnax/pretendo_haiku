
#ifndef MAPPER075_20080314_H_
#define MAPPER075_20080314_H_

#include "Mapper.h"


// -----------------------------------------------------------------------------
// vrc1_debug_state_t
//
// Debugger-facing snapshot of VRC1-specific mapper state.
//
// Generic resolved PRG/CHR mappings are already exposed through the mapper
// debugger state.  This structure preserves the raw VRC1 CHR register values
// and control state used to produce those mappings.
// -----------------------------------------------------------------------------
struct vrc1_debug_state_t {
	uint8_t chr[2] = {};

	uint8_t control = 0;
	bool horizontal_mirroring = false;
};



class Mapper75 final : public Mapper
{
	public:
	Mapper75();

	public:
	std::string name() const override;

	public:
	void write_8(uint_least16_t address, uint8_t value) override;
	void write_9(uint_least16_t address, uint8_t value) override;
	void write_a(uint_least16_t address, uint8_t value) override;
	void write_c(uint_least16_t address, uint8_t value) override;
	void write_e(uint_least16_t address, uint8_t value) override;
	void write_f(uint_least16_t address, uint8_t value) override;
	
	public:
	vrc1_debug_state_t debug_mapper_state() const;
	
	private:
	uint8_t chr_reg_[2] = {};
	uint8_t control_ = 0;
};

#endif
