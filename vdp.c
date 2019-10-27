// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "vdp.h"
#include "memory.h"
//XXX
#include "glue.h"

static uint8_t vdpregisters[16];

void
vdp_init()
{
	srand(time(NULL));
}

uint8_t
vdp_read(uint8_t reg)
{

	return vdpregisters[reg];
}

void
vdp_write(uint8_t reg, uint8_t value)
{
	vdpregisters[reg] = value;
	// TODO
}
