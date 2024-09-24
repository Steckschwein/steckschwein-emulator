// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <glue.h>
#include "MOS6502.h"
#include "Board.h"
#include "cpu/mnemonics.h"				// Automatically generated mnemonic table.

// *******************************************************************************************
//
//		Disassemble a single 65C02 instruction into buffer. Returns the length of the
//		instruction in total in bytes.
//
// *******************************************************************************************
int disasm(uint16_t pc, char *line, unsigned int max_line, bool debugOn, uint8_t bank) {

	uint8_t opcode = read6502Debug(pc, debugOn, bank);
	char *mnemonic = mnemonics[opcode];

	//
	//		Test for branches, relative address. These are BRA ($80) and
	//		$10,$30,$50,$70,$90,$B0,$D0,$F0.
	int isBranch = (opcode == 0x80) || ((opcode & 0x1F) == 0x10);
	//		Test for BBx opcode
	int isBBx = ((opcode & 0x0F) == 0x0F);

	int length = 1;

	strncpy(line,mnemonic,max_line);

	if (strstr(line,"%02x")) {
		length = 2;
		if (isBBx){
			length = 3;
			snprintf(line, max_line, mnemonic, read6502Debug(pc+1, debugOn, bank), pc+3 + (int8_t)read6502Debug(pc+2, debugOn, bank));
		}else if (isBranch) {
			snprintf(line, max_line, mnemonic, pc+2 + (int8_t)read6502Debug(pc+1, debugOn, bank));
		} else {
			snprintf(line, max_line, mnemonic, read6502Debug(pc+1, debugOn, bank));
		}
	}
	if (strstr(line,"%04x")) {
		length = 3;
		snprintf(line, max_line, mnemonic, read6502Debug(pc+1, debugOn, bank) | read6502Debug(pc+2, debugOn, bank)<<8);
	}
  DEBUG("%s\n", line);
	return length;
}
