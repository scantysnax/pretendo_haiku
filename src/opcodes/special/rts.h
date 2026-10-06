#ifndef RTS_20140417_H_
#define RTS_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_rts
// Desc: Return from Subroutine
//------------------------------------------------------------------------------
class opcode_rts {
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
			// Pull return address from stack.
			return S + kStackAddress;

		case 5:
			/*
			 * Final internal/read cycle before PC is incremented.
			 * The CPU is presenting the current return address.
			 */
			return PC.raw;

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
			// pull PCL from stack, increment S
			PC.lo = read_byte(S++ + kStackAddress);
			break;

		case 4:
			// pull PCH from stack
			PC.hi = read_byte(S + kStackAddress);
			break;

		case 5:
			LAST_CYCLE;

			// increment PC
			++PC.raw;

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
