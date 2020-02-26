// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ROM_SIZE (32*1024)
#define RAM_SIZE (64*1024)

uint8_t read6502(uint16_t address);
uint8_t real_read6502(uint16_t address, bool debugOn, uint8_t bank);

void memory_init();

void memory_save(FILE *f, bool dump_ram, bool dump_bank);

uint8_t emu_read(uint8_t reg);
void emu_write(uint8_t reg, uint8_t value);

void memory_destroy();

#endif
