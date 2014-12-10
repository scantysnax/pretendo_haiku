#ifndef DCP_20121206_H_
#define DCP_20121206_H_

//------------------------------------------------------------------------------
// Name: opcode_dcp
// Desc: Decrement Memory (no borrow)
//------------------------------------------------------------------------------
struct opcode_dcp  : compare {

	typedef operation_modify memory_access;
	
	static void execute(uint8_t &data) {
		--data;
		update_nz_flags(data);
		do_compare(A, data);
	}
};

#endif

