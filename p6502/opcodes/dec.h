#ifndef DEC_20121206_H_
#define DEC_20121206_H_

//------------------------------------------------------------------------------
// Name: opcode_dec
// Desc: Decrement Memory
//------------------------------------------------------------------------------
struct opcode_dec {

	typedef operation_modify memory_access;
	
	static void execute(uint8_t &data) {
		--data;
		update_nz_flags(data);
	}
};

#endif

