
#ifndef ZERO_PAGE_X_H_
#define ZERO_PAGE_X_H_

template <class Op>
class zero_page_x {
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
		return cycle_ >= 4
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static bus_cycle_type bus_cycle(const operation_write &) {
		return cycle_ == 3
			? bus_cycle_type::write
			: bus_cycle_type::read;
	}


	static uint_least16_t bus_address(const operation_read &) {
		switch (cycle_) {
		case 1:
			// Fetch zero-page base address from instruction stream.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page address.
			return effective_address_.lo;

		case 3:
			// Final read from the indexed zero-page address.
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_modify &) {
		switch (cycle_) {
		case 1:
			// Fetch zero-page base address from instruction stream.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page address.
			return effective_address_.lo;

		case 3:
		case 4:
		case 5:
			// Operand read, dummy write, and final write all use
			// the indexed zero-page address.
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static uint_least16_t bus_address(const operation_write &) {
		switch (cycle_) {
		case 1:
			// Fetch zero-page base address from instruction stream.
			return PC.raw;

		case 2:
			// Dummy read from the unindexed zero-page address.
			return effective_address_.lo;

		case 3:
			// Final write to the indexed zero-page address.
			return effective_address_.lo;

		default:
			return PC.raw;
		}
	}


	static void execute(const operation_read &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page base address from the instruction
			 * stream and increment PC.
			 */
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			{
				/*
				 * Indexed zero-page addressing performs a dummy read
				 * from the unindexed zero-page address before applying X.
				 */
				const uint8_t base = effective_address_.lo;

				read_byte_zp(base);

				/*
				 * Apply X with zero-page wrapping.
				 */
				effective_address_.raw =
					static_cast<uint8_t>(base + X);
			}
			break;

		case 3:
			LAST_CYCLE;

			// Read from the indexed zero-page address.
			Op::execute(read_byte_zp(effective_address_.lo));

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_modify &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page base address from the instruction
			 * stream and increment PC.
			 */
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			{
				/*
				 * Perform the indexed zero-page dummy read from the
				 * unindexed address.
				 */
				const uint8_t base = effective_address_.lo;

				read_byte_zp(base);

				/*
				 * Apply X afterward, wrapping within zero page.
				 */
				effective_address_.raw =
					static_cast<uint8_t>(base + X);
			}
			break;

		case 3:
			// Read the original value from the indexed address.
			data8_ = read_byte_zp(effective_address_.lo);
			break;

		case 4:
			/*
			 * RMW instructions write the original value back first,
			 * then perform the modification.
			 */
			write_byte_zp(effective_address_.lo, data8_);
			Op::execute(data8_);
			break;

		case 5:
			LAST_CYCLE;

			// Write the modified value back.
			write_byte_zp(effective_address_.lo, data8_);

			OPCODE_COMPLETE;

		default:
			abort();
		}
	}


	static void execute(const operation_write &) {
		switch (cycle_) {
		case 1:
			/*
			 * Fetch the zero-page base address from the instruction
			 * stream and increment PC.
			 */
			effective_address_.raw = read_byte(PC.raw++);
			break;

		case 2:
			{
				/*
				 * Indexed zero-page stores perform the same dummy read
				 * from the unindexed address before applying X.
				 */
				const uint8_t base = effective_address_.lo;

				read_byte_zp(base);

				/*
				 * Apply X with zero-page wrapping.
				 */
				effective_address_.raw =
					static_cast<uint8_t>(base + X);
			}
			break;

		case 3:
			LAST_CYCLE;

			// Write to the indexed zero-page address.
			{
				const uint_least16_t address = effective_address_.lo;
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
