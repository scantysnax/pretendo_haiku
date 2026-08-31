// Nes.cpp
#include "Nes.h"

#include "Cart.h"
#include "Cpu.h"
#include "Ppu.h"
#include "Apu.h"
#include "Bus.h"

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
void reset(Reset reset_type) {
    cpu::reset(reset_type);
    apu::reset(reset_type);
    ppu::reset(reset_type);
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

	while (true) {
		const uint16_t scanline = static_cast<uint16_t>(nes::ppu::vpos());

		/*
		 * PPU scanline 0 is prerender after reset.
		 *
		 * At the end of a completed frame vpos_ is 262. Entering the
		 * prerender scanline at 262 causes the PPU's start_frame() to
		 * wrap vpos_ back to zero before executing the line.
		 */
		if (scanline == 0 || scanline == 262) {
			if (!nes::ppu::execute_scanline(
					nes::ppu::scanline_prerender{})) {

				return false;
			}

			/*
			 * Prerender completed. Latch the scroll state exactly once
			 * before visible rendering begins.
			 */
			{
				const nes::ppu::scroll_state_t s = nes::ppu::scroll_state();
				const uint32_t v = s.v;
				const int32_t coarseX = v & 0x1f;
				const int32_t coarseY = (v >> 5) & 0x1f;
				const int32_t fineY = (v >> 12) & 0x7;
				const int32_t fineX = s.x & 0x7;

				const int32_t ntX = (v & 0x400) ? 256 : 0;
				const int32_t ntY = (v & 0x800) ? 240 : 0;

				const uint32_t scrollX = static_cast<uint32_t>(
					(ntX + coarseX * 8 + fineX) & 0x1ff);

				const uint32_t scrollY = static_cast<uint32_t>(
					(ntY + coarseY * 8 + fineY) % 480);

				output->SetLatchedScroll(scrollX, scrollY);
			}

			continue;
		}

		/*
		 * Visible scanlines.
		 *
		 * PPU vpos 1..240 maps to output rows 0..239.
		 */
		if (scanline >= 1 && scanline <= 240) {
			const int32_t y = static_cast<int32_t>(scanline - 1);

			if (!nes::ppu::execute_scanline(
					nes::ppu::scanline_render(sFrameScanlineBuffer))) {

				return false;
			}

			/*
			 * Only submit a completely rendered scanline.
			 */
			output->SubmitScanline(y, sFrameScanlineBuffer);
			continue;
		}

		/*
		 * Postrender scanline.
		 */
		if (scanline == 241) {
			if (!nes::ppu::execute_scanline(
					nes::ppu::scanline_postrender{})) {

				return false;
			}

			continue;
		}

		/*
		 * VBlank scanlines.
		 */
		if (scanline >= 242 && scanline <= 261) {
			if (!nes::ppu::execute_scanline(
					nes::ppu::scanline_vblank{})) {

				return false;
			}

			/*
			 * Reaching vpos_ 262 means the complete frame has
			 * finished. The next invocation begins the next
			 * prerender scanline.
			 */
			if (nes::ppu::vpos() == 262) {
				return true;
			}

			continue;
		}

		/*
		 * This should never happen during correctly synchronized
		 * PPU execution.
		 */
		return false;
	}

	/*
	 * The loop above always exits via a return, but keep an explicit
	 * fallback so the compiler sees a value on every control path.
	 */
	return false;
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

	const uint64_t startFrame = nes::ppu::ppu_frame_counter();

	for (int32_t safety = 0; safety < 2000000; safety++) {
		nes::ppu::debug_step_dot();

		if (nes::ppu::ppu_frame_counter() != startFrame) {
			return;
		}
	}
}


} // namespace nes
