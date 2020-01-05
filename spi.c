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
#include <SDL_events.h>

// VIA
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

void spi_init() {

}

static uint8_t last_keycode = 0;

uint8_t spi_handle_keyboard(){
	uint8_t outbyte = last_keycode;
	last_keycode = 0;
	return outbyte;
}

void spi_handle_keyevent(SDL_KeyboardEvent* keyBrdEvent) {

	static bool shift = false;

	bool is_up = keyBrdEvent->type == SDL_KEYUP;

	switch(keyBrdEvent->keysym.sym){
		case SDLK_LCTRL:
		case SDLK_RCTRL:

		break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			shift = !is_up;
		break;
		case SDLK_LALT:
		case SDLK_RALT:
		break;
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
		case SDLK_F7:
		case SDLK_F8:
		case SDLK_F9:
		case SDLK_F10:
		case SDLK_F11:
		case SDLK_F12:
			if(is_up)
				last_keycode = 0xf1 + (keyBrdEvent->keysym.sym - SDLK_F1);
			break;
		case SDLK_RIGHT:
		case SDLK_LEFT:
			if(is_up)
				last_keycode = 0x10 + (keyBrdEvent->keysym.sym - SDLK_RIGHT);
			break;
		case SDLK_UP:
		case SDLK_DOWN:
			if(is_up)
				last_keycode = 0x1e + (keyBrdEvent->keysym.sym - SDLK_UP);
			break;
		default:
			if(is_up){
				last_keycode = (shift ? toupper(keyBrdEvent->keysym.sym) : keyBrdEvent->keysym.sym);
				printf("spi_handle_keyboard() %x\n", last_keycode);
			}
	}
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
		spi_sdcard_select();
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

//	TODO FIXME ugly
	last_sdcard = is_sdcard;
	last_rtc = is_rtc;
	last_key = is_keyboard;

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
		outbyte = spi_sdcard_handle(inbyte);
	} else if (is_keyboard) {
		outbyte = spi_handle_keyboard();
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
