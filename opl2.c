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
//#include "glue.h"
#include "fmopl.h"

static FM_OPL* p_fm_opl;
static int sample_rate = 16000;

void
opl2_init()
{

	p_fm_opl = OPLCreate(OPL_TYPE_YM3812, 3579545, sample_rate);

}

uint8_t
opl2_read(uint8_t reg)
{

	return 0;
}

void
opl2_write(uint8_t reg, uint8_t value)
{

}
