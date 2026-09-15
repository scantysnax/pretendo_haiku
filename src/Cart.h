#ifndef _CART_H_
#define _CART_H_

#include "Mapper.h"
#include "iNES/Rom.h"

#include <memory>
#include <string>
#include <vector>


// Loaded NES cartridge image, mapper, ROM data, hashes, and mirroring state.
class Cart {
	public:
	enum cart_mirroring {
		MIR_VERTICAL,
		MIR_HORIZONTAL,
		MIR_SINGLE_HIGH,
		MIR_SINGLE_LOW,
		MIR_4SCREEN,
		MIR_MAPPER
	};

	public:
	const std::string &filename() const;
	bool load(const std::string &s);
	void unload();
	bool has_chr_rom() const;
	uint32_t prg_mask() const;
	uint32_t chr_mask() const;
	uint32_t prg_hash() const;
	uint32_t chr_hash() const;
	uint32_t rom_hash() const;
	uint8_t *prg() const;
	uint8_t *chr() const;
	cart_mirroring mirroring() const;
	Mapper *mapper() const { return mapper_.get(); }
	std::vector<uint8_t> raw_image() const;

	private:
	// Parsed ROM image and derived PRG/CHR metadata.
	std::unique_ptr<iNES::Rom> rom_;
	uint32_t prg_mask_   = 0;
	uint32_t chr_mask_   = 0;
	uint32_t prg_hash_   = 0;
	uint32_t chr_hash_   = 0;
	uint32_t rom_hash_   = 0;

	// Cartridge mapping configuration and source filename.
	cart_mirroring mirroring_ = MIR_HORIZONTAL;
	std::unique_ptr<Mapper> mapper_;
	std::string filename_;
};


#endif // _CART_H_

