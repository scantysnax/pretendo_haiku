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

// -----------------------------------------------------------------------------
// implied::execute
//
// Executes an implied-addressing instruction.
//
// The final instruction cycle checks for pending interrupts, performs
// the implied-addressing dummy read, executes the operation, and marks
// the instruction complete.
//
// Parameters:
//   T - Operation memory-access category.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
template <class T>
static void execute(T) {
	switch (cycle_) {
		case 1:
			// Sample pending interrupts at the instruction boundary.
			LAST_CYCLE;

			// Read the next instruction byte and discard it.
			read_byte(PC.raw);

			// Execute the implied operation.
			Op::execute();

			// Mark the instruction complete.
			OPCODE_COMPLETE;

		default:
			abort();
	}
}


// -------------------------------------------------------------------------
// implied::execute
//
// Handles operations that use the operation_read memory-access category.
//
// For implied-addressing instructions, this is equivalent to the
// operation_none implementation.
//
// Parameters:
//   operation_read - Memory-access category selector.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------------------
static void execute(const operation_read &) {
		execute(operation_none());
	}
};

#endif
