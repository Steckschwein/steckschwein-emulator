#ifndef _JC_MEMORY_H_
#define _JC_MEMORY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <Machine.h>
#include <MOS6502.h>

void memoryJuniorComputerCreate(void* cpu, RomImage* romImage);
void memoryJuniorComputerDestroy();

UInt8 memoryJuniorComputerReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn);
void memoryJuniorComputerWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value);

#endif
