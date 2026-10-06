#ifndef RTI_20140417_H_
#define RTI_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_rti
// Desc: Return from Interrupt
//------------------------------------------------------------------------------
class opcode_rti {
public:

	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		switch (cycle_) {
		case 1:
			// Dummy read from the next instruction address.
			return PC.raw;

		case 2:
			// Internal stack cycle before S is incremented.
			return S + kStackAddress;

		case 3:
		case 4:
		case 5:
			// Pull P, PCL, and PCH from the stack.
			return S + kStackAddress;

		default:
			return PC.raw;
		}
	}


	static void execute() {
		switch (cycle_) {
		case 1:
			// read next instruction byte (and throw it away)
			read_byte(PC.raw);
			break;

		case 2:
			// increment S
			++S;
			break;

		case 3:
			// pull P from stack, increment S
			P = (read_byte(S++ + kStackAddress) & ~B_MASK) | R_MASK;
			break;

		case 4:
			// pull PCL from stack, increment S
			PC.lo = read_byte(S++ + kStackAddress);
			break;

		case 5:
			LAST_CYCLE;

			// pull PCH from stack
			PC.hi = read_byte(S + kStackAddress);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
