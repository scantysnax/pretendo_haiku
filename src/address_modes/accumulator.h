#ifndef ACCUMULATOR_H_
#define ACCUMULATOR_H_

template <class Op>
class accumulator {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	// Accumulator instructions perform a dummy CPU read on their extra cycle.
	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		return PC.raw;
	}


private:

	static void execute(const operation_modify &) {
		switch (cycle_) {
		case 1:
			LAST_CYCLE;

			// read next instruction byte (and throw it away)
			read_byte(PC.raw);

			Op::execute(A);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
