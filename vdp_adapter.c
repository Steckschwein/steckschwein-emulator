#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "vdp_adapter.h"
#include "memory.h"
#include "glue.h"

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static uint8_t vdp_ctrl_reg[47];
static uint8_t vdp_stat_reg[10];


bool
vdp_step(float mhz)
{
	return false; // true - new_frame
}

bool 
vdp_update(){
	//printf("vdp_update()\n");			
	
	return true;
}

void
vdp_reset()
{
	
}

bool
vdp_init(int window_scale, char *quality)
{
	srand(time(NULL));
	vdp_reset();

#ifndef __MORPHOS__

#endif
	
	if (debugger_enabled) {
	//	DEBUGInitUI(renderer);
	}

	return true;
}

uint8_t
vdp_read(uint8_t reg)
{
	if (reg > 9)
	{
		fprintf(stderr, "attempted to read non-existent status register R#%d\n", reg);
		return -1;
	}
	return vdp_stat_reg[reg];
}

void
vdp_write(uint8_t reg, uint8_t value)
{
	if (reg > 46)
	{
		fprintf(stderr, "attempted to write to non-existent R#%d\n", reg);
		return;
	}
	framebuffer[0] = 0;
	vdp_ctrl_reg[reg] = value;
	// TODO
}

void vdp_end(){
}
