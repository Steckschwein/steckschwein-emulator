// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "spi.h"
#include "sdcard.h"
#include "via.h"
#include "ds1306.h"

#include <SDL_keysym.h>

// VIA#2
// PB0 SPICLK
// PB1 SS1 SDCARD
// PB2 SS2 KEYBOARD
// PB3 SS3 RTC
// PB4 unassigned
// PB5 SD write protect
// PB6 SD detect
// PB7 MOSI
// CB1 SPICLK (=PB0)
// CB2 MISO

static bool initialized;

void spi_init() {
	initialized = false;
}

#define SPI_DEV_SDCARD	0b1100
#define SPI_DEV_KEYBRD	0b1010
#define SPI_DEV_RTC		0b0110
#define SPI_DESELECT	(SPI_DEV_SDCARD | SPI_DEV_KEYBRD | SPI_DEV_RTC)

static uint8_t last_keycode = 0;
void spi_handle_keyboard(SDLKey key) {
	last_keycode = key;
	printf("spi_handle_keyboard() %x\n", last_keycode);
}

void dispatch_device(uint8_t port) {
	bool clk = port & 1;

	bool is_sdcard = !((port >> 1) & 1);
	bool is_keyboard = !((port >> 2) & 1);
	bool is_rtc = !((port >> 3) & 1);

	bool mosi = port >> 7;

	static bool last_sdcard;
	static bool last_key;
	static bool last_rtc;
	static uint8_t bit_counter = 0;

	if (!last_sdcard && is_sdcard) {
		bit_counter = 0;
		sdcard_select();
	} else if (!last_key && is_keyboard) {
		bit_counter = 0;
//		printf("keyboard\n");
		last_key = 0;
	} else if (!last_rtc && is_rtc) {
		bit_counter = 0;
		spi_rtc_select();
	}
	if (last_rtc && !is_rtc){
		spi_rtc_deselect();
	}

//	if(is_rtc)
//		printf("%d %d %x\n", bit_counter, mosi, port);

//	TODO FIXME ugly
	last_sdcard = is_sdcard;
	last_rtc = is_rtc;
	last_key = is_keyboard;

// For initialization, the client has to pull&release CLK 74 times.
// The SD card should be deselected, because it's not actual
// data transmission (we ignore this).
//	if (!initialized) { // TODO FIXME move to sdcard.c
//		if (clk == 1) {
//			static int init_counter = 0;
//			init_counter++;
//			if (init_counter >= 70) {
//				sdcard_select();
//				initialized = true;
//			}
//		}
//		return;
//	}

// for everything else, a device has to be selected
	if (!is_sdcard && !is_keyboard && !is_rtc) {
		return;
	}

	static bool last_clk = false;
	if (clk == last_clk) {
		return;
	}
	last_clk = clk;
	if (clk == 0) {	// only care about rising clock
		return;
	}

// receive byte
	static uint8_t inbyte, outbyte;
	inbyte <<= 1;
	inbyte |= mosi;
	bit_counter++;

	if (bit_counter != 8) {
		return;
	}

	bit_counter = 0;

	if (is_sdcard) {
		outbyte = sdcard_handle(inbyte);
	} else if (is_keyboard) {
		outbyte = last_keycode;
		last_keycode = 0;
	} else if (is_rtc) {
		outbyte = spi_rtc_handle(inbyte);
	}

	// send byte
	via1_sr_set(outbyte);
}

void spi_step() {
	uint8_t port = via1_pb_get_out();	//PB

	dispatch_device(port);
}
