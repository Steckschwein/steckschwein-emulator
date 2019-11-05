// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "via.h"
#include "vdp.h"
#include "uart.h"
#include "opl2.h"

#include "memory.h"
#include "video.h"

uint8_t ctrl_port;

uint8_t *RAM;
uint8_t ROM[ROM_SIZE];

#define DEVICE_EMULATOR (0x9fb0)

void
memory_init()
{
	RAM = malloc(RAM_SIZE);
	ctrl_port = 0;
}

//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever

uint8_t
read6502(uint16_t address) {
	return real_read6502(address, false, 0);
}

uint8_t
real_read6502(uint16_t address, bool debugOn, uint8_t bank)
{
//	printf("read6502 %x %x\n", address, bank);

	if (address < 0x0200)
	{ // RAM
		return RAM[address];
	}
	else if (address < 0x0280) { // I/O
		// TODO I/O map?
		if (address  < 0x210) // UART at $0200
		{
			return uart_read(address & 0xf);
		}
		else if (address < 0x0220) // VIA at $0210
		{
			return via2_read(address & 0xf);
		}
		else if (address < 0x0230) // VDP at $0220
		{
			return vdp_read(address & 0xf);
		}
		else if (address < 0x0240) // latch at $0x0230
		{
			return ctrl_port;
		}
		else if (address < 0x0250) // OPL2 at $0240
		{
			return opl2_read(address & 0xf);
		}
		else
		{
			return emu_read(address & 0xf);
		}
	} else {
		if (address < 0xe000 || ctrl_port & 1){
			return RAM[address];// RAM
		}
		/* bank select upon ctrl_port - see steckos/asminc/system.inc
			BANK_SEL0 = 0010
			BANK_SEL1 = 0100 
			BANK0 = 0000
			BANK1 = 0010
			BANK2 = 0100
			BANK3 = 0110
		*/
		return ROM[(address & 0x1fff) | ((ctrl_port & 0x06)<<12)];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
//	printf("write6502 %x %x\n", address, value);
	if (address < 0x0200) { // RAM
		RAM[address] = value;
	} else if (address < 0x0280) { // I/O
		if (address  < 0x210) // UART at $0200
		{
			return uart_write(address & 0xf, value);
		}
		else if (address < 0x0220) // VIA at $0210
		{
			return via2_write(address & 0xf, value);
		}
		else if (address < 0x0230) // VDP at $0220
		{
			return vdp_write(address & 0xf, value);
		}
		else if (address < 0x0240) // latch at $0x0230
		{
			ctrl_port = value;
			if (value == 1)
				printf("ctrl_port %x\n", ctrl_port);
		}
		else if (address < 0x0250) // OPL2 at $0240
		{
			return opl2_write(address & 0xf, value);
		}

		// TODO I/O map?
/*
		if (address < 0x0310) {
			via1_write(address & 0xf, value);
		} else {
			emu_write(address & 0xf, value);
		}
*/
	// } else if (address < 0xe000) { // RAM
		// RAM[address] = value;
	} else { // Writes go to ram, regardless if ROM active or not
		// ignore
		RAM[address] = value;
	}
}

//
// saves the memory content into a file
//

void
memory_save(FILE *f, bool dump_ram, bool dump_bank)
{
	fwrite(RAM, sizeof(uint8_t), RAM_SIZE, f);
}


///
///
///

// Control the GIF recorder
void
emu_recorder_set(gif_recorder_command_t command)
{
	// turning off while recording is enabled
	if (command == RECORD_GIF_PAUSE && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_PAUSED; // need to save
	}
	// turning on continuous recording
	if (command == RECORD_GIF_RESUME && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_ACTIVE;		// activate recording
	}
	// capture one frame
	if (command == RECORD_GIF_SNAP && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_SINGLE;		// single-shot
	}
}

//
// read/write emulator state (feature flags)
//
// 0: debugger_enabled
// 1: log_video
// 2: log_keyboard
// 3: echo_mode
// 4: save_on_exit
// 5: record_gif
// POKE $9FB3,1:PRINT"ECHO MODE IS ON":POKE $9FB3,0
void
emu_write(uint8_t reg, uint8_t value)
{
	bool v = value != 0;
	switch (reg) {
		case 0: debugger_enabled = v; break;
		case 1: log_video = v; break;
		case 2: log_keyboard = v; break;
		case 3: echo_mode = v; break;
		case 4: save_on_exit = v; break;
		case 5: emu_recorder_set((gif_recorder_command_t) value); break;
		default: printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	}
}

uint8_t
emu_read(uint8_t reg)
{
	if (reg == 0) {
		return debugger_enabled ? 1 : 0;
	} else if (reg == 1) {
		return log_video ? 1 : 0;
	} else if (reg == 2) {
		return log_keyboard ? 1 : 0;
	} else if (reg == 3) {
		return echo_mode;
	} else if (reg == 4) {
		return save_on_exit ? 1 : 0;
	} else if (reg == 5) {
		return record_gif;
	} else if (reg == 13) {
		return keymap;
	} else if (reg == 14) {
		return '1'; // emulator detection
	} else if (reg == 15) {
		return '6'; // emulator detection
	}
	printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	return -1;
}
