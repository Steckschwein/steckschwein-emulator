// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "uart.h"
#include "memory.h"
#include "glue.h"

#define UART_CHECK_UPLOAD_INTERVAL_SECONDS 3

/**

# serial loopback
$ socat -d -d pty,link=/tmp/ssw_uart0,raw,echo=0 pty,link=/tmp/ssw_emu_uart0,raw,echo=0 &
2022/09/17 12:02:50 socat[2554893] N PTY is /dev/pts/32
2022/09/17 12:02:50 socat[2554893] N PTY is /dev/pts/33
2022/09/17 12:02:50 socat[2554893] N starting data transfer loop with FDs [5,5] and [7,7]

# host device
$ cat /tmp/ssw_emu_uart0 & # im emu mit open("/tmp/ssw... 

# emulator device
$ echo "Hallo Thomas... some serial" > /tmp/ssw_uart0 
Hallo Thomas... some serial

*/

static uint8_t uartregisters[16];

extern int errno;

bool uart_checkUploadLmf = false;
clock_t uart_checkUploadLmfTs = 0;

time_t uart_file_lmf = -1;

uint8_t *p_prg_img;
uint8_t *p_prg_img_ix;

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

long getFilesize(FILE *file){

	long filesize = -1L;
	
	if(fseek(file, 0L, SEEK_END) == EOF){
		fprintf(stderr, "error fseek %s\n", strerror(errno));
	}else{
		filesize = ftell(file);
	}
	return filesize;
}

void loadFile(int prg_override_start, FILE *prg_file) {

	long filesize = getFilesize(prg_file);
	if (filesize == -1L) {
		fprintf(stderr, "invalid file size %s\n", strerror(errno));
	} else {
		if(fseek(prg_file, 0L, SEEK_SET) == EOF){
			fprintf(stderr, "error fseek %s\n", strerror(errno));
			return;
		}
		uint8_t offs = (prg_override_start == -1 ? 2 : 0);
		prg_size = filesize - offs; //-offs byte, if start address is given as argument
		p_prg_size = &prg_size;
		p_prg_img = p_prg_img_ix = malloc(2 + prg_size); //align memory for prg image, we always allocate 2 byte start address + prg. image size
		if (p_prg_img == NULL) {
			fprintf(stderr, "out of memory\n");
			return;
		}
		if (prg_override_start != -1) {
			*(p_prg_img + 0) = prg_override_start & 0xff;
			*(p_prg_img + 1) = prg_override_start >> 8 & 0xff;
		}
		size_t r = fread((p_prg_img + (2 - offs)), 1, filesize, prg_file);
		if (r) {
			printf("uart() load program to 0x%04x-0x%04x (size 0x%04x / read 0x%04x)\n", 
				*(p_prg_img + 0) | *(p_prg_img + 1) << 8,
				(*(p_prg_img + 0) | *(p_prg_img + 1) << 8) + prg_size, 
				prg_size, r);
		} else {
			fprintf(stderr, "uart() load file start 0x%04x size 0x%04x error: %s\n",
					(*(p_prg_img + 0) | *(p_prg_img + 1) << 8), strerror(errno));
			free(p_prg_img);
			p_prg_img = NULL;
		}
		bytes_available = 2;
	}
}

void reset_upload() {
	upload_protocol_ix = 0;
	if (p_prg_img != NULL) {
		free(p_prg_img); //free
		p_prg_img = NULL;
		p_prg_img_ix = NULL;
	}
}

void uart_init(unsigned char *p_prg_path, int p_prg_override_start, bool checkLmf) {
	prg_path = p_prg_path;
	prg_override_start = p_prg_override_start;
	uart_checkUploadLmf = checkLmf;
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
    
    if (!p_prg_img && prg_path && (uart_checkUploadLmfTs + UART_CHECK_UPLOAD_INTERVAL_SECONDS < (uart_checkUploadLmfTs = clock()))) {
		#if __MINGW32_NO__
		struct __stat64 attrib;
		int rc = __stat64(prg_path, &attrib);
		#else
		struct stat attrib;
		int rc = stat(prg_path, &attrib);
		#endif
		if (rc) {
			fprintf(stderr, "could not stat file %s, error was %s\n", prg_path, strerror(errno));
			return 0;
		}
		if (uart_checkUploadLmf && uart_file_lmf == attrib.st_mtime) {
			DEBUG (stdout, "skip upload of file %s, file has not changed\n", prg_path);
			return 0;
		}
		uart_file_lmf = attrib.st_mtime;
		FILE *prg_file = fopen(prg_path, "rb");
		if (!prg_file) {
			fprintf(stderr, "uart upload read start address - cannot open file %s, error %s\n", prg_path,
					strerror(errno));
			reset_upload();
			return 0;
		}
		loadFile(prg_override_start, prg_file);
		fclose(prg_file);
	}
	if (p_prg_img) {
		return upload_read_bytes(r, &p_prg_img_ix, &bytes_available);
	}
	return 0;
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
