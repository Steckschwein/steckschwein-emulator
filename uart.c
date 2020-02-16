// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/stat.h>
#include "uart.h"
#include "memory.h"

static uint8_t uartregisters[16];

#include <errno.h>
extern int errno;

uint8_t *p_prg_img = NULL;
uint8_t *p_prg_img_ix = NULL;

uint16_t prg_size;
uint16_t *p_prg_size;
uint16_t bytes_available;
unsigned char upload_protocol_ix = 0;

unsigned char *prg_path;
int prg_override_start;

struct upload_state {
	uint16_t bytes;
	uint8_t (*read)(uint8_t r);
	uint8_t (*write)(uint8_t r, uint8_t v);
};

//static int c = 2;
uint8_t upload_read_startAddress(uint8_t r);
uint8_t upload_read_OK(uint8_t r);
void upload_write_OK(uint8_t r, uint8_t v);
uint8_t upload_read_length(uint8_t r);
uint8_t upload_read_program(uint8_t r);

static struct upload_state upload_protocol[] = { //
		{ .read = upload_read_startAddress, }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ .read = upload_read_length }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ .read = upload_read_program }, //
				{ .read = upload_read_OK, .write = upload_write_OK }, //
				{ }, //end state
		};

void loadFile(int prg_override_start, FILE *prg_file) {

	struct stat file_stat;
	int r = fstat(prg_file->_fileno, &file_stat);
	if (r) {
		fprintf(stderr, "error fstat %s\n", strerror(errno));
	} else {
		uint8_t offs = (prg_override_start == -1 ? 2 : 0);
		prg_size = file_stat.st_size - offs; //-offs byte, if start address is given as argument
		p_prg_img = p_prg_img_ix = malloc(prg_size + (2 - offs)); //align memory for prg image, we always allocate 2 byte + prg. image size
		p_prg_size = &prg_size;
		if (p_prg_img_ix == NULL) {
			fprintf(stderr, "out of memory\n");
		}
		if (prg_override_start != -1) {
			*(p_prg_img_ix + 0) = prg_override_start & 0xff;
			*(p_prg_img_ix + 1) = prg_override_start >> 8 & 0xff;
		}
		r = fread(p_prg_img_ix + (2 - offs), 1, prg_size, prg_file);
		if (r) {
			printf("uart() load file, start 0x%04x size 0x%04x\n",
					(*(p_prg_img_ix + 0) | *(p_prg_img_ix + 1) << 8), prg_size);
		} else {
			fprintf(stderr,
					"uart() load file, start 0x%04x size 0x%04x error: %s\n",
					(*(p_prg_img_ix + 0) | *(p_prg_img_ix + 1) << 8),
					strerror(errno));
			free(p_prg_img);
		}
		bytes_available = 2;
	}
}

void uart_init(unsigned char *p_prg_path, int p_prg_override_start) {
	prg_path = p_prg_path;
	prg_override_start = p_prg_override_start;
	reset_upload();
}

uint8_t upload_read_bytes(uint8_t r, uint8_t **p_data, uint16_t *c) {
	if (r == UART_REG_LSR) {
		return lsr_DR;
	} else if (r == UART_REG_IER) {
		if (--(*c) == 0) {
			upload_protocol_ix++;
			bytes_available = 2;
		}
		uint8_t b = *(*p_data)++; //inc given ptr. if byte was read
		return b;
	}
	return 0;
}

uint8_t upload_read_startAddress(uint8_t r) {
	if (!p_prg_img) {
		FILE *prg_file = fopen(prg_path, "rb");
		if (!prg_file) {
			fprintf(stderr, "uart_init() - cannot open file %s, error %s\n",
					prg_path, strerror(errno));
			return 0;
		}
		loadFile(prg_override_start, prg_file);
		fclose(prg_file);
	}
	return upload_read_bytes(r, &p_prg_img_ix, &bytes_available);
}

uint8_t upload_read_length(uint8_t r) {
	return upload_read_bytes(r, &p_prg_size, &bytes_available);
}

uint8_t upload_read_program(uint8_t r) {
	uint8_t outbyte = upload_read_bytes(r, &p_prg_img_ix, &prg_size);
	return outbyte;
}

uint8_t upload_read_OK(uint8_t r) {
	if (r == UART_REG_LSR) {
		return lsr_THRE;
	}
	return lsr_DR;
}

void reset_upload() {
	upload_protocol_ix = 0;
	if (p_prg_img != NULL) {
		free(p_prg_img); //free upon start
		p_prg_img = NULL;
		p_prg_img_ix = NULL;
	}
}

void upload_write_OK(uint8_t r, uint8_t v) {
	static int c = 2;
	if (--c == 0) {
		upload_protocol_ix++;
		c = 2; //reset, for next "OK" check
		if (upload_protocol_ix >= 6) { //
			reset_upload();
		}
	}
}

uint8_t uart_read(uint8_t reg) {
//	printf("uart r %x\n", reg);
	if (upload_protocol[upload_protocol_ix].read) {
		return upload_protocol[upload_protocol_ix].read(reg);
	}
	return uartregisters[reg];
}

void uart_write(uint8_t reg, uint8_t value) {
//	printf("uart w %x %x\n", reg, value);
	uartregisters[reg] = value;
	if (p_prg_img_ix) {
		if (upload_protocol[upload_protocol_ix].write) {
			upload_protocol[upload_protocol_ix].write(reg, value);
		}
	}
}
