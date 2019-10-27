// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "opl2.h"
#include "memory.h"
//XXX
#include "glue.h"

static uint8_t opl2registers[16];

void
opl2_init()
{
	srand(time(NULL));
}

uint8_t
opl2_read(uint8_t reg)
{

	return opl2registers[reg];
}

void
opl2_write(uint8_t reg, uint8_t value)
{
	opl2registers[reg] = value;
	// TODO
}
