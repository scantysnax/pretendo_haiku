
#ifndef _INPUT_H_
#define _INPUT_H_

#include "Nes.h"

#include <cstdint>


class Controller;


namespace nes::input {

void reset();
void set_dma_controller_read_repeat(bool repeat);

void write4016(uint8_t value);
uint8_t read4016();
uint8_t read4017();

extern Controller controller1;
extern Controller controller2;
extern Controller controller3;
extern Controller controller4;

}

#endif	// _INPUT_H_
