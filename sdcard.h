// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD
#ifndef _SD_CARD_H_
#define _SD_CARD_H_
#include <inttypes.h>

extern FILE *sdcard_file;

void spi_sdcard_select();
void spi_sdcard_deselect();
uint8_t spi_sdcard_handle(uint8_t inbyte);

#endif
