
#include "Cart.h"
#include "Mapper.h"
#include "iNES/Error.h"
#include <cstring>
#include <iomanip>
#include <iostream>

namespace {

// -----------------------------------------------------------------------------
// create_mask
//
// Creates a bit mask large enough to cover the supplied size.
//
// If the size is not already a power of two, it is rounded up to the next power
// of two.  The returned mask is one less than that power-of-two size, making it
// suitable for wrapping or masking addresses.
//
// Parameters:
//   size - Size for which to create the address mask.
//
// Returns:
//   Bit mask equal to the selected power-of-two size minus one.
// -----------------------------------------------------------------------------
uint32_t create_mask(uint32_t size) {

	// returns 1 less than closest fitting power of 2
	// is this number not a power of two or 0?
	if ((size & (size - 1)) != 0) {
		// yea, fix it!
		--size;
		size |= size >> 1;
		size |= size >> 2;
		size |= size >> 4;
		size |= size >> 8;
		size |= size >> 16;
		++size;
	} else if (size == 0) {
		++size;
	}

	return --size;
}


// -----------------------------------------------------------------------------
// is_power_of_2
//
// Determines whether the supplied size is a power of two.
//
// Parameters:
//   size - Value to test.
//
// Returns:
//   true if the supplied value is a power of two.
// -----------------------------------------------------------------------------
constexpr bool is_power_of_2(size_t size) {
	return (size & (size - 1)) == 0;
}


}


// -----------------------------------------------------------------------------
// Cart::load
//
// Loads an iNES ROM image from disk and initializes cartridge metadata.
//
// The ROM image is parsed, PRG and CHR address masks are created, mirroring and
// hash values are recorded, and the appropriate cartridge mapper is created.
// Any iNES loading error clears the partially initialized cartridge state.
//
// Parameters:
//   s - Path to the ROM image to load.
//
// Returns:
//   true if the ROM was loaded and initialized successfully.
// -----------------------------------------------------------------------------
bool Cart::load(const std::string &s) {

	std::cout << "[Cart::load] loading '" << s << "'...";

	try {
		filename_ = s;
		rom_      = std::make_unique<iNES::Rom>(s.c_str());

		std::cout << " OK!" << std::endl;

		// get mask values
		prg_mask_ = create_mask(rom_->prg_size());
		chr_mask_ = create_mask(rom_->chr_size());

		switch (rom_->header()->mirroring()) {
		case iNES::Mirroring::HORIZONTAL:
			mirroring_ = MIR_HORIZONTAL;
			break;
		case iNES::Mirroring::VERTICAL:
			mirroring_ = MIR_VERTICAL;
			break;
		case iNES::Mirroring::FOUR_SCREEN:
			mirroring_ = MIR_4SCREEN;
			break;
		default:
			mirroring_ = MIR_MAPPER;
			break;
		}

		prg_hash_ = rom_->prg_hash();
		chr_hash_ = rom_->chr_hash();
		rom_hash_ = rom_->rom_hash();

		std::cout << "PRG HASH: " << std::hex << std::setw(8) << std::setfill('0') << 								prg_hash_ << std::dec << std::endl;
		std::cout << "CHR HASH: " << std::hex << std::setw(8) << std::setfill('0') << 								chr_hash_ << std::dec << std::endl;
		std::cout << "ROM HASH: " << std::hex << std::setw(8) << std::setfill('0') << 								rom_hash_ << std::dec << std::endl;

		if (!is_power_of_2(rom_->prg_size())) {
			std::cout << "WARNING: PRG size is not a power of 2, this is unusual" << 					std::endl;
		}

		if (!is_power_of_2(rom_->chr_size())) {
			std::cout << "WARNING: CHR size is not a power of 2, this is unusual" << 					std::endl;
		}

		mapper_ = Mapper::create_mapper(rom_->header()->mapper());

		return true;
	} catch (const iNES::ines_error &e) {
		std::cout << " ERROR Loading ROM File! " << e.what() << std::endl;
		rom_      = nullptr;
		mapper_   = nullptr;
		prg_hash_ = 0;
		chr_hash_ = 0;
		rom_hash_ = 0;
		filename_.clear();
	}

	return false;
}


// -----------------------------------------------------------------------------
// Cart::unload
//
// Unloads the current cartridge and clears its ROM, mapper, and filename state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Cart::unload() {
	rom_    = nullptr;
	mapper_ = nullptr;
	filename_.clear();
}


