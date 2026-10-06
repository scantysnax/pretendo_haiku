#ifndef INDIRECT_H_
#define INDIRECT_H_

template <class Op>
class indirect {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		switch (cycle_) {
		case 1:
		case 2:
			return PC.raw;

		case 3:
			return
				(static_cast<uint_least16_t>(data16_.hi) << 8)
				| data16_.lo;

		case 4:
			/*
			 * JMP ($xxFF) wraps within the same page on the 6502.
			 * execute() increments only data16_.lo before this read,
			 * so predict that address here without changing state.
			 */
			return
				(static_cast<uint_least16_t>(data16_.hi) << 8)
				| static_cast<uint8_t>(data16_.lo + 1);

		default:
			return PC.raw;
		}
	}


private:

	static void execute(const operation_jump &) {
		switch (cycle_) {
		case 1:
			// fetch pointer address low, increment PC
			data16_.lo = read_byte(PC.raw++);
			break;

		case 2:
			// fetch pointer address high, increment PC
			data16_.hi = read_byte(PC.raw++);
			break;

		case 3:
			// fetch low address byte
			effective_address_.lo =
				read_byte((data16_.hi << 8) | data16_.lo);
			break;

		case 4:
			LAST_CYCLE;

			// fetch PCH, copy latch to PCL
			++data16_.lo;

			effective_address_.hi =
				read_byte((data16_.hi << 8) | data16_.lo);

			// jump to effective address
			Op::execute(effective_address_.raw);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
