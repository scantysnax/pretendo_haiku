
#ifndef VRAM_BANK_20110314_H_
#define VRAM_BANK_20110314_H_

#include <cstddef>
#include <cstdint>

class VRAMBank
{
	public:
	enum vram_type {
		ROM,
		RAM
	};

	public:
	VRAMBank()                      = default;
	VRAMBank(const VRAMBank &other) = default;
	VRAMBank &operator=(const VRAMBank &rhs) = default;

	VRAMBank &operator=(std::nullptr_t) {
		ptr_  = nullptr;
		type_ = ROM;
		return *this;
	}

	explicit VRAMBank(std::nullptr_t)
		: ptr_(nullptr), type_(ROM) {
	}

	VRAMBank(uint8_t *p, vram_type type)
		: ptr_(p), type_(type) {
	}

	public:
	uint8_t operator[](size_t n) const { return ptr_[n]; }
	uint8_t &operator[](size_t n) { return ptr_[n]; }
	explicit operator bool() const { return ptr_; }

	public:
	bool writeable() const { return type_ == RAM; }

	private:
	uint8_t *ptr_ = nullptr;
	vram_type type_ = ROM;
};


#endif

