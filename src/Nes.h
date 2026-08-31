#ifndef NES_H_
#define NES_H_

#include <cstdint>

#include "Cart.h"
#include "Reset.h"

namespace nes {

// -----------------------------------------------------------------------------
// FrameOutput
//
// OS-neutral interface used by the NES core to deliver completed video
// scanlines and frame-latched scroll information to the host application.
//
// Platform-specific front ends implement this interface without introducing UI
// or operating-system dependencies into the emulator core.
// -----------------------------------------------------------------------------
class FrameOutput {
public:
	virtual ~FrameOutput() = default;

	// -------------------------------------------------------------------------
	// FrameOutput::SubmitScanline
	//
	// Delivers one completely rendered NES scanline to the host application.
	//
	// Parameters:
	//   y      - Output scanline number in the range 0..239.
	//   pixels - Pointer to the 256 rendered 32-bit pixels.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	virtual void SubmitScanline(int32_t y, const uint32_t *pixels) = 0;

	// -------------------------------------------------------------------------
	// FrameOutput::SetLatchedScroll
	//
	// Delivers the scroll position latched immediately before visible frame
	// rendering begins.
	//
	// Parameters:
	//   x - Horizontal scroll position in the 512-pixel name-table space.
	//   y - Vertical scroll position in the 480-pixel name-table space.
	//
	// Returns:
	//   Nothing.
	// -------------------------------------------------------------------------
	virtual void SetLatchedScroll(uint32_t x, uint32_t y) = 0;
};


// Global cartridge instance owned by the NES core.
extern Cart cart;


void reset(Reset reset_type);

uint32_t *frame_scanline_buffer();

bool run_frame(FrameOutput *output);

void debug_step_instruction();

void debug_step_frame();

} // namespace nes

#endif
