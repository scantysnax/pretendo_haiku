
#ifndef _MAPPER_085_H_
#define _MAPPER_085_H_

#include "VRC7.h"


// -----------------------------------------------------------------------------
// Mapper85VRC7a
//
// Implements the VRC7a hardware variant associated with mapper 85,
// NES 2.0 submapper 2.
//
// VRC7a uses CPU A4 for the secondary mapper-register selection, producing the
// canonical $x000/$x010 arrangement used directly by the VRC7 base class.
// -----------------------------------------------------------------------------
class Mapper85VRC7a final : public VRC7
{
	public:
	Mapper85VRC7a() = default;

	public:
	std::string name() const override;
};


// -----------------------------------------------------------------------------
// Mapper85VRC7b
//
// Implements the VRC7b hardware variant associated with mapper 85,
// NES 2.0 submapper 1.
//
// VRC7b uses CPU A3 for the secondary mapper-register selection.  Mapper
// register addresses are translated into the canonical $x000/$x010 layout
// expected by the VRC7 base implementation.
//
// The VRC7 audio ports remain at $9010/$9030 and are not translated.
// -----------------------------------------------------------------------------
class Mapper85VRC7b final : public VRC7
{
	public:
	Mapper85VRC7b() = default;

	public:
	std::string name() const override;

	public:
	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
	void write_f (uint_least16_t address, uint8_t value) override;
};


#endif	// _MAPPER_085_H_
