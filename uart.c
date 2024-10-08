// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "uart.h"
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

void loadFile();
void reset_upload() ;

static uint8_t uartregisters[16];

extern int errno;

bool uart_checkUploadLmf = true;
clock_t uart_checkUploadLmfTs = 0;

time_t uart_file_lmf = -1;

uint8_t *p_prg_img;
uint8_t *p_prg_img_ix;

uint16_t prg_size;
uint16_t *p_prg_size;
uint16_t bytes_available;

unsigned char *prg_path;
int prg_override_start;

struct serial_state {
	uint16_t bytes;
	uint8_t (*read)(uint8_t r);
	uint8_t (*write)(uint8_t r, uint8_t v);
};

uint8_t xmodem_blocknr;
uint8_t xmodem_crc16_l = 0;
uint8_t xmodem_crc16_h = 0;

uint8_t uart_xmodem_ready_to_send(uint8_t reg);
uint8_t uart_xmodem_read_block_start(uint8_t reg);
uint8_t uart_xmodem_read_block_end(uint8_t reg);
uint8_t uart_xmodem_read_block_data(uint8_t reg);
uint8_t uart_xmodem_read_eot(uint8_t reg);
void uart_xmodem_write_crc(uint8_t reg, uint8_t val);
void uart_xmodem_write_ack(uint8_t reg, uint8_t val);

uint8_t uart_read_data_bytes(uint8_t r, uint8_t **p_data, uint16_t *c);

static struct serial_state xmodem_protocol[] = { //
        { .read = uart_xmodem_ready_to_send, .write = uart_xmodem_write_crc },
    		{ .read = uart_xmodem_read_block_start }, //
    		{ .read = uart_xmodem_read_block_data }, //
    		{ .read = uart_xmodem_read_block_end }, //
    		{ .read = uart_xmodem_ready_to_send, .write = uart_xmodem_write_ack },
    		{ .read = uart_xmodem_read_eot, .write = uart_xmodem_write_ack },
				{}, //end state
		};

struct serial_state *protocol =
  &xmodem_protocol;
  //&upload_protocol;

unsigned char protocol_ix = 0;

#define XMODEM_SOH  0x01		// start block
#define XMODEM_EOT  0x04		// end of text marker
#define XMODEM_ACK  0x06		// good block acknowledged
#define XMODEM_NAK  0x15		// bad block acknowledged
#define XMODEM_CAN  0x18		// cancel (not standard, not supported)
#define XMODEM_CR   0x0d		// carriage return
#define XMODEM_LF   0x0a		// line feed
#define XMODEM_ESC  0x1b		// ESC to exit

uint8_t xmodem_crc16_tab_l[256];
uint8_t xmodem_crc16_tab_h[256];

void uart_xmodem_write_crc(uint8_t reg, uint8_t val){
	static int c = 3;
  if(val == 'C' && --c == 0){
    loadFile();
    if(p_prg_img){
      protocol_ix++;
      c = 3; //reset for next crc check
      xmodem_blocknr = 1;
    }
  }
}

void uart_xmodem_write_ack(uint8_t reg, uint8_t val){
  if(val == XMODEM_ACK){
    if(p_prg_img_ix == (p_prg_img + prg_size + 2)){//end of prg reached?
      protocol_ix++;// proceed to next protocol step, send EOT
    }else{
      protocol_ix = 1;//if ack, go on with next block set to block_start
    }
  }else if(val == XMODEM_NAK){

  }
}

uint8_t uart_xmodem_ready_to_send(uint8_t reg){
  if (reg == UART_REG_LSR) {
		return lsr_THRE;
	}
  return 0;
}

uint8_t uart_xmodem_read_eot(uint8_t reg){
	static int c = 1;
  if(reg == UART_REG_LSR){
    switch(c--){
      case 1:
        return lsr_DR;
      case 0:
        c = 1;
        reset_upload();
        return lsr_THRE;
    }
  }
	if (reg == UART_REG_RXTX) {
    return XMODEM_EOT;
  }
  return 0;
}

uint8_t uart_xmodem_read_block_start(uint8_t reg){
	static int c = 2;
	if (reg == UART_REG_LSR) {
		return lsr_DR;
	} else if (reg == UART_REG_RXTX) {
    switch(c--){
      case 2:
        return XMODEM_SOH;
      case 1:
        DEBUG("blockno: 0x$%02x\n", xmodem_blocknr);
        return xmodem_blocknr;
      case 0:
        xmodem_crc16_l = 0;
        xmodem_crc16_h = 0;
        bytes_available = 128;// 128 data bytes
        protocol_ix++;
        c = 2; //reset for next block
        return xmodem_blocknr ^ 0xff;
    }
  }
  return 0;
}

uint8_t uart_xmodem_read_block_end(uint8_t reg){
	static int c = 1;
	if (reg == UART_REG_LSR) {
		return lsr_DR;
	} else if (reg == UART_REG_RXTX) {
    switch(c--){
      case 1:
        return xmodem_crc16_h;
      case 0:
        c = 1;
        protocol_ix++;
        xmodem_blocknr++;
        return xmodem_crc16_l;
    }
  }
  return 0;
}

