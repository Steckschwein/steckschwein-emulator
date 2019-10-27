// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"
//XXX
#include "glue.h"

static uint8_t uartregisters[16];

void
uart_init()
{
	srand(time(NULL));
}

uint8_t
uart_read(uint8_t reg)
{

	return uartregisters[reg];
}

void
uart_write(uint8_t reg, uint8_t value)
{
	uartregisters[reg] = value;
	// TODO
}
