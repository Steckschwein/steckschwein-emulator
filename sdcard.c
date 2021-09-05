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

static uint8_t *p_data = NULL;
static int data_length = 0;
static int data_counter = 0;
static uint8_t mblock = 0; //multi block counter

int readBlock(uint32_t lba){
	int res = fread(&block_response[2], sizeof(uint8_t), BLOCK_SIZE, sdcard_file);
	if (ferror(sdcard_file)) {
		fprintf(stderr, "fread error lba %x, block $%x (lba: %x) - %s\n", lba + mblock, mblock, lba, strerror(errno));
		mblock = 0;
	}else{
		if (res != BLOCK_SIZE) {
			WARN("Warning: short read lba %x, block $%x (lba: %x) - %d/%d bytes!\n", lba + mblock, mblock, lba, res, BLOCK_SIZE);
		}
		p_data = block_response;
		data_length = 2 + BLOCK_SIZE + 2 + 2;
		data_counter = 0; //start over
		block_response[0] = 0;
		block_response[1] = 0xfe;
		memset(&block_response[2 + BLOCK_SIZE + 2], 0xff, 4);
	}
	return res;
}


uint8_t spi_sdcard_handle(uint8_t inbyte) {

	if (!sdcard_file)
		return 0;

	static uint8_t cmd[6];
	static uint8_t last_cmd;

	uint8_t outbyte = 0xff;

	if (p_data) {
		outbyte = p_data[data_counter]; //read one byte from buffer
		p_data[data_counter] = inbyte; // write one byte to buffer
		data_counter++;
		if (data_counter == data_length) {

			if (last_cmd == 0x40 + 24) { //write block if it was requested
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				printf("Writing lba %x, block $%x (lba: %x)\n", lba + mblock, mblock, lba);
				for (int i = 0; i < data_length; ++i) {
					DEBUG("%x ", p_data[i]);
				}
				int res = fwrite(&block_response[2], sizeof(uint8_t), BLOCK_SIZE, sdcard_file);
				if (ferror(sdcard_file)) {
					fprintf(stderr, "fwrite error lba %x, block $%x (lba: %x) - %s\n", lba + mblock, mblock, lba, strerror(errno));
				}else if (res != BLOCK_SIZE) {
					WARN("Warning: short write lba %x, block $%x (lba: %x) - %d/%d bytes!\n", lba + mblock, mblock, lba, res, BLOCK_SIZE);
				}
			}
			p_data = NULL;
		}
	} else if (cmd_receive_counter == 0 && inbyte == 0xff) {
		if (!p_data) { // send response data
			if (last_cmd == 0x40 + 18) { //last captured cmd was read multiblock
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				DEBUG("Reading LBA %d multiblock (%d)\n", lba + mblock, mblock);

				int res = readBlock(lba);
				mblock++;
			}
		}
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
				p_data = block_response;
				data_length = sizeof(r);
				break;
			}
			case 0x40 + 8: { //CMD8: SEND_IF_COND
				static const uint8_t r[] = { 1, 0x00, 0x00, 0x01, 0xaa };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			case 0x40 + 12: { //CMD12 - end transmission, e.g. on multi block read
				mblock = 0;
				static const uint8_t r[] = { 0 }; //1, 0x00, 0x00, 0x01, 0xaa };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			case 0x40 + 41: {
				static const uint8_t r[] = { 0 };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			case 0x40 + 16: {
				// set block size
				DEBUG("set block size = 0x%04x\n", cmd[2] << 8 | cmd[3]);
				static const uint8_t r[] = { 1 };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			case 0x40 + 17: { // read block
				mblock = 0;
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				block_response[0] = 0;
				block_response[1] = 0xfe;
				DEBUG("Reading LBA %d\n", lba);
				int fs = fseek(sdcard_file, lba * BLOCK_SIZE, SEEK_SET);
				if(fs){
					fprintf(stderr, "error fseek %s\n", strerror(errno));
				}else{
					int res = readBlock(lba);
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
				mblock = 0;
				break;
			case 0x40 + 24: { // write block (0x18)
				uint32_t lba = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
				int fs = fseek(sdcard_file, lba * BLOCK_SIZE, SEEK_SET);
				if(fs){
					fprintf(stderr, "error fseek %s\n", strerror(errno));
				}else{
					block_response[0] = 0;
					block_response[1] = 0xfe;
					memset(&block_response[2 + BLOCK_SIZE + 2], 0xff, 4);
					p_data = block_response;
					data_length = 2 + BLOCK_SIZE + 2 + 2;
				}
				mblock = 0;
				break;
			}
			case 0x40 + 55: { //CMD55 APP_CMD
				static const uint8_t r[] = { 1 };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			case 0x40 + 58: { //READ_OCR
				static const uint8_t r[] = { 0, 0, 0, 0 };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			default: {
				static const uint8_t r[] = { 0 };
				data_length = sizeof(r);
				memcpy(block_response, r, data_length);
				p_data = block_response;
				break;
			}
			}
			last_cmd = cmd[0];
			data_counter = 0;
		}
	}

	DEBUG("$%02x => $%02x \n", inbyte, outbyte);

	return outbyte;
}