// -----------------------------------------------------------------------------
// Cart::has_chr_rom
//
// Reports whether the loaded cartridge contains CHR ROM.
//
// Parameters:
//   None.
//
// Returns:
//   true if the loaded cartridge provides CHR ROM.
// -----------------------------------------------------------------------------
bool Cart::has_chr_rom() const {
	return rom_->chr_rom();
}


// -----------------------------------------------------------------------------
// Cart::prg_mask
//
// Returns the address mask used for PRG ROM accesses.
//
// Parameters:
//   None.
//
// Returns:
//   Current PRG address mask.
// -----------------------------------------------------------------------------
uint32_t Cart::prg_mask() const {
	return prg_mask_;
}


// -----------------------------------------------------------------------------
// Cart::chr_mask
//
// Returns the address mask used for CHR ROM accesses.
//
// Parameters:
//   None.
//
// Returns:
//   Current CHR address mask.
// -----------------------------------------------------------------------------
uint32_t Cart::chr_mask() const {
	return chr_mask_;
}


// -----------------------------------------------------------------------------
// Cart::prg
//
// Returns a pointer to the loaded cartridge's PRG ROM data.
//
// Parameters:
//   None.
//
// Returns:
//   Pointer to the PRG ROM byte array.
// -----------------------------------------------------------------------------
uint8_t *Cart::prg() const {
	return rom_->prg_rom();
}


// -----------------------------------------------------------------------------
// Cart::chr
//
// Returns a pointer to the loaded cartridge's CHR ROM data.
//
// Parameters:
//   None.
//
// Returns:
//   Pointer to the CHR ROM byte array, or nullptr when no CHR ROM is present.
// -----------------------------------------------------------------------------
uint8_t *Cart::chr() const {
	return rom_->chr_rom();
}


// -----------------------------------------------------------------------------
// Cart::mirroring
//
// Returns the cartridge's configured nametable mirroring mode.
//
// Parameters:
//   None.
//
// Returns:
//   Current cartridge mirroring mode.
// -----------------------------------------------------------------------------
Cart::Mirroring Cart::mirroring() const {
	return mirroring_;
}


// -----------------------------------------------------------------------------
// Cart::prg_hash
//
// Returns the stored hash of the cartridge PRG ROM data.
//
// Parameters:
//   None.
//
// Returns:
//   PRG ROM hash value.
// -----------------------------------------------------------------------------
uint32_t Cart::prg_hash() const {
	return prg_hash_;
}


// -----------------------------------------------------------------------------
// Cart::chr_hash
//
// Returns the stored hash of the cartridge CHR ROM data.
//
// Parameters:
//   None.
//
// Returns:
//   CHR ROM hash value.
// -----------------------------------------------------------------------------
uint32_t Cart::chr_hash() const {
	return chr_hash_;
}


// -----------------------------------------------------------------------------
// Cart::rom_hash
//
// Returns the stored hash of the complete cartridge ROM image.
//
// Parameters:
//   None.
//
// Returns:
//   ROM image hash value.
// -----------------------------------------------------------------------------
uint32_t Cart::rom_hash() const {
	return rom_hash_;
}


// -----------------------------------------------------------------------------
// Cart::raw_image
//
// Builds a contiguous copy of the cartridge ROM image from its PRG and CHR data.
//
// PRG data is copied first, followed by CHR data when CHR ROM is present.
//
// Parameters:
//   None.
//
// Returns:
//   Vector containing the combined PRG and CHR ROM image.
// -----------------------------------------------------------------------------
std::vector<uint8_t> Cart::raw_image() const {

	const uint8_t *const prg_rom = prg();
	const uint8_t *const chr_rom = chr();

	// create a vector and copy the PRG into it
	std::vector<uint8_t> image(prg_rom, prg_rom + rom_->prg_size());

	// if there is CHR, insert it at the end of the vector
	if (chr_rom) {
		image.insert(image.end(), chr_rom, chr_rom + rom_->chr_size());
	}

	return image;
}


// -----------------------------------------------------------------------------
// Cart::filename
//
// Returns the filename of the currently loaded cartridge ROM.
//
// Parameters:
//   None.
//
// Returns:
//   Reference to the stored ROM filename.
// -----------------------------------------------------------------------------
const std::string &Cart::filename() const {
	return filename_;
}


