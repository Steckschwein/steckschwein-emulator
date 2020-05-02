// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "sdcard.h"

#define BLOCK_SIZE 512

static int cmd_receive_counter;
FILE *sdcard_file = NULL;

void spi_sdcard_select() {
	cmd_receive_counter = 0;
}

void spi_sdcard_deselect() {
}

uint8_t block_response[2 //data token
+ BLOCK_SIZE //
		+ 2 //block check crc
		+ 4 //busy wait
];

//uint8_t block_write[BLOCK_SIZE];

uint8_t spi_sdcard_handle(uint8_t inbyte) {

	if (!sdcard_file)
		return 0;

	static uint8_t cmd[6];
	static uint8_t last_cmd;

	static uint8_t *response = NULL;
	static int response_length = 0;
	static int response_counter = 0;

	static uint8_t mblock = 0; //multi block counter
	uint8_t outbyte = 0xff;

	if (response) {
		outbyte = response[response_counter]; //read byte
		response[response_counter] = inbyte; // write byte to buffer
		response_counter++;
		if (response_counter == response_length) {

			if (last_cmd == 0x40 + 24) { //write block
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				printf("Writing LBA %d block (%d)\n", lba + mblock, mblock);
				for (int i = 0; i < response_length; ++i) {
					DEBUG("%x ", response[i]);
				}
				int res = fwrite(&block_response[2], 1, BLOCK_SIZE, sdcard_file);
				if (!res) {
					fprintf(stderr, "Error fwrite file lba: %x, block $%x: %s\n", mblock, lba, strerror(errno));
				}
				if (res != BLOCK_SIZE) {
					WARN("Warning: short write bytes %d/%d'!\n", res, BLOCK_SIZE);
				}
			}

			response = NULL;
		}
	} else if (cmd_receive_counter == 0 && inbyte == 0xff) {
		if (!response) { // send response data
			if (last_cmd == 0x40 + 18) { //last captured cmd is read multiblock
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				DEBUG("Reading LBA %d multiblock (%d)\n", lba + mblock, mblock);

				int res = fread(&block_response[2], 1, BLOCK_SIZE, sdcard_file);
				if (!res) {
					fprintf(stderr, "fread error block $%x, lba %x: %s\n", mblock, lba, strerror(errno));
				}
				if (res != BLOCK_SIZE) {
					WARN("Warning: short read bytes %d/%d'!\n", res, BLOCK_SIZE);
				}
				block_response[0] = 0;
				block_response[1] = 0xfe;
				memset(&block_response[2 + BLOCK_SIZE + 2], 0xff, 4);
				response = block_response;
				response_length = 2 + BLOCK_SIZE + 2 + 2;
				response_counter = 0; //start over
				mblock++;
			}
		}
//		if (response) {
//			outbyte = response[response_counter++];
//			if (response_counter == response_length) {
//				response = NULL;
//			}
//		}
	} else {
		cmd[cmd_receive_counter++] = inbyte;
		if (cmd_receive_counter == 6) { //cmd length 6 byte
			cmd_receive_counter = 0;
			last_cmd = 0;
//			printf("*** COMMAND: $40 + %d\n", cmd[0] - 0x40);
			switch (cmd[0]) {
			case 0x40: {
				// CMD0: init SD card to SPI mode
				static const uint8_t r[] = { 1 };
				block_response[0] = 1;
				response = block_response;
				response_length = sizeof(r);
				break;
			}
			case 0x40 + 8: { //CMD8: SEND_IF_COND
				static const uint8_t r[] = { 1, 0x00, 0x00, 0x01, 0xaa };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			case 0x40 + 12: { //CMD12 - end transmission, e.g. on multi block read
				mblock = 0;
				static const uint8_t r[] = { 0 }; //1, 0x00, 0x00, 0x01, 0xaa };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			case 0x40 + 41: {
				static const uint8_t r[] = { 0 };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			case 0x40 + 16: {
				// set block size
				DEBUG("set block size = 0x%04x\n", cmd[2] << 8 | cmd[3]);
				static const uint8_t r[] = { 1 };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			case 0x40 + 17: { // read block
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				block_response[0] = 0;
				block_response[1] = 0xfe;
				DEBUG("Reading LBA %d\n", lba);
				int fs = fseek(sdcard_file, lba * BLOCK_SIZE, SEEK_SET);
				if(fs){
					fprintf(stderr, "error fseek %s\n", strerror(errno));
				}else{
					int bytes_read = fread(&block_response[2], 1, BLOCK_SIZE, sdcard_file);
					if (bytes_read != BLOCK_SIZE) {
						WARN("Warning: short read lba %x, bytes %d/%d'!\n", lba, bytes_read, BLOCK_SIZE);
					}
					response = block_response;
					response_length = 2 + BLOCK_SIZE + 2;
				}
				break;
			}
			case 0x40 + 18: { // CMD18 - read multi block
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				int fs = fseek(sdcard_file, lba * BLOCK_SIZE, SEEK_SET); //do just the seek here, the fread is done above
				if(fs){
					fprintf(stderr, "error fseek %s\n", strerror(errno));
				}
				mblock = 0;
				break;
			}
			case 0x40 + 25:  // write multi block
				break;
			case 0x40 + 24: { // write block
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				int fs = fseek(sdcard_file, lba * BLOCK_SIZE, SEEK_SET);
				if(fs){
					fprintf(stderr, "error fseek %s\n", strerror(errno));
				}else{
					block_response[0] = 0;
					block_response[1] = 0xfe;
					memset(&block_response[2 + BLOCK_SIZE + 2], 0xff, 4);
					response = block_response;
					response_length = 2 + BLOCK_SIZE + 2 + 2;
					mblock = 0;
				}
				break;
			}
			case 0x40 + 55: { //CMD55 APP_CMD
				static const uint8_t r[] = { 1 };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			case 0x40 + 58: { //READ_OCR
				static const uint8_t r[] = { 0, 0, 0, 0 };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			default: {
				static const uint8_t r[] = { 0 };
				response_length = sizeof(r);
				memcpy(block_response, r, response_length);
				response = block_response;
				break;
			}
			}
			last_cmd = cmd[0];
			response_counter = 0;
		}
	}

	DEBUG("$%02x => $%02x \n", inbyte, outbyte);

	return outbyte;
}
