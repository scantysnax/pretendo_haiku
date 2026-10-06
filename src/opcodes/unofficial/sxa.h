#ifndef SXA_20121206_H_
#define SXA_20121206_H_

//------------------------------------------------------------------------------
// Name: opcode_sxa
// Desc: SHX
//
//       Store:
//           X & (high byte of base address + 1)
//
//       On an indexed page crossing, the unstable NMOS 6502 behavior also
//       replaces the high byte of the destination address with the value
//       being stored.
//------------------------------------------------------------------------------
struct opcode_sxa {

	using memory_access = operation_write;

	static uint8_t execute(uint_least16_t &address) {

		const bool page_crossed = data16_.raw > 0xff;

		/*
		 * address is the corrected final effective address here.
		 *
		 * Recover the original high byte from before indexing if
		 * the index crossed a page.
		 */
		uint8_t high_byte =
			static_cast<uint8_t>((address >> 8) & 0xff);

		if (page_crossed) {
			--high_byte;
		}

		const uint8_t mask =
			static_cast<uint8_t>(high_byte + 1);

		const uint8_t value =
			static_cast<uint8_t>(X & mask);

		/*
		 * On a page crossing, SHX corrupts the destination high byte
		 * with the value being stored.
		 *
		 * Without a crossing, use the normal indexed effective address.
		 */
		if (page_crossed) {
			address =
				(address & 0x00ff) |
				(static_cast<uint_least16_t>(value) << 8);
		}

		return value;
	}
};

#endif
