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
        nes::ppu::scroll_state_t const  s = nes::ppu::scroll_state();
        uint32_t const v = s.v;

        int32 const coarseX = v & 0x1f;
        int32 const coarseY = (v >> 5) & 0x1f;
        int32 const fineY = (v >> 12) & 0x7;
        int32 const fineX = s.x & 0x7;

        int32 const ntX = (v & 0x000) ? 256 : 0;
        int32 const ntY = (v & 0x800) ? 240 : 0;

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

} // namespace nes
