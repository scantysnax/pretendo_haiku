// Nes.cpp
#include "Nes.h"
#include "Ppu.h"
#include "Apu.h"


namespace nes {

// Define this exactly once in the program
Cart cart;

alignas(512) static uint32_t sFrameScanlineBuffer[256] = {};

// -----------------------------------------------------------------------------
// reset
//
// Resets the major NES subsystems using the requested reset type.
//
// The CPU, APU, and PPU are reset in sequence so each subsystem returns to its
// appropriate startup state.
//
// Parameters:
//   reset_type - Type of reset to perform.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
reset(reset_type type)
{
	cpu::reset(type);
    apu::reset(type);

    if (cart.mapper()) {
        cart.mapper()->reset();
    }

    ppu::reset(type);
}


// -----------------------------------------------------------------------------
// frame_scanline_buffer
//
// Returns the persistent scanline buffer used while rendering a frame.
//
// Parameters:
//   None.
//
// Returns:
//   Pointer to the shared 32-bit scanline pixel buffer.
// -----------------------------------------------------------------------------
uint32_t*
frame_scanline_buffer()
{
	return sFrameScanlineBuffer;
}


// -----------------------------------------------------------------------------
// nes::run_frame
//
// Executes or resumes one NES frame.
//
// Frame execution is derived directly from the PPU's persistent vpos/hpos state.
// If a debugger BreakPoint interrupts a scanline, the function returns without
// losing the current PPU position. The next call resumes that same scanline.
//
// Visible-scanline pixel storage is persistent so an interrupted visible line can
// continue rendering into the same 256-pixel buffer after debugger resume.
//
// Parameters:
//   output - Host video-output interface receiving completed scanlines and the
//            frame-latched scroll position.
//
// Returns:
//   true  - A complete NES frame finished.
//   false - Execution stopped before the frame finished.
// -----------------------------------------------------------------------------
bool
run_frame(FrameOutput *output)
{
	if (!output) {
		return false;
	}

	/*
	 * Scanline 0 is the pre-render scanline in Pretendo's
	 * current PPU numbering.
	 */
	if (!nes::ppu::execute_scanline(nes::ppu::scanline_prerender{})) {
		return false;
	}

	/*
	 * Visible scanlines 1..240 correspond to output rows 0..239.
	 *
	 * Use the persistent scanline buffer so normal execution and
	 * debugger stepping operate on the same rendering storage.
	 */
	for (int y = 0; y < 240; ++y) {
		uint32_t *buffer = frame_scanline_buffer();

		if (!nes::ppu::execute_scanline(nes::ppu::scanline_render(buffer))) {
			return false;
		}

		output->SubmitScanline(y, buffer);
	}

	/*
	 * Scanline 241 is the post-render scanline.
	 */
	if (!nes::ppu::execute_scanline(nes::ppu::scanline_postrender{})) {
		return false;
	}

	/*
	 * Scanlines 242..261 are vblank.
	 */
	for (int scanline = 242; scanline <= 261; ++scanline) {
		if (!nes::ppu::execute_scanline(nes::ppu::scanline_vblank{})) {
			return false;
		}
	}

	/*
	 * Scanline 262 is handled by the PPU's frame-transition
	 * logic when execution reaches the next frame.
	 */
	return true;
}


// -----------------------------------------------------------------------------
// nes::debug_step_instruction
//
// Advances the emulator until one CPU instruction has completed.  The stepping
// path runs through the PPU dot scheduler so CPU, PPU, APU, DMA, mapper sync,
// NMI timing, and PPU write logging stay synchronized.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_step_instruction()
{
	if (!nes::cart.mapper()) {
		return;
	}

	bool leftInstructionBoundary = false;

	for (int32_t safety = 0; safety < 200000; safety++) {
		nes::ppu::debug_step_dot();

		if (!nes::cpu::debug_instruction_boundary()) {
			leftInstructionBoundary = true;
		} else if (leftInstructionBoundary) {
			return;
		}
	}
}


// -----------------------------------------------------------------------------
// nes::debug_step_frame
//
// Advances the emulator by one PPU frame while the debugger is paused.  The
// stepping path runs through the PPU dot scheduler so CPU, PPU, APU, DMA,
// mapper sync, NMI timing, and PPU write logging stay synchronized.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
void
debug_step_frame()
{
	if (!nes::cart.mapper()) {
		return;
	}

	const uint64_t startFrame = nes::ppu::frame_counter();

	for (int32_t safety = 0; safety < 2000000; safety++) {
		nes::ppu::debug_step_dot();

		if (nes::ppu::frame_counter() != startFrame) {
			return;
		}
	}
}


} // namespace nes
