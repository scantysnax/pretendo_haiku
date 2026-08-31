#include "Controller.h"

// -----------------------------------------------------------------------------
// Controller::read
//
// Returns the next serial controller bit.
//
// The first eight reads shift out the latched controller state.  Subsequent
// reads return 1, matching the NES controller's post-read behavior.
//
// Parameters:
//   None.
//
// Returns:
//   Next controller data bit.
// -----------------------------------------------------------------------------
uint8_t Controller::read() {
	if (read_index_++ < 8) {
		return data_.read();
	} else {
		return 0x1;
	}
}


// -----------------------------------------------------------------------------
// Controller::poll
//
// Latches the current controller key state for subsequent serial reads.
//
// When the controller is connected, the current key-state bits are loaded into
// the controller data register and the serial read index is reset.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void Controller::poll() {

	if (connected_) {
		data_.load(static_cast<uint8_t>(keystate_.to_ulong()));
		read_index_ = 0;
	}
}

