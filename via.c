// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "via.h"
#include "memory.h"
//XXX
#include "glue.h"

static uint8_t via1registers[16];
static uint8_t via1pb_in;

void
via1_init()
{
	srand(time(NULL));
}

uint8_t
via1_read(uint8_t reg)
{
	if (reg == 0) {
		// PB
		// 0 input  -> take input bit
		// 1 output -> take output bit
		return (via1pb_in & (via1registers[2] ^ 0xff)) |
		  (via1registers[0] & via1registers[2]);
		} else {
		  switch (reg) {
			case 4:
			case 5:
			case 8:
			case 9:
				// timer A and B: return random numbers for RND(0)
				// XXX TODO: these should be real timers :)
				return rand() & 0xff;
			default:
				return via1registers[reg];
		  }
}
}



void
via1_write(uint8_t reg, uint8_t value)
{
	via1registers[reg] = value;
	if (reg == 0) {
		// PB
	} else if (reg == 2) {
		// PB DDRB
	}
}



uint8_t
via1_pb_get_out()
{
	return via1registers[2] /* DDR  */ & via1registers[0]; /* PB */
}

void
via1_pb_set_in(uint8_t value)
{
	via1pb_in = value;
}

void
via1_sr_set(uint8_t value)
{
	via1registers[10] = value;
}
