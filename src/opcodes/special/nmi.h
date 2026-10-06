#ifndef NMI_20140417_H_
#define NMI_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_nmi
// Desc: Non-Maskable Interrupt
//------------------------------------------------------------------------------
class opcode_nmi {
public:

	static bus_cycle_type bus_cycle() {
		switch (cycle_) {
		case 2:
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
			// Dummy/read cycle at the current PC.
			return PC.raw;

		case 2:
		case 3:
		case 4:
			// Stack pushes.
			return S + kStackAddress;

		case 5:
			// Fetch NMI vector low byte.
			return kNMIVectorAddress + 0;

		case 6:
			// Fetch NMI vector high byte.
			return kNMIVectorAddress + 1;

		default:
			return PC.raw;
		}
	}


	static void execute() {
		switch (cycle_) {
		case 1:
			// read next instruction byte (and throw it away),
			// increment PC
			read_byte(PC.raw);
			break;

		case 2:
			// push PCH on stack, decrement S
			write_byte(S-- + kStackAddress, PC.hi);
			break;

		case 3:
			// push PCL on stack, decrement S
			write_byte(S-- + kStackAddress, PC.lo);
			break;

		case 4:
			// push P on stack, decrement S
			write_byte(S-- + kStackAddress, P);
			break;

		case 5:
			set_flag<I_MASK>();

			// fetch PCL
			PC.lo = read_byte(kNMIVectorAddress + 0);
			break;

		case 6:
			LAST_CYCLE;

			// fetch PCH
			PC.hi = read_byte(kNMIVectorAddress + 1);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
