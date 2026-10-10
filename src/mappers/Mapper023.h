#ifndef _MAPPER_023_H_
#define _MAPPER_023_H_

#include "VRC2.h"
#include "VRC4.h"


// -----------------------------------------------------------------------------
// Mapper23VRC2b
//
// NES 2.0 mapper 23, submapper 3.
//
// VRC2b uses CPU A1/A0 as its register-select address lines.  The VRC2 base
// class already decodes the canonical $x000-$x003 register arrangement, so no
// address translation is required here.
// -----------------------------------------------------------------------------
class Mapper23VRC2b final : public VRC2
{
	public:
	Mapper23VRC2b();

	public:
	std::string name() const override;
};


// -----------------------------------------------------------------------------
// Mapper23VRC4e
//
// NES 2.0 mapper 23, submapper 2.
//
// VRC4e uses CPU A3/A2 as its register-select address lines.  The VRC4 base
// class already decodes this arrangement as $x000/$x004/$x008/$x00C, so no
// address translation is required here.
// -----------------------------------------------------------------------------
class Mapper23VRC4e final : public VRC4
{
	public:
	Mapper23VRC4e();

	public:
	std::string name() const override;
};


// -----------------------------------------------------------------------------
// Mapper23VRC4f
//
// NES 2.0 mapper 23, submapper 1.
//
// VRC4f uses CPU A1/A0 for register selection.  Its writes are translated into
// the canonical VRC4e-style $x000/$x004/$x008/$x00C addresses expected by the
// VRC4 base class.
// -----------------------------------------------------------------------------
class Mapper23VRC4f final : public VRC4
{
	public:
	Mapper23VRC4f();

	public:
	std::string name() const override;

	public:
	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
	void write_f (uint_least16_t address, uint8_t value) override;
};


#endif 	// _MAPPER_023_H_
