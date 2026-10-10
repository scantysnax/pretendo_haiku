
#ifndef _MAPPER_021_H_
#define _MAPPER_021_H_

#include "VRC4.h"


// -----------------------------------------------------------------------------
// Mapper21VRC4a
//
// Implements the VRC4a hardware variant associated with mapper 21.
//
// VRC4a uses CPU A2/A1 for register selection.  These addresses are translated
// into the canonical $x000/$x004/$x008/$x00C register arrangement expected by
// the VRC4 base implementation.
// -----------------------------------------------------------------------------
class Mapper21VRC4a final : public VRC4
{
	public:
	Mapper21VRC4a();

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
// Mapper21VRC4c
//
// Implements the VRC4c hardware variant associated with mapper 21.
//
// VRC4c uses CPU A7/A6 for register selection.  These addresses are translated
// into the canonical $x000/$x004/$x008/$x00C register arrangement expected by
// the VRC4 base implementation.
// -----------------------------------------------------------------------------
class Mapper21VRC4c final : public VRC4
{
	public:
	Mapper21VRC4c();

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


#endif	// _MAPPER_021_H_

