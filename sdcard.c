// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <errno.h>

#include "sdcard.h"

static int cmd_receive_counter;
FILE *sdcard_file = NULL;

void
sdcard_select()
{
	cmd_receive_counter = 0;
}

uint8_t
sdcard_handle(uint8_t inbyte)
{
	static uint8_t cmd[6];

	static const uint8_t *response = NULL;
	static int response_length = 0;
	static int response_counter = 0;
	static uint8_t mblock = 0;//multi block counter
	uint8_t outbyte;

	if (cmd_receive_counter == 0 && inbyte == 0xff) {
		// send response data
		if (!response && cmd[0] == 0x40 + 18){//last cmd is read multiblock
			uint32_t lba =
			cmd[1] << 24 |
			cmd[2] << 16 |
			cmd[3] << 8 |
			cmd[4];
			static uint8_t read_block_response[2 + 512 + 2];
			read_block_response[0] = 0;
			read_block_response[1] = 0xfe;

			printf("\nReading LBA %d multiblock (%d)\n", lba+mblock, mblock);

			int bytes_read = fread(&read_block_response[2], 1, 512, sdcard_file);
			if (!bytes_read) {
				fprintf(stderr, "read block ($%x): Error fread file: (lba: %x): %s\n", mblock, lba, strerror(errno));
			}
			if (bytes_read != 512) {
				printf("Warning: short read!\n");
			}
			response = read_block_response;
			response_length = 2 + 512 + 2;
			response_counter = 0;//start over
			mblock++;
		}
		if (response) {
			outbyte = response[response_counter++];
			if (response_counter == response_length) {
				response = NULL;
			}
		} else {
			outbyte = 0xff;
		}
	} else {
		cmd[cmd_receive_counter++] = inbyte;
		if (cmd_receive_counter == 6) {//cmd length 6 byte
			cmd_receive_counter = 0;

//							printf("*** COMMAND: $40 + %d\n", cmd[0] - 0x40);
			switch (cmd[0]) {
				case 0x40: {
					// CMD0: init SD card to SPI mode
					static const uint8_t r[] = { 1 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 8: {
					static const uint8_t r[] = { 1, 0x00, 0x00, 0x01, 0xaa };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 12: { //CMD12 - end transmission, e.g. on multi block read
					static const uint8_t r[] = { 1, 0x00, 0x00, 0x01, 0xaa };
					mblock = 0;
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 41: {
					static const uint8_t r[] = { 0 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 16: {
					// set block size
					printf("Set block size = 0x%04x\n", cmd[2] << 8 | cmd[3]);
					static const uint8_t r[] = { 1 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 17: { // read block
					uint32_t lba =
					cmd[1] << 24 |
					cmd[2] << 16 |
					cmd[3] << 8 |
					cmd[4];
					static uint8_t read_block_response[2 + 512 + 2];
					read_block_response[0] = 0;
					read_block_response[1] = 0xfe;
					printf("Reading LBA %d\n", lba);
					fseek(sdcard_file, lba * 512, SEEK_SET);
					int bytes_read = fread(&read_block_response[2], 1, 512, sdcard_file);
					if (bytes_read != 512) {
						printf("Warning: short read!\n");
					}
					response = read_block_response;
					response_length = 2 + 512 + 2;
					break;
				}
				case 0x40 + 18: { // CMD18 - read multi block
					uint32_t lba =
					cmd[1] << 24 |
					cmd[2] << 16 |
					cmd[3] << 8 |
					cmd[4];
					int r = fseek(sdcard_file, lba * 512, SEEK_SET);//do just the seek here, the fread is done above
					mblock = 0;
					break;
				}
				case 0x40 + 55: {
					static const uint8_t r[] = { 1 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 58: {
					static const uint8_t r[] = { 0, 0, 0, 0 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				default: {
					static const uint8_t r[] = { 0 };
					response = r;
					response_length = sizeof(r);
					break;
				}
			}
			response_counter = 0;
		}
		outbyte = 0xff;
	}

#ifdef TRACE
	printf("$%02x ->$%02x \n", inbyte, outbyte);
#endif

	return outbyte;
}
