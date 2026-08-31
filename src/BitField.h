#ifndef _BITFIELD_H_
#define _BITFIELD_H_

#include <cstddef>
#include <cstdint>

template <class T, size_t Index, size_t Bits = 1>
class BitField {
private:
	enum {
		Mask = (1u << Bits) - 1u
	};

public:
	// -------------------------------------------------------------------------
	// BitField::operator=
	//
	// Assigns the value of another bit field to this field.
	//
	// Only the bits represented by this BitField are copied; the remaining bits
	// in the underlying storage value are preserved.
	//
	// Parameters:
	//   rhs - Bit field whose value is copied.
	//
	// Returns:
	//   Reference to this BitField.
	// -------------------------------------------------------------------------
	BitField &operator=(const BitField &rhs) {
		value_ = (value_ & ~Mask) | (rhs.value_ & Mask);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BitField::operator=
	//
	// Assigns a value to this bit field.
	//
	// The supplied value is masked to the field width and inserted at the
	// configured bit position while preserving all unrelated storage bits.
	//
	// Parameters:
	//   value - Value to store in the bit field.
	//
	// Returns:
	//   Reference to this BitField.
	// -------------------------------------------------------------------------
	template <class T2>
	BitField &operator=(T2 value) {
		value_ = (value_ & ~(Mask << Index)) | ((value & Mask) << Index);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BitField::operator T
	//
	// Extracts the current bit-field value from the underlying storage.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Current field value shifted down to bit zero.
	// -------------------------------------------------------------------------
	operator T() const {
		return (value_ >> Index) & Mask;
	}

	// -------------------------------------------------------------------------
	// BitField::operator bool
	//
	// Tests whether any bit within the field is currently set.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if one or more bits in the field are set.
	// -------------------------------------------------------------------------
	explicit operator bool() const {
		return (value_ & (Mask << Index)) != 0;
	}

	// -------------------------------------------------------------------------
	// BitField::operator++
	//
	// Increments the current bit-field value and stores the result back into the
	// field.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Reference to this BitField after incrementing.
	// -------------------------------------------------------------------------
	BitField &operator++() {
		return *this = *this + 1;
	}

	// -------------------------------------------------------------------------
	// BitField::operator++
	//
	// Post-increments the current bit-field value.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Field value before the increment.
	// -------------------------------------------------------------------------
	T operator++(int) {
		T r = *this;
		++*this;
		return r;
	}

	// -------------------------------------------------------------------------
	// BitField::operator--
	//
	// Decrements the current bit-field value and stores the result back into the
	// field.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Reference to this BitField after decrementing.
	// -------------------------------------------------------------------------
	BitField &operator--() {
		return *this = *this - 1;
	}

	// -------------------------------------------------------------------------
	// BitField::operator--
	//
	// Post-decrements the current bit-field value.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Field value before the decrement.
	// -------------------------------------------------------------------------
	T operator--(int) {
		T r = *this;
		++*this;
		return r;
	}

private:
	T value_;
};


template <class T, size_t Index>
class BitField<T, Index, 1> {
private:
	enum {
		Bits = 1,
		Mask = 0x01
	};

public:
	// -------------------------------------------------------------------------
	// BitField::operator=
	//
	// Assigns a boolean value to this single-bit field.
	//
	// The selected bit is updated while all unrelated bits in the underlying
	// storage value are preserved.
	//
	// Parameters:
	//   value - Boolean value to store.
	//
	// Returns:
	//   Reference to this BitField.
	// -------------------------------------------------------------------------
	BitField &operator=(bool value) {
		value_ = (value_ & ~(Mask << Index)) | (value << Index);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BitField::operator bool
	//
	// Returns the current state of this single-bit field.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the selected bit is set.
	// -------------------------------------------------------------------------
	operator bool() const {
		return (value_ & (Mask << Index)) != 0;
	}

	// -------------------------------------------------------------------------
	// BitField::operator!
	//
	// Tests whether this single-bit field is clear.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   true if the selected bit is not set.
	// -------------------------------------------------------------------------
	bool operator!() const {
		return (value_ & (Mask << Index)) == 0;
	}

private:
	T value_;
};

#endif	// _BITFIELD_H_

