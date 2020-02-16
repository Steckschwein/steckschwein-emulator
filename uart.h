// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include <stdio.h>

#define UART_REG_IER 0
#define UART_REG_LSR 5

#define lsr_DR 		1<<0 // data ready
#define lsr_THRE 	1<<5 // transmitter holding register

void
uart_init(unsigned char *prg_path, int prg_override_start);

uint8_t uart_read(uint8_t reg);
void uart_write(uint8_t reg, uint8_t value);

#endif
