#ifndef ZERO_PAGE_H_
#define ZERO_PAGE_H_

template <class Op>
class zero_page {
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

	static bus_cycle_type bus_cycle(const operation_read &) {
		return bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_modify &) {
		return cycle_ >= 3
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_write &) {
		return cycle_ == 2
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static uint_least16_t bus_address(const operation_read &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_modify &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
		case 3:
		case 4:
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_write &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			// fetch address, increment PC
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			LAST_CYCLE;

			// read from effective address
			Op::execute(read_byte_zp(effective_address_.lo));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_modify &) {
		switch (cycle_) {
		case 1:
			// fetch address, increment PC
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			// read from effective address
			data8_ = read_byte_zp(effective_address_.lo);
			break;

		case 3:
			// write the value back to effective address,
			// and do the operation on it
			write_byte_zp(effective_address_.lo, data8_);
			Op::execute(data8_);
			break;

		case 4:
			LAST_CYCLE;

			// write the new value to effective address
			write_byte_zp(effective_address_.lo, data8_);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_write &) {
		switch (cycle_) {
		case 1:
			// fetch address, increment PC
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			LAST_CYCLE;

			// write register to effective address
			{
				const uint8_t address = effective_address_.lo;
				const uint8_t value = Op::execute(address);

				write_byte_zp(address, value);
			}

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}
};

#endif
