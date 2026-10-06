#ifndef STACK_H_
#define STACK_H_

template <class Op>
class stack {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	static bus_cycle_type bus_cycle() {
		return bus_cycle(typename Op::memory_access());
	}


	static uint_least16_t bus_address() {
		return bus_address(typename Op::memory_access());
	}


private:

	static bus_cycle_type bus_cycle(const operation_stack_read &) {
		return bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_stack_write &) {
		return cycle_ == 2
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static uint_least16_t bus_address(const operation_stack_read &) {
		switch (cycle_) {
		case 1:
			/*
			 * Dummy read of the next instruction byte.
			 */
			return PC.raw;

		case 2:
			/*
			 * Dummy stack read before S is incremented.
			 */
			return S + kStackAddress;

		case 3:
			/*
			 * Actual stack pull.  S was incremented during cycle 2.
			 */
			return S + kStackAddress;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_stack_write &) {
		switch (cycle_) {
		case 1:
			/*
			 * Dummy read of the next instruction byte.
			 */
			return PC.raw;

		case 2:
			/*
			 * Actual stack push.
			 */
			return S + kStackAddress;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_stack_read &) {
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
			LAST_CYCLE;

			// pull register from stack
			Op::execute(read_byte(S + kStackAddress));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_stack_write &) {
		switch (cycle_) {
		case 1:
			// read next instruction byte (and throw it away)
			read_byte(PC.raw);
			break;

		case 2:
			LAST_CYCLE;

			// push register on stack, decrement S
			write_byte(S-- + kStackAddress, Op::execute());

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
