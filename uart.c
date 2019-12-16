// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"
#include <sys/stat.h>
//XXX
#include "glue.h"

static uint8_t uartregisters[16];

#include <errno.h>
extern int errno;

static uint8_t *p_prg_img = NULL;
static uint16_t prg_size;
static uint16_t *p_prg_size;
static upload_protocol_ix = 0;

void uart_init(FILE *prg_file, int prg_override_start) {
	srand(time(NULL));
	if (prg_file) {
		struct stat file_stat;
		int r = fstat(prg_file->_fileno, &file_stat);
		if (r) {
			printf("error fstat %s\n", strerror(errno));
		} else {
			prg_size = file_stat.st_size - 2; //-2 byte start address
			p_prg_size = &prg_size;
			p_prg_img = malloc(file_stat.st_size);
			if (p_prg_img == NULL) {
				fprintf(stderr, "out of memory\n");
			}
			r = fread(p_prg_img, 1, file_stat.st_size, prg_file);
			if (r) {
				printf("uart() load program, start $%x size $%x\n",
						*(p_prg_img + 1) << 8 | *p_prg_img, prg_size);
			}
		}
		fclose(prg_file);
		prg_file = NULL;
	}

}

uint8_t upload_read_bytes(uint8_t r, uint8_t **p_data, int *c) {
	if (r == 5) {
		return lsr_DR;
	} else if (r == 0) {
		if (--(*c) == 0) {
			upload_protocol_ix++;
		}
		uint8_t b = *(*p_data)++; //inc given ptr. if byte was read
		return b;
	}
	return 0;
}

uint8_t upload_read_startAddress(uint8_t r) {
	static int c = 2;
	return upload_read_bytes(r, &p_prg_img, &c);
}

uint8_t upload_read_length(uint8_t r) {
	static int c = 2;
	return upload_read_bytes(r, &p_prg_size, &c);
}

uint8_t upload_read_program(uint8_t r) {
	uint8_t outbyte = upload_read_bytes(r, &p_prg_img, &prg_size);
	return outbyte;
}

uint8_t upload_read_OK(uint8_t r) {
	if (r == 5) {
		return lsr_THRE;
	}
	return lsr_DR;
}

void upload_write_OK(uint8_t r, uint8_t v) {
	static int c = 2;
	if (--c == 0) {
		upload_protocol_ix++;
		c = 2; //reset, for next "OK" check
	}
}

struct upload_state {
	uint8_t (*read)(uint8_t r);
	uint8_t (*write)(uint8_t r, uint8_t v);
};

struct upload_state upload_protocol[] = { //
		{ .read = upload_read_startAddress }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ .read = upload_read_length }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ .read = upload_read_program }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ .read = NULL, .write = NULL }, //
		};

uint8_t uart_read(uint8_t reg) {
//	printf("uart r %x\n", reg);
	if (p_prg_img) {
		if (upload_protocol[upload_protocol_ix].read) {
			return upload_protocol[upload_protocol_ix].read(reg);
		}
	}
	return uartregisters[reg];
}

void uart_write(uint8_t reg, uint8_t value) {
//	printf("uart w %x %x\n", reg, value);
	uartregisters[reg] = value;
	if (p_prg_img) {
		if (upload_protocol[upload_protocol_ix].write) {
			upload_protocol[upload_protocol_ix].write(reg, value);
		}
	}
}
