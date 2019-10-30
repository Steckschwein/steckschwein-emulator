// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VDP_H_
#define _VDP_H_

#include <stdint.h>
#include <SDL.h>

// visible area we're darwing
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 313

void vdp_reset(void);
bool vdp_init(int window_scale, char *quality);
uint8_t vdp_read(uint8_t reg);
void vdp_write(uint8_t reg, uint8_t value);

bool vdp_step(float mhz); 

bool vdp_update();

#endif
