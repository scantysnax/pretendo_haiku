#ifndef NES_H_
#define NES_H_

#include <cstdint>
#include "Ppu.h"
#include "Reset.h"

class Cart;
class PretendoWindow;

namespace nes {

extern Cart cart;

// Public API
void reset(Reset reset_type);

// This is what PretendoWindow.cpp is calling
void run_frame(PretendoWindow *window);
void debug_step_instruction();
void debug_step_frame();

} // namespace nes

#endif // NES_H_
