// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>

void uart_init();
uint8_t uart_read(uint8_t reg);
void uart_write(uint8_t reg, uint8_t value);

#endif
