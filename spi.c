// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "Actions.h"
#include "Board.h"
#include "spi.h"
#include "sdcard.h"
#include "via.h"
#include "ds1306.h"

#include "scancodes_de_cp437.h"

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

volatile uint8_t last_keycode = 0;

uint8_t spi_handle_keyboard(uint8_t inbyte) {

	uint8_t outbyte = 0xff;

	static uint8_t cmd = 0;

	if (inbyte != 0 && cmd == 0) {
		switch (inbyte) {
      case 0xff: //KBD_CMD_RESET:
      case 0xf4: //KBD_CMD_SCAN_ON:
      case 0xf5: //KBD_CMD_SCAN_OFF:
      case 0xf3: //KBD_CMD_TYPEMATIC:
      case 0xed: //KBD_CMD_LEDS:
        cmd = inbyte;
        outbyte = 0xfa; //ack KBD_CMD_ACK
        break;
      case 0x01: // KBD_HOST_CMD_KBD_STATUS
      case 0x02: // KBD_HOST_CMD_CMD_STATUS
        outbyte = 0xaa; //cmd eot - TODO emulate command state response - we always send 0xaa to signal end of transmission
        break;
		}
	} else {
		switch (cmd) {
      case 0xf3: //KBD_CMD_TYPEMATIC:
      {
        unsigned int delay = (((inbyte >> 5) & 0x03) + 1) * 250;
        unsigned int interval = 1000 / (30 - (inbyte & 0x1c));

        // if (SDL_EnableKeyRepeat(delay, interval)) {
        //   fprintf(stderr, "could not set keyboard delay/rate!\n");
        //   outbyte = 0xff;
        // } else {
        outbyte = 0xfa;
        // }
        break;
      }
      default: //output captured keycode
        outbyte = last_keycode;
        last_keycode = 0;
        boardClearInt(0x08);
    }
		cmd = 0;
	}
	return outbyte;
}

static uint8_t kbd_index = 0;

void keyboardInit(){
  SDL_PumpEvents();
  kbd_index = (SDL_GetModState() & KMOD_CAPS) == KMOD_CAPS ? kbd_index | 1 : kbd_index & ~(1);
}

void spi_handle_keyevent(SDL_KeyboardEvent *keyBrdEvent) {

	static is_lalt = false;

	bool is_up = (keyBrdEvent->type == SDL_KEYUP);

	SDL_Keycode keyCode = keyBrdEvent->keysym.sym;

	switch (keyCode) {
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      kbd_index = is_up ? (kbd_index & ~(2)) : kbd_index | 2;
      break;
    case SDLK_CAPSLOCK:
      kbd_index = (SDL_GetModState() & KMOD_CAPS) == KMOD_CAPS ? kbd_index | 1 : kbd_index & ~(1);
      break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      kbd_index = is_up ? (kbd_index & ~(1)) : kbd_index | 1;
      break;
    case SDLK_LALT:
      is_lalt = !is_up;
      break;
    case SDLK_RALT:
      kbd_index = is_up ? (kbd_index & ~(4)) : kbd_index | 4;
      break;
    case SDLK_MODE:
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
      if (is_up)
        last_keycode = 0xf1 + (keyCode - SDLK_F1);
      break;
    case SDLK_RIGHT:
      if (!is_up)
        last_keycode = 0x10 + (keyCode - SDLK_RIGHT);
      break;
    case SDLK_LEFT:
      if (!is_up)
        last_keycode = 0x11 + (keyCode - SDLK_LEFT);
      break;
    case SDLK_UP:
      if (!is_up)
        last_keycode = 0x1e + (keyCode - SDLK_UP);
      break;
    case SDLK_DOWN:
      if (!is_up)
        last_keycode = 0x1d + (keyCode - SDLK_DOWN);
      break;
    default:
      if (!is_up) {
        if (is_lalt) {
          switch (keyCode) {
          case SDLK_r:
            actionEmuResetSoft();
            break;
          }
        }

        uint8_t i = (kbd_index >= 4 ? 3 : kbd_index >= 2 ? 2 : kbd_index);
        if (keyCode == SDLK_s && i == 2) {
            boardSetNmi(NMI);
        } else if (keyCode < SCAN_CODES_SIZE && scancodes[keyCode] && scancodes[keyCode][i]) {
            last_keycode = scancodes[keyCode][i];
        } else {
          last_keycode = keyCode; // unmapped
        }
      }else{
        boardClearNmi(NMI);
      }
    }
#ifdef EMU_AVR_KEYBOARD_IRQ
  if (last_keycode != 0) {
    boardSetInt(0x08);
	}
#endif

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
	} else if (!last_rtc && is_rtc) {
		bit_counter = 0;
		spi_rtc_select();
	}else if (last_rtc && !is_rtc) {
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
	if (!clk) {	// only care about rising clock
		return;
	}

// receive byte
	static uint8_t inbyte, outbyte;
	inbyte <<= 1;
	inbyte |= mosi;
	bit_counter++;

	if (is_sdcard) {
		static bool initialized = false;
		// For initialization, the client has to pull&release CLK 74 times.
		// The SD card should be deselected, because it's not actual
		// data transmission (we ignore this).
		if (!initialized) { // TODO FIXME move to sdcard.c
			if (clk) {
				static int init_counter = 0;
				init_counter++;
				if (init_counter >= 74) {
					spi_sdcard_select();
					initialized = true;
				}
			}
			return;
		}
	}

	if (bit_counter != 8) {
		return;
	}

	bit_counter = 0;

	if (is_sdcard) {
		outbyte = spi_sdcard_handle(inbyte);
	} else if (is_keyboard) {
		outbyte = spi_handle_keyboard(inbyte);
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
