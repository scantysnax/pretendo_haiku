#ifndef RST_20140417_H_
#define RST_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_rst
// Desc: Reset
//------------------------------------------------------------------------------
class opcode_rst {
public:

	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		switch (cycle_) {
		case 1:
			// Read from current PC.
			return PC.raw;

		case 2:
		case 3:
		case 4:
			// Fake stack pushes during reset are actually reads.
			return S + kStackAddress;

		case 5:
			// Fetch reset vector low byte.
			return kRSTVectorAddress + 0;

		case 6:
			// Fetch reset vector high byte.
			return kRSTVectorAddress + 1;

		default:
			return PC.raw;
		}
	}


	static void execute() {
		switch (cycle_) {
		case 1:
			// read from current PC
			read_byte(PC.raw);
			break;

		case 2:
			// push PCH on stack, decrement S (fake)
			read_byte(S-- + kStackAddress);
			break;

		case 3:
			// push PCL on stack, decrement S (fake)
			read_byte(S-- + kStackAddress);
			break;

		case 4:
			// push P on stack, decrement S (fake)
			read_byte(S-- + kStackAddress);
			break;

		case 5:
			set_flag<I_MASK>();

			// fetch PCL
			PC.lo = read_byte(kRSTVectorAddress + 0);
			break;

		case 6:
			// fetch PCH
			LAST_CYCLE;

			PC.hi = read_byte(kRSTVectorAddress + 1);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
