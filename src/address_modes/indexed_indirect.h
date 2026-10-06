
#ifndef INDEXED_INDIRECT_H_
#define INDEXED_INDIRECT_H_

template <class Op>
class indexed_indirect {
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


	static uint_least16_t bus_address(const operation_read &) {
		switch (cycle_) {
		case 1:
			// Fetch zero-page pointer operand.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page pointer.
			return data8_;

		case 3:
			// Fetch effective-address low byte.
			return data8_;

		case 4:
			// Fetch effective-address high byte, wrapping in zero page.
			return static_cast<uint8_t>(data8_ + 1);

		case 5:
			// Read operand.
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_modify &) {
		switch (cycle_) {
		case 1:
			// Fetch zero-page pointer operand.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page pointer.
			return data8_;

		case 3:
			// Fetch effective-address low byte.
			return data8_;

		case 4:
			// Fetch effective-address high byte, wrapping in zero page.
			return static_cast<uint8_t>(data8_ + 1);

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
			// Fetch zero-page pointer operand.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page pointer.
			return data8_;

		case 3:
			// Fetch effective-address low byte.
			return data8_;

		case 4:
			// Fetch effective-address high byte, wrapping in zero page.
			return static_cast<uint8_t>(data8_ + 1);

		case 5:
			// Final write.
			return effective_address_.raw;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page pointer operand from the
			 * instruction stream.
			 */
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			/*
			 * (zp,X) performs a dummy read from the original
			 * zero-page pointer before applying X.
			 */
			read_byte_zp(data8_);

			data8_ = static_cast<uint8_t>(data8_ + X);
			break;

		case 3:
			// Fetch effective-address low byte.
			effective_address_.lo = read_byte_zp(data8_);
			break;

		case 4:
			/*
			 * Fetch effective-address high byte.  The pointer
			 * increment wraps within zero page.
			 */
			effective_address_.hi =
				read_byte_zp(static_cast<uint8_t>(data8_ + 1));
			break;

		case 5:
			LAST_CYCLE;

			// Read from effective address.
			Op::execute(read_byte(effective_address_.raw));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_modify &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page pointer operand from the
			 * instruction stream.
			 */
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			/*
			 * Dummy read from the unindexed zero-page pointer,
			 * then apply X.
			 */
			read_byte_zp(data8_);

			data8_ = static_cast<uint8_t>(data8_ + X);
			break;

		case 3:
			// Fetch effective-address low byte.
			effective_address_.lo = read_byte_zp(data8_);
			break;

		case 4:
			// Fetch effective-address high byte with zero-page wrap.
			effective_address_.hi =
				read_byte_zp(static_cast<uint8_t>(data8_ + 1));
			break;

		case 5:
			// Read original operand.
			data8_ = read_byte(effective_address_.raw);
			break;

		case 6:
			/*
			 * RMW dummy write of the original value,
			 * then perform the operation.
			 */
			write_byte(effective_address_.raw, data8_);
			Op::execute(data8_);
			break;

		case 7:
			LAST_CYCLE;

			// Write modified value.
			write_byte(effective_address_.raw, data8_);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_write &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page pointer operand from the
			 * instruction stream.
			 */
			data8_ = read_byte(PC.raw++);
			break;

		case 2:
			/*
			 * Dummy read from the unindexed zero-page pointer,
			 * then apply X.
			 */
			read_byte_zp(data8_);

			data8_ = static_cast<uint8_t>(data8_ + X);
			break;

		case 3:
			// Fetch effective-address low byte.
			effective_address_.lo = read_byte_zp(data8_);
			break;

		case 4:
			// Fetch effective-address high byte with zero-page wrap.
			effective_address_.hi =
				read_byte_zp(static_cast<uint8_t>(data8_ + 1));
			break;

		case 5:
			LAST_CYCLE;

			// Write to effective address.
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
