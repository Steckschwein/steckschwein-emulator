// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VDP_H_
#define _VDP_H_

#include <stdint.h>

void vdp_init();
uint8_t vdp_read(uint8_t reg);
void vdp_write(uint8_t reg, uint8_t value);

#endif
