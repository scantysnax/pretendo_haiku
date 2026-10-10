#ifndef _MAPPER_025_H_
#define _MAPPER_025_H_

#include "VRC2.h"
#include "VRC4.h"


// -----------------------------------------------------------------------------
// Mapper25VRC2c
//
// Implements the VRC2c hardware variant associated with mapper 25.
//
// VRC2c uses CPU A0/A1 for register selection, with the two address lines
// reversed relative to the canonical ordering used by the VRC2 base class.
// -----------------------------------------------------------------------------
class Mapper25VRC2c final : public VRC2 
{
	public:
	Mapper25VRC2c();

	public:
	std::string name() const override;

	public:
	uint8_t read_6 (uint_least16_t address) override;

	void write_6 (uint_least16_t address, uint8_t value) override;

	void write_8 (uint_least16_t address, uint8_t value) override;
	void write_9 (uint_least16_t address, uint8_t value) override;
	void write_a (uint_least16_t address, uint8_t value) override;
	void write_b (uint_least16_t address, uint8_t value) override;
	void write_c (uint_least16_t address, uint8_t value) override;
	void write_d (uint_least16_t address, uint8_t value) override;
	void write_e (uint_least16_t address, uint8_t value) override;
};


// -----------------------------------------------------------------------------
// Mapper25VRC4b
//
// Implements the VRC4b hardware variant associated with mapper 25.
//
// VRC4b uses CPU A0/A1 for register selection.  These addresses are translated
// into the canonical $x000/$x004/$x008/$x00C register arrangement expected by
// the VRC4 base implementation.
// -----------------------------------------------------------------------------
class Mapper25VRC4b final : public VRC4
{
	public:
	Mapper25VRC4b();

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


// -----------------------------------------------------------------------------
// Mapper25VRC4d
//
// Implements the VRC4d hardware variant associated with mapper 25.
//
// VRC4d uses CPU A2/A3 for register selection, with those address lines
// reversed relative to the canonical VRC4e-style ordering used by VRC4.
// -----------------------------------------------------------------------------
class Mapper25VRC4d final : public VRC4
{
	public:
	Mapper25VRC4d();

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


#endif 	// _MAPPER_025_H_
