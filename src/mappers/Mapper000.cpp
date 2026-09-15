#include "Mapper000.h"
#include "Cart.h"
#include "Nes.h"

SETUP_STATIC_INES_MAPPER_REGISTRAR(0)


//------------------------------------------------------------------------------
// Name: Mapper0
//
// Initializes mapper 0 (NROM), the simplest NES cartridge mapping.
//
// NROM does not perform runtime bank switching. PRG ROM is mapped once at
// startup, and the cartridge either supplies CHR ROM or uses 8 KB of CHR RAM.
//
// The PRG helpers apply the cartridge's PRG mask, so both common NROM layouts
// are handled:
//
//   NROM-128:
//     16 KB PRG ROM mirrored into both $8000-$BFFF and $C000-$FFFF.
//
//   NROM-256:
//     32 KB PRG ROM mapped directly across $8000-$FFFF.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
//------------------------------------------------------------------------------
Mapper0::Mapper0()
{
	// Map the first 16 KB PRG bank into $8000-$BFFF.
	set_prg_89ab(0);

	// Map the final 16 KB PRG bank into $C000-$FFFF.
	//
	// For a 16 KB NROM cartridge, PRG masking causes this region to mirror
	// the same physical PRG ROM used at $8000-$BFFF.
	set_prg_cdef(-1);

	if (nes::cart.has_chr_rom()) {
		// NROM with CHR ROM maps the fixed 8 KB CHR bank at $0000-$1FFF.
		set_chr_0000_1fff(0);
	} else {
		// NROM cartridges without CHR ROM use 8 KB of CHR RAM instead.
		set_chr_0000_1fff_ram(chr_ram_, 0);
	}
}

//------------------------------------------------------------------------------
// Name: name
//
// Returns the human-readable mapper name.
//
// Parameters:
//   None.
//
// Returns:
//   "NROM".
//------------------------------------------------------------------------------
std::string
Mapper0::name() const
{
	return "NROM";
}
