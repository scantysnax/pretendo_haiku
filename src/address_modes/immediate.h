#ifndef IMMEDIATE_H_
#define IMMEDIATE_H_

template <class Op>
class immediate {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	// Immediate-mode cycles are CPU reads.
	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		return PC.raw;
	}


private:

	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			LAST_CYCLE;

			// fetch value, increment PC
			Op::execute(read_byte(PC.raw++));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
