// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _OPL2_H_
#define _OPL2_H_

#include <stdint.h>

void opl2_init();
uint8_t opl2_read(uint8_t reg);
void opl2_write(uint8_t reg, uint8_t value);

#endif
