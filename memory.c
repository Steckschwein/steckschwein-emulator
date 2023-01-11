// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "via.h"
#include "uart.h"
#include "ym3812.h"

#include "memory.h"

uint8_t ctrl_port;
uint8_t *ram;
uint8_t *rom;

#define DEVICE_EMULATOR (0x9fb0)

void memory_init() {
	ram = malloc(RAM_SIZE);
	rom = malloc(ROM_SIZE);
	ctrl_port = 0;
}

uint8_t memory_get_ctrlport() {
	return ctrl_port;
}

void memory_destroy() {
	free(ram);
	free(rom);
}

//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever

uint8_t read6502(uint16_t address) {
	return real_read6502(address, false, 0);
}

uint8_t real_read6502(uint16_t address, bool debugOn, uint8_t bank) {
//	printf("read6502 %x %x\n", address, bank);

	if (address < 0x0200) { // RAM
		return ram[address];
	} else if (address < 0x0280) { // I/O
               // TODO I/O mapper
		if (address < 0x210) // UART at $0200
				{
			  return ioPortRead(NULL, address);
          // TODO FIXME api
          // return uart_read(uartIo0x200, address & 0xf);
		} else if (address < 0x0220) // VIA at $0210
				{
			return via1_read(address & 0xf);
		} else if (address < 0x0230) // VDP at $0220
				{
			return ioPortRead(NULL, address);
		} else if (address < 0x0240) // latch at $0x0230
				{
			return ctrl_port;
		} else if (address < 0x0250) // OPL2 at $0240
				{
			return ioPortRead(NULL, address);
		} else {
			return emu_read(address & 0xf);
		}
	} else {
		if (address < 0xe000 || (ctrl_port & 1)) {
			return ram[address]; // RAM
		}
		/* bank select upon ctrl_port - see steckos/asminc/system.inc
		 BANK_SEL0 = 0010
		 BANK_SEL1 = 0100
		 BANK0 = 0000
		 BANK1 = 0010
		 BANK2 = 0100
		 BANK3 = 0110
		 */
		return rom[(address & 0x1fff) | ((ctrl_port & 0x06) << 12)];
	}
}

void write6502(uint16_t address, uint8_t value) {
//	printf("write6502 %x %x\n", address, value);
	if (address < 0x0200) { // RAM
		ram[address] = value;
	} else if (address < 0x0280) { // I/O
		if (address < 0x210) // UART at $0200
				{
      // TODO FIXME api
//        return uart_write(uartIo0x200, address & 0xf, value);
			  return ioPortWrite(NULL, address, value);
		} else if (address < 0x0220) // VIA at $0210
				{
			return via1_write(address & 0xf, value);
		} else if (address < 0x0230) // VDP at $0220
				{
			ioPortWrite(NULL, address, value);
			return;
		} else if (address < 0x0240) // latch at $0x0230
				{
			ctrl_port = value;
			DEBUG ("ctrl_port %x\n", ctrl_port);
		} else if (address < 0x0250) // OPL2 at $0240
				{
			ioPortWrite(NULL, address, value);
			return;
		}

		// TODO I/O map?
		/*
		 if (address < 0x0310) {
		 via1_write(address & 0xf, value);
		 } else {
		 emu_write(address & 0xf, value);
		 }
		 */
		// } else if (address < 0xe000) { // ram
		// ram[address] = value;
	} else {
		// Writes go to ram, regardless if ROM active or not
		ram[address] = value;
	}
}

//
// saves the memory content into a file
//

void memory_save(FILE *f, bool dump_ram, bool dump_bank) {
	fwrite(ram, sizeof(uint8_t), RAM_SIZE, f);
}

///
///
///

// Control the GIF recorder
void emu_recorder_set(gif_recorder_command_t command) {
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
void emu_write(uint8_t reg, uint8_t value) {
	bool v = value != 0;
	switch (reg) {
	case 0:
		isDebuggerEnabled = v;
		break;
	case 1:
		log_video = v;
		break;
	case 2:
		log_keyboard = v;
		break;
	case 3:
		echo_mode = v;
		break;
	case 4:
		save_on_exit = v;
		break;
	case 5:
		emu_recorder_set((gif_recorder_command_t) value);
		break;
	default:
		printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	}
}

uint8_t emu_read(uint8_t reg) {
	if (reg == 0) {
		return isDebuggerEnabled ? 1 : 0;
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
