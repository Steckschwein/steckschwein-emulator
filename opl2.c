// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#include <SDL_mixer.h>
#include "opl2.h"
#include "extern/src/fmopl.h"
#include "memory.h"
//XXX
//#include "glue.h"
#include "fmopl.h"

static FM_OPL *p_fm_opl;

int opl2_samplerate = 11025;     // higher samplerates result in "out of memory"

void opl2_irq(int status) {
	printf("opl irq %x\n", status);
}

void opl2_timer(int channel) {
	printf("opl timer %x\n", channel);
}

void opl2_init() {
	int opl2_audiobuffer = (4096 / (44100 / opl2_samplerate));

	if(Mix_OpenAudio(opl2_samplerate, AUDIO_S16, 2, opl2_audiobuffer))
	{
		printf("Unable to open audio: %s\n", Mix_GetError());
		return;
	}

	p_fm_opl = OPLCreate(OPL_TYPE_YM3812, 3579545, opl2_samplerate);

	OPLSetIRQHandler(p_fm_opl, opl2_irq, 0);
	OPLSetTimerHandler(p_fm_opl, opl2_timer, 0);
}

void opl2_step() {

}

uint8_t opl2_read(uint8_t reg) {
//	printf("opl2_read %x\n", reg);
	return OPLRead(p_fm_opl, reg);
}

void opl2_write(uint8_t reg, uint8_t value) {
//	printf("opl2_write %x %x\n", reg, value);
	OPLWrite(p_fm_opl, reg, value);
}

void opl2_destroy() {
	OPLDestroy(p_fm_opl);
//	Mix_ChannelFinished(
}
