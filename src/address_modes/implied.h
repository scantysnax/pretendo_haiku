#ifndef IMPLIED_H_
#define IMPLIED_H_

template <class Op>
class implied {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		return PC.raw;
	}


private:

	template <class T>
	static void execute(T) {
		switch (cycle_) {
		case 1:
			LAST_CYCLE;

			// read next instruction byte (and throw it away)
			read_byte(PC.raw);
			Op::execute();

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	// Really this is only for NOP support...
	// synonym for other version.
	static void execute(const operation_read &) {
		execute(operation_none());
	}
};

#endif
