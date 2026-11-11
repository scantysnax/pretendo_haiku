
#include "Input.h"
#include "Controller.h"
#include "Nes.h"
#include <iostream>

namespace nes::input {
namespace {

uint8_t strobe_ = 0;

bool dma_controller_read_repeat_ = false;

uint8_t last_controller1_read_ = 0x40;
uint8_t last_controller2_read_ = 0x40;

}

Controller controller1;
Controller controller2;
Controller controller3;
Controller controller4;

/*
 * Notes about the Four Score:
 * The read procedure looks like this:
 *
 * Read $4016 8 times to get the 8 buttons for controller #1.
 * Read $4016 8 more times to get the buttons for controller #3.
 * Read $4016 8 more times to get the signature, $10.
 * Read $4017 8 times to get the 8 buttons for controller #2.
 * Read $4017 8 more times to get the buttons for controller #4.
 * Read $4017 8 more times to get the signature, $20.
 *
 * The $10 and $20 signatures are sent most significant bit first.
 * They can be used to detect 1. whether the Four Score switch is in the
 * 4 player position and 2. whether a DMC sample fetch caused a bit deletion.
 */


// -----------------------------------------------------------------------------
// set_dma_controller_read_repeat
//
// Indicates whether the current CPU bus read repeats a controller read
// already performed during the same DMC DMA transaction.
//
// Repeated reads still reach the input subsystem, but do not clock the
// controller shift register again.
//
// Parameters:
//   repeat - true to preserve the previous controller sample.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
set_dma_controller_read_repeat (bool repeat)
{
	dma_controller_read_repeat_ = repeat;
}



// -----------------------------------------------------------------------------
// reset
//
// Resets the controller input strobe state.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void reset()
{
	strobe_ = 0;

	dma_controller_read_repeat_ = false;
	last_controller1_read_ = 0x40;
	last_controller2_read_ = 0x40;
}


// -----------------------------------------------------------------------------
// write4016
//
// Writes the controller strobe register.
//
// The low bit controls the controller latch signal.  On a high-to-low
// transition, the current state of all connected controllers is latched for
// subsequent serial reads.
//
// Parameters:
//   value - Value written to controller register $4016.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void write4016(uint8_t value) {

	/*
	The 4021 has inverting inputs.
	Every time the Strobe (STRB) connection gets a HIGH then LOW signal,
	the data on the 8 parrallel inputs gets latched in the register.
	Since the inputs of the 4021 are inverting,
	the 8 resistors act as pull-up resistors to the +5V making the input HIGH by default.
	When a button is pressed, 0V is allowed to the input, thus enabling the state in the register.
	The serial data input is tied to 0V so as not to act upon the internal flip-flops of the 4021.
	Then, everytime the Clock (CLK) connection gets a HIGH signal,
	rightmost bit (P8) is sent to D0 through Q8. After 8 shift,
	the entire register is zero and thus all other read return a 0 state.
	*/

	value &= 0x1;

	if (!value && strobe_) {
		controller1.poll();
		controller2.poll();
		controller3.poll();
		controller4.poll();
	}

	strobe_ = value;
}



 // -----------------------------------------------------------------------------
 // read4016
 //
 // Reads serial controller data from the first controller port.
 //
 // Consecutive DMC DMA reads belonging to one uninterrupted controller
 // read-clock assertion return the previously sampled value without
 // advancing the controller shift register.
 //
 // Parameters:
 //   None.
 //
 // Returns:
 //   Current controller-port value.
 // -----------------------------------------------------------------------------
uint8_t
read4016()
{
	if (dma_controller_read_repeat_) {
		return last_controller1_read_;
	}

	last_controller1_read_ = (controller1.read() & 0x1f) | 0x40;

	return last_controller1_read_;
}


// -----------------------------------------------------------------------------
// read4017
//
// Reads serial controller data from the second controller port.
//
// Repeated DMA reads preserve the previously sampled controller value
// without advancing its serial shift register.
//
// Parameters:
//   None.
//
// Returns:
//   Current controller-port value.
// -----------------------------------------------------------------------------
uint8_t
read4017()
{
	if (dma_controller_read_repeat_) {
		return last_controller2_read_;
	}

	last_controller2_read_ = (controller2.read() & 0x1f) | 0x40;

	return last_controller2_read_;
}


}
