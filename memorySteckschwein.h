// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _STS_MEMORY_H_
#define _STS_MEMORY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <Machine.h>
#include <MOS6502.h>

void memorySteckschweinCreate(void* cpu, RomImage* romImage);

UInt8 memorySteckschweinReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn);
void memorySteckschweinWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value);

uint8_t memory_get_ctrlport(uint16_t address);

void memory_save(FILE *f, bool dump_ram, bool dump_bank);

uint8_t emu_read(uint8_t reg);
void emu_write(uint8_t reg, uint8_t value);

void memory_destroy();

#endif
