#ifndef JSR_20140417_H_
#define JSR_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_jsr
// Desc: Jump to Subroutine
//------------------------------------------------------------------------------
class opcode_jsr {
public:

	static bus_cycle_type bus_cycle() {
		switch (cycle_) {
		case 3:
		case 4:
			return bus_cycle_type::write;

		default:
			return bus_cycle_type::read;
		}
	}


	static uint_least16_t bus_address() {
		switch (cycle_) {
		case 1:
			// Fetch low target byte.
			return PC.raw;

		case 2:
			/*
			 * Internal/dummy read cycle.  The CPU is still presenting
			 * the current PC address.
			 */
			return PC.raw;

		case 3:
		case 4:
			// Stack pushes.
			return S + kStackAddress;

		case 5:
			// Fetch high target byte.
			return PC.raw;

		default:
			return PC.raw;
		}
	}


	static void execute() {
		switch (cycle_) {
		case 1:
			// fetch low address byte, increment PC
			effective_address_.lo = read_byte(PC.raw++);
			break;

		case 2:
			// internal operation (predecrement S?)
			break;

		case 3:
			// push PCH on stack, decrement S
			write_byte(S-- + kStackAddress, PC.hi);
			break;

		case 4:
			// push PCL on stack, decrement S
			write_byte(S-- + kStackAddress, PC.lo);
			break;

		case 5:
			LAST_CYCLE;

			// fetch high address byte to PCH
			effective_address_.hi = read_byte(PC.raw);
			PC.raw = effective_address_.raw;

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
