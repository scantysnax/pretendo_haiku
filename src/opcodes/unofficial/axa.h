#ifndef AXA_20121206_H_
#define AXA_20121206_H_

//------------------------------------------------------------------------------
// Name: opcode_axa
// Desc: AHX/AXA
//
//       Store A & X & (high byte of base address + 1).
//
//       On an indexed page crossing, the unstable NMOS 6502 behavior also
//       replaces the high byte of the destination address with the value
//       being stored.
//------------------------------------------------------------------------------
struct opcode_axa {

	using memory_access = operation_write;

	static uint8_t execute(uint_least16_t &address) {

		const bool page_crossed = data16_.raw > 0xff;

		/*
		 * address is the corrected final effective address by the time
		 * this function is called.
		 *
		 * Recover the original high byte from before indexing when the
		 * index crossed a page.
		 */
		uint8_t high_byte =
			static_cast<uint8_t>((address >> 8) & 0xff);

		if (page_crossed) {
			--high_byte;
		}

		const uint8_t mask =
			static_cast<uint8_t>(high_byte + 1);

		const uint8_t value =
			static_cast<uint8_t>(A & X & mask);

		/*
		 * On a page crossing, AHX corrupts the destination high byte.
		 * Without a crossing, the normal effective address is used.
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
