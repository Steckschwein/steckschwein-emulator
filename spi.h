// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>
#include <SDL.h>

void spi_step();

// TODO FIXME design
void spi_handle_keyevent(SDL_KeyboardEvent *keyBrdEvent);

#endif
