
#ifndef _SHIFT_REGISTER_H_
#define _SHIFT_REGISTER_H_


template <class T>
class ShiftRegister {
public:
	// -------------------------------------------------------------------------
	// ShiftRegister::ShiftRegister
	//
	// Constructs a shift register with the supplied initial value.
	//
	// Parameters:
	//   value - Initial register contents.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	explicit ShiftRegister(T value = 0)
		: data_(value) {
	}

	ShiftRegister(const ShiftRegister &other) = default;
	ShiftRegister &operator=(const ShiftRegister &rhs) = default;

	// -------------------------------------------------------------------------
	// ShiftRegister::load
	//
	// Loads a new value into the shift register.
	//
	// Parameters:
	//   value - Value to load.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void load(T value) {
		data_ = value;
	}

	// -------------------------------------------------------------------------
	// ShiftRegister::read
	//
	// Returns the current low-order bit and then advances the shift register by
	// one position.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Current low-order bit before the register is shifted.
	// -------------------------------------------------------------------------
	T read() {
		const T ret = value();
		clock();
		return ret;
	}

	// -------------------------------------------------------------------------
	// ShiftRegister::value
	//
	// Returns the current low-order bit without modifying the shift register.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Current low-order bit.
	// -------------------------------------------------------------------------
	T value() const {
		return (data_ & 0x1);
	}

	// -------------------------------------------------------------------------
	// ShiftRegister::clock
	//
	// Advances the shift register by one bit position toward the low end.
	//
	// Parameters:
	//   None.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	void clock() {
		data_ >>= 1;
	}

private:
	T data_ = 0;
};


#endif	// _SHIFT_REGISTER_H_

