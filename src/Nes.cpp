// Nes.cpp
#include "Nes.h"
#include "Ppu.h"
#include "Apu.h"


namespace nes {

// Define this exactly once in the program
Cart cart;

alignas(512) static uint32_t sFrameScanlineBuffer[256] = {};



// -------------------------------------------------------------------------
// nes::reset
//
// Resets the major NES subsystems using the requested reset type.
//
// The CPU and APU are reset first, followed by the cartridge mapper.
// Once all audio-producing subsystems have been initialized, the APU
// resampler and DC-blocking filter are initialized from the final
// post-reset audio output.
//
// The PPU is reset last, preserving the existing subsystem reset order.
//
// Parameters:
//   type - Type of reset to perform.
//
// Returns:
//   Nothing.
// -------------------------------------------------------------------------
void
reset(reset_type type)
{
	cpu::reset(type);
	apu::reset(type);
		
	if (cart.mapper()) {
		cart.mapper()->reset();
	}
	
	// Initialize audio filters after both the native APU and
	// cartridge expansion-audio hardware have been reset.
	apu::finalize_audio_reset();

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
// Executes or resumes one NES frame from the PPU's current scanline.
//
// The PPU maintains its own scanline (vpos_) and dot (hpos_) positions.
// Therefore, the frame dispatcher must not assume that every invocation begins
// at the prerender scanline.
//
// A debugger breakpoint may interrupt execution anywhere in a frame. On resume,
// this function selects the correct scanline handler for the saved PPU position.
//
// The PPU's execute_scanline() implementation is responsible for preserving
// the dot position and any pending CPU/APU timing slot.
//
// The persistent visible-scanline buffer allows an interrupted rendering line
// to continue writing into the same pixel storage.
//
// Parameters:
//   output - Host video-output interface.
//
// Returns:
//   true  - The remainder of the current NES frame completed.
//   false - Execution was interrupted before the frame completed.
// -----------------------------------------------------------------------------
bool
run_frame(FrameOutput *output)
{
    if (!output) {
        return false;
    }

    /*
     * Continue from the PPU's current scanline instead of always
     * restarting at the prerender scanline.
     */
    while (true) {
        const uint16_t scanline = nes::ppu::scanline();

        /*
         * Scanline 262 is the frame-transition position.
         *
         * The PPU's prerender handler calls start_frame() at this
         * boundary, resetting vpos_ to zero. Scanline zero is also
         * used for prerender immediately after a PPU reset.
         */
        if (scanline == 0 || scanline == 262) {
            if (!nes::ppu::execute_scanline(
                    nes::ppu::scanline_prerender{})) {
                return false;
            }

            continue;
        }

        /*
         * Visible scanlines 1..240 correspond to output rows 0..239.
         *
         * Keep using the persistent buffer so partial scanlines
         * survive debugger interruptions.
         */
        if (scanline >= 1 && scanline <= 240) {
            uint32_t *buffer = frame_scanline_buffer();

            if (!nes::ppu::execute_scanline(
                    nes::ppu::scanline_render(buffer))) {
                return false;
            }

            output->SubmitScanline(scanline - 1, buffer);
            continue;
        }

        /*
         * Scanline 241 is post-render.
         */
        if (scanline == 241) {
            if (!nes::ppu::execute_scanline(
                    nes::ppu::scanline_postrender{})) {
                return false;
            }

            continue;
        }

        /*
         * Scanlines 242..261 are the vblank period.
         */
        if (scanline >= 242 && scanline <= 261) {
            if (!nes::ppu::execute_scanline(
                    nes::ppu::scanline_vblank{})) {
                return false;
            }

            /*
             * Completing scanline 261 advances the PPU to 262,
             * marking the end of this frame.
             */
            if (scanline == 261) {
                return true;
            }

            continue;
        }

        /*
         * An unexpected scanline value indicates inconsistent
         * PPU timing state. Do not spin indefinitely.
         */
        return false;
    }
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
