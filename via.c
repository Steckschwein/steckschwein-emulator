// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "via.h"
#include "memory.h"
#include "glue.h"
#include "joystick.h"

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

static uint8_t via1registers[16];
static uint8_t via1pb_in;

#define MAX_JOYS 2

void via1_init() {
}

uint8_t via1_pb_get_reg(uint8_t reg) {
	return via1registers[reg];
}

uint8_t via1_read(uint8_t reg) {
	if (reg == 0) {	// PB
		// 0 input  -> take input bit
		// 1 output -> take output bit
		uint8_t v = (via1pb_in & (via1registers[2] ^ 0xff)) | (via1registers[0] & via1registers[2]);

		if ((via1registers[2] & SD_DETECT) == 0) {		//SD_DETECT pin set as input?
			if (sdcard_file) {
				v &= ~(SD_DETECT);
			} else {
				v |= SD_DETECT;
			}
		}
		return v;
	} else if (reg == 1) { // PA
		uint8_t value = (joystick1_data ? JOY_DATA1_MASK : 0) |
							(joystick2_data ? JOY_DATA2_MASK : 0);
		return value;
	} else {
		return via1registers[reg];
	}
}

void via1_write(uint8_t reg, uint8_t value) {
	via1registers[reg] = value;

//	printf("via write() %x %x\n", reg, value);

	if (reg == 0) {
		// PB
	} else if (reg == 2) {
		// PB DDRB
	} else if (reg == 1 || reg == 3) {
		// PA
		joystick_latch = via1registers[1] & JOY_LATCH_MASK;
		joystick_clock = via1registers[1] & JOY_CLK_MASK;
	}

}

uint8_t via1_pb_get_out() {
	return via1registers[2] /* DDR  */& via1registers[0]; /* PB */
}

void via1_pa_set_in(uint8_t value) {
	via1registers[1] = value;
}

void via1_pb_set_in(uint8_t value) {
	via1pb_in = value;
}

void via1_sr_set(uint8_t value) {
	via1registers[10] = value;
}
