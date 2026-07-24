// Nes.cpp
#include "Nes.h"

#include "Cart.h"
#include "Cpu.h"
#include "Ppu.h"
#include "Apu.h"
#include "Bus.h"

// Needed because we call window->submit_scanline(...)
#include "PretendoWindow.h"

namespace nes {

// Define this exactly once in the program
Cart cart;

void reset(Reset reset_type) {
    cpu::reset(reset_type);
    apu::reset(reset_type);
    ppu::reset(reset_type);
}


void 
run_frame(PretendoWindow *window)
{
    if (!window)
        return;

    // Scanline 20: prerender
    ppu::execute_scanline(ppu::scanline_prerender{});

    // Latch scroll for the frame BEFORE visible rendering starts
    {
        nes::ppu::scroll_state_t const s = nes::ppu::scroll_state();
        uint32_t const v = s.v;

        int32_t const coarseX = v & 0x1f;
        int32_t const coarseY = (v >> 5) & 0x1f;
        int32_t const fineY = (v >> 12) & 0x7;
        int32_t const fineX = s.x & 0x7;

        int32_t const ntX = (v & 0x400) ? 256 : 0;
        int32_t const ntY = (v & 0x800) ? 240 : 0;

        uint32_t const scrollX = static_cast<uint32_t>((ntX + coarseX * 8 + fineX) & 0x1ff);
        uint32_t const scrollY = static_cast<uint32_t>((ntY + coarseY * 8 + fineY) % 480);

        window->SetLatchedScroll(scrollX, scrollY);
    }

    // Visible scanlines 21-260
    alignas(512) uint32_t buffer[256] = {};
    for (int y = 0; y < 240; ++y) {
        ppu::execute_scanline(ppu::scanline_render(buffer));
        window->submit_scanline(y, buffer);
    }

    // Scanline 261: postrender
    ppu::execute_scanline(ppu::scanline_postrender{});

    // Scanlines 0-19: vblank
    for (int i = 0; i < 20; ++i) {
        ppu::execute_scanline(ppu::scanline_vblank{});
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

	const uint64 startFrame = nes::ppu::ppu_frame_counter();

	for (int32_t safety = 0; safety < 2000000; safety++) {
		nes::ppu::debug_step_dot();

		if (nes::ppu::ppu_frame_counter() != startFrame) {
			return;
		}
	}
}


} // namespace nes
