#ifndef INDIRECT_INDEXED_H_
#define INDIRECT_INDEXED_H_

template <class Op>
class indirect_indexed {
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
		return cycle_ >= 6
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_write &) {
		return cycle_ == 5
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static uint_least16_t indexed_address() {
		return
			(static_cast<uint_least16_t>(effective_address_.hi) << 8)
			| data16_.lo;
	}


	static uint_least16_t bus_address(const operation_read &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
			return data8_;

		case 3:
			return static_cast<uint8_t>(data8_ + 1);

		case 4:
			/*
			 * First indexed access.  This uses the original high byte
			 * together with the Y-adjusted low byte.
			 */
			return indexed_address();

		case 5:
			/*
			 * Page-crossing retry using the corrected address.
			 */
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_modify &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
			return data8_;

		case 3:
			return static_cast<uint8_t>(data8_ + 1);

		case 4:
			return indexed_address();

		case 5:
		case 6:
		case 7:
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_write &) {
		switch (cycle_) {
		case 1:
			return PC.raw;

		case 2:
			return data8_;

		case 3:
			return static_cast<uint8_t>(data8_ + 1);

		case 4:
			return indexed_address();

		case 5:
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			// fetch pointer address, increment PC
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			// fetch effective address low
			data16_.raw = read_byte_zp(data8_ + 0);
			break;

		case 3:
			// fetch effective address high,
			// add Y to low byte of effective address
			effective_address_.hi = read_byte_zp(data8_ + 1);
			data16_.raw += Y;
			break;

		case 4:
			// read from effective address,
			// fix high byte of effective address
			effective_address_.lo = data16_.lo;
			data8_ = read_byte(effective_address_.raw);

			if (data16_.raw > 0xff) {
				++effective_address_.hi;
				break;
			} else {
				LAST_CYCLE;

				Op::execute(data8_);

				OPCODE_COMPLETE;
			}

		case 5:
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
			// fetch pointer address, increment PC
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			// fetch effective address low
			data16_.raw = read_byte_zp(data8_ + 0);
			break;

		case 3:
			// fetch effective address high,
			// add Y to low byte of effective address
			effective_address_.hi = read_byte_zp(data8_ + 1);
			data16_.raw += Y;
			break;

		case 4:
			// read from effective address,
			// fix high byte of effective address
			effective_address_.lo = data16_.lo;
			data8_ = read_byte(effective_address_.raw);

			if (data16_.raw > 0xff) {
				++effective_address_.hi;
			}
			break;

		case 5:
			// read from effective address
			data8_ = read_byte(effective_address_.raw);
			break;

		case 6:
			// write the value back to effective address,
			// and do the operation on it
			write_byte(effective_address_.raw, data8_);
			Op::execute(data8_);
			break;

		case 7:
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
			// fetch pointer address, increment PC
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			// fetch effective address low
			data16_.raw = read_byte_zp(data8_ + 0);
			break;

		case 3:
			// fetch effective address high,
			// add Y to low byte of effective address
			effective_address_.hi = read_byte_zp(data8_ + 1);
			data16_.raw += Y;
			break;

		case 4:
			// read from effective address,
			// fix high byte of effective address
			effective_address_.lo = data16_.lo;
			data8_ = read_byte(effective_address_.raw);

			if (data16_.raw > 0xff) {
				++effective_address_.hi;
			}
			break;

		case 5:
			LAST_CYCLE;

			// write to effective address
			{
				uint_least16_t address = effective_address_.raw;
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
