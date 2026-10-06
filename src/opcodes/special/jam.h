#ifndef JAM_20140417_H_
#define JAM_20140417_H_

//------------------------------------------------------------------------------
// Name: opcode_jam
// Desc: stall the CPU
//------------------------------------------------------------------------------
class opcode_jam {
public:

	static bus_cycle_type bus_cycle() {
		return bus_cycle_type::read;
	}


	static uint_least16_t bus_address() {
		return PC.raw;
	}


	static void execute() {
		switch (cycle_) {
		case 1:
			// make sure we spin forever
			--PC.raw;
			jam_handler();

			OPCODE_COMPLETE;
			break;

		default:
			abort();
		}
	}
};

#endif
