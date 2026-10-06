#ifndef ABSOLUTE_H_
#define ABSOLUTE_H_

template <class Op>
class absolute {
public:

	// Dispatch to the appropriate version of the address mode.
	static void execute() {
		execute(typename Op::memory_access());
	}


	/*
	 * Reports the bus direction of the CPU cycle that is about to execute.
	 *
	 * DMC DMA uses this before executing the micro-cycle so that a DMA halt can
	 * be deferred across CPU write cycles.
	 */
	static bus_cycle_type bus_cycle() {
		return bus_cycle(typename Op::memory_access());
	}


	static uint_least16_t bus_address() {
		return bus_address(typename Op::memory_access());
	}


private:

	static bus_cycle_type bus_cycle(const operation_jump &) {
		return bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_read &) {
		return bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_modify &) {
		return cycle_ >= 4
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_write &) {
		return cycle_ == 3
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static uint_least16_t bus_address(const operation_jump &) {
		switch (cycle_) {
		case 1:
		case 2:
			return PC.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_read &) {
		switch (cycle_) {
		case 1:
		case 2:
			return PC.raw;

		case 3:
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_modify &) {
		switch (cycle_) {
		case 1:
		case 2:
			return PC.raw;

		case 3:
		case 4:
		case 5:
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_write &) {
		switch (cycle_) {
		case 1:
		case 2:
			return PC.raw;

		case 3:
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_jump &) {
		switch (cycle_) {
		case 1:
			// fetch low address byte, increment PC
			effective_address_.lo = read_byte(PC.raw++);
			break;

		case 2:
			LAST_CYCLE;

			// copy low address byte to PCL, fetch high address
			// byte to PCH
			effective_address_.hi = read_byte(PC.raw++);
			Op::execute(effective_address_.raw);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			// fetch low byte of address, increment PC
			effective_address_.lo = read_byte(PC.raw++);
			break;

		case 2:
			// fetch high byte of address, increment PC
			effective_address_.hi = read_byte(PC.raw++);
			break;

		case 3:
			LAST_CYCLE;

			// read from effective address
			Op::execute(read_byte(effective_address_.raw));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_modify &) {
		switch (cycle_) {
		case 1:
			// fetch low byte of address, increment PC
			effective_address_.lo = read_byte(PC.raw++);
			break;

		case 2:
			// fetch high byte of address, increment PC
			effective_address_.hi = read_byte(PC.raw++);
			break;

		case 3:
			// read from effective address
			data8_ = read_byte(effective_address_.raw);
			break;

		case 4:
			// write the value back to effective address,
			// and do the operation on it
			write_byte(effective_address_.raw, data8_);
			Op::execute(data8_);
			break;

		case 5:
			LAST_CYCLE;

			// write the new value to effective address
			write_byte(effective_address_.raw, data8_);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_write &) {
		switch (cycle_) {
		case 1:
			// fetch low byte of address, increment PC
			effective_address_.lo = read_byte(PC.raw++);
			break;

		case 2:
			// fetch high byte of address, increment PC
			effective_address_.hi = read_byte(PC.raw++);
			break;

		case 3:
			LAST_CYCLE;

			// write to effective address
			{
				const uint_least16_t address = effective_address_.raw;
				const uint8_t value = Op::execute(address);

				write_byte(address, value);
			}

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
