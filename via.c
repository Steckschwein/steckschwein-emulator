// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "via.h"
#include "glue.h"
#include "joystick.h"

#define VIA1_PORT_B 0
#define VIA1_PORT_A 1
#define VIA1_DDR_B 2
#define VIA1_DDR_A 3
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

#define MAX_JOYS 2

void via1_reset() {
	via1registers[VIA1_PORT_A] = 0xff; // set initial to high if used as input
	via1registers[VIA1_PORT_B] = 0xff; // set initial to high if used as input
}


uint8_t via1_read(uint8_t reg) {
	if (reg == VIA1_PORT_B) {	// PB
		// 0 input  -> take input bit
		// 1 output -> take output bit
//		uint8_t v = (via1pb_in & (via1registers[2] ^ 0xff)) | (via1registers[0] & via1registers[2]);
		uint8_t v = (via1registers[reg] & via1registers[VIA1_DDR_B]);

		if ((via1registers[2] & SD_DETECT) == 0) {		//SD_DETECT pin set as input?
			if (sdcard_file) {
				v &= ~(SD_DETECT);
			} else {
				v |= SD_DETECT;
			}
		}
		return v;
	} else if (reg == VIA1_PORT_A) {
		// 0 input  -> take input bit
		// 1 output -> take output bit
		uint8_t v = (via1registers[VIA1_DDR_A] ^ 0xff);
		if(joy1_mode != NONE){
			v = (joystick1_data ? (v | JOY_DATA1_MASK) : (v & ~(JOY_DATA1_MASK)));
		}
		if(joy2_mode != NONE){
			v = (joystick2_data ? (v | JOY_DATA2_MASK) : (v & ~(JOY_DATA2_MASK)));
		}
		return v;
	} else {
		return via1registers[reg];
	}
}

void via1_write(uint8_t reg, uint8_t value) {
//	printf("via write() %x %x\n", reg, value);
	via1registers[reg] = value;

	if (reg == VIA1_PORT_A) {
		joystick_latch = via1registers[reg] & JOY_LATCH_MASK;
		joystick_clock = via1registers[reg] & JOY_CLOCK_MASK;
	}
}

uint8_t via1_pb_get_out() {
	return via1registers[2] /* DDR  */& via1registers[0]; /* PB */
}

void via1_sr_set(uint8_t value) {
	via1registers[10] = value;
}
