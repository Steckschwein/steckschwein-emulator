// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "opl2.h"
#include "extern/src/fmopl.h"
#include "memory.h"
//XXX
//#include "glue.h"
#include "fmopl.h"

static FM_OPL* p_fm_opl;
static int sample_rate = 16000;

void opl2_irq(int status){
	printf("opl irq %x\n", status);
}

void opl2_timer(int channel){
	printf("opl timer %x\n", channel);
}

void
opl2_init()
{
	p_fm_opl = OPLCreate(OPL_TYPE_YM3812, 3579545, sample_rate);

	OPLSetIRQHandler(p_fm_opl, opl2_irq, 0);
	OPLSetTimerHandler(p_fm_opl, opl2_timer, 0);
}

uint8_t
opl2_read(uint8_t reg)
{
//	printf("opl2_read %x\n", reg);
	return OPLRead(p_fm_opl, reg);
}

void
opl2_write(uint8_t reg, uint8_t value)
{
//	printf("opl2_write %x %x\n", reg, value);
	OPLWrite(p_fm_opl, reg, value);
}

void opl2_destroy(){
	OPLDestroy(p_fm_opl);
}
