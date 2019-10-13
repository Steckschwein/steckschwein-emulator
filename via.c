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

void
via1_init()
{
	srand(time(NULL));
}

uint8_t
via1_read(uint8_t reg)
{
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

void
via1_write(uint8_t reg, uint8_t value)
{
	via1registers[reg] = value;
	// TODO
}

//
// VIA#2
//
// PA0 PS/2 DAT
// PA1 PS/2 CLK
// PA2 LCD backlight
// PA3 NESJOY latch (for both joysticks)
// PA4 NESJOY joy1 data
// PA5 NESJOY joy1 CLK
// PA6 NESJOY joy2 data
// PA7 NESJOY joy2 CLK

static uint8_t via2registers[16];
static uint8_t via2pb_in;

void
via2_init()
{
}

uint8_t
via2_read(uint8_t reg)
{
	if (reg == 0) {
		// PB
		// 0 input  -> take input bit
		// 1 output -> take output bit
		return (via2pb_in & (via2registers[2] ^ 0xff)) |
			(via2registers[0] & via2registers[2]);
	} else {
		return via2registers[reg];
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via2registers[reg] = value;

	if (reg == 0) {
		// PB
	} else if (reg == 2) {
		// PB DDRB
	}
}

uint8_t
via2_pb_get_out()
{
	return via2registers[2] /* DDR  */ & via2registers[0]; /* PB */
}

void
via2_pb_set_in(uint8_t value)
{
	via2pb_in = value;
}

void
via2_sr_set(uint8_t value)
{
	via2registers[10] = value;
}
