// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VIA_H_
#define _VIA_H_

#include <stdint.h>
#include "sdcard.h"
#include <SDL_joystick.h>
#include <SDL_events.h>

#define SD_DETECT 1<<6

//void via1_joystick(SDL_Event *event);

uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);
void via1_init();
uint8_t via1_pb_get_out();
//void via1_pb_set_in(uint8_t value);
void via1_sr_set(uint8_t value);

#endif
