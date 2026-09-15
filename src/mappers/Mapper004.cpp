#include "Mapper004.h"
#include "Cart.h"
#include "Nes.h"


SETUP_STATIC_INES_MAPPER_REGISTRAR(4)


//------------------------------------------------------------------------------
// Name: Mapper4
//
// Initializes mapper 4 (Nintendo MMC3).
//
// The common MMC3 implementation is provided by the MMC3 base class. This
// derived class selects between the two MMC3 IRQ-counter behaviors used by
// known cartridge revisions.
//
// A small set of ROM hashes is treated as MMC3A-compatible and uses ModeA.
// All other mapper-4 cartridges default to ModeB.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper4::Mapper4()
{
	switch (nes::cart.rom_hash()) {
	case 0xf312d1de:
	case 0xa512bdf6:
	case 0x633afe6f:
	case 0x1335cb05:
		// These known ROMs require MMC3A-style IRQ-counter behavior.
		mode_ = ModeA;
		break;

	default:
		// Most mapper-4 cartridges use MMC3B-style IRQ-counter behavior.
		mode_ = ModeB;
		break;
	}
}

