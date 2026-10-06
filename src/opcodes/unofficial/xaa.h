#ifndef XAA_20121206_H_
#define XAA_20121206_H_

//------------------------------------------------------------------------------
// Name: opcode_xaa
// Desc: XAA/ANE
//
//       Unstable undocumented opcode.
//
//       A commonly used deterministic model is:
//
//           A = (A | magic) & X & immediate
//
//       The real NMOS 6502 behavior is analog-dependent and the effective
//       magic value can vary between chips and operating conditions.
//------------------------------------------------------------------------------
struct opcode_xaa {

	using memory_access = operation_read;

	static void execute(uint8_t data) {

		/*
		 * Common deterministic approximation for XAA/ANE.
		 *
		 * Real hardware is unstable, so this is an emulation policy
		 * rather than a universally exact hardware constant.
		 */
		static const uint8_t magic = 0xee;

		A = static_cast<uint8_t>((A | magic) & X & data);
		update_nz_flags(A);
	}
};

#endif