uint8_t uart_xmodem_read_block_data(uint8_t reg){
	if (reg == UART_REG_LSR) {
		return lsr_DR;
	} else if (reg == UART_REG_RXTX) {
    uint8_t b = uart_read_data_bytes(reg, &p_prg_img_ix, &bytes_available);
    uint8_t i = b ^ xmodem_crc16_h;
    xmodem_crc16_h = xmodem_crc16_l ^ xmodem_crc16_tab_h[i];
    xmodem_crc16_l = xmodem_crc16_tab_l[i];
    DEBUG("$%02x%02x\n", xmodem_crc16_h, xmodem_crc16_l);

    return b;
  }
  return XMODEM_NAK;
}

long getFilesize(FILE *file){
	long filesize = -1L;
	if(fseek(file, 0L, SEEK_END) == EOF){
		fprintf(stderr, "error fseek %s\n", strerror(errno));
	}else{
		filesize = ftell(file);
	}
	return filesize;
}

void readProgram(int prg_override_start, FILE *prg_file) {

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
			printf("uart() load program file for address space 0x%04x-0x%04x (size 0x%04x / read 0x%04x)\n",
        *(p_prg_img + 0) | *(p_prg_img + 1) << 8,
				(*(p_prg_img + 0) | *(p_prg_img + 1) << 8) + prg_size,
				prg_size, r);
		} else {
			fprintf(stderr, "uart() load file start 0x%04x size 0x%04x error: %s\n",
					(*(p_prg_img + 0) | *(p_prg_img + 1) << 8), strerror(errno));
			free(p_prg_img);
			p_prg_img = NULL;
		}
	}
}

void loadFile(){

    if(prg_path == NULL)
      return;

#if __MINGW32_NO__
		struct __stat64 attrib;
		int rc = __stat64(prg_path, &attrib);
#else
		struct stat attrib;
		int rc = stat(prg_path, &attrib);
#endif
		if (rc) {
			fprintf(stderr, "could not stat file %s, error was: %s\n", prg_path, strerror(errno));
      reset_upload();
			return;
		}
		if (uart_checkUploadLmf && uart_file_lmf == attrib.st_mtime) {
			DEBUG (stdout, "skip upload of file %s, file has not changed\n", prg_path);
			return;
		}
		uart_file_lmf = attrib.st_mtime;
		FILE *prg_file = fopen(prg_path, "rb");
		if (!prg_file) {
			fprintf(stderr, "uart upload read start address - cannot open file %s, error %s\n", prg_path,
					strerror(errno));
			reset_upload();
			return;
		}
		readProgram(prg_override_start, prg_file);
		fclose(prg_file);
}

void reset_upload() {
	protocol_ix = 0;
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

  for(uint16_t i=0;i<=255;i++){
    xmodem_crc16_tab_l[i] = 0;
    xmodem_crc16_tab_h[i] = 0;
  }
  for(uint16_t i=0;i<=255;i++){
    xmodem_crc16_tab_h[i] = xmodem_crc16_tab_h[i] ^ i;
    for(uint8_t j=8;j>0;j--){
      uint16_t c_l = xmodem_crc16_tab_l[i]<<1;
      uint16_t c_h = xmodem_crc16_tab_h[i]<<1;
      xmodem_crc16_tab_l[i] = c_l & 0xff;
      xmodem_crc16_tab_h[i] = (c_h | (c_l >> 8)) & 0xff;
      if((c_h & 0x100) == 0x100){
        xmodem_crc16_tab_h[i] ^= 0x10;
        xmodem_crc16_tab_l[i] ^= 0x21;
      }
    }
  }
  for(int i=0;i<=255;i++){
    if(i % 16 == 0)
      DEBUG("\n");
    DEBUG("$%02x ", xmodem_crc16_tab_l[i]);
  }
  DEBUG("\n");
  for(int i=0;i<=255;i++){
    if(i % 16 == 0)
      DEBUG("\n");
    DEBUG("$%02x ", xmodem_crc16_tab_h[i]);
  }
  DEBUG("\n");
}

uint8_t uart_read_data_bytes(uint8_t r, uint8_t **p_data, uint16_t *c) {
	if (r == UART_REG_LSR) {
		return lsr_DR;
	} else if (r == UART_REG_RXTX) {
    uint8_t b = 0;
    if(*p_data != (p_prg_img + prg_size + 2)){//end of prg reached?
		  b = *(*p_data)++; //inc given ptr. if byte was read
    }
    if (--(*c) == 0) {
			protocol_ix++;
		}
		return b;
	}
	return 0;
}

uint8_t uart_read(uint8_t reg) {
//	printf("uart r %x\n", reg);
	if (protocol[protocol_ix].read) {
		return protocol[protocol_ix].read(reg);
	}
	return uartregisters[reg];
}

void uart_write(uint8_t reg, uint8_t value) {
//	printf("uart w %x %x\n", reg, value);
	uartregisters[reg] = value;
  if (protocol[protocol_ix].write) {
    protocol[protocol_ix].write(reg, value);
  }
  if (p_prg_img_ix) {

	}
}
