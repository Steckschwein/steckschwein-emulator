// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _GLUE_H_
#define _GLUE_H_

#include <uart.h>
#include <stdint.h>
#include <stdbool.h>


#define NANOSECONDS_PER_SECOND 1000000000LL

#define WARN(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG_ENABLED
	#define DEBUG(...) printf(__VA_ARGS__)
#else
	#define DEBUG(...)
#endif

#define EMU_FREQUENCY 3579545
// #define EMU_FREQUENCY 8000000

extern bool isDebuggerEnabled;

void emulatorStart(const char *stateName);

typedef enum {
	ECHO_MODE_NONE,
	ECHO_MODE_RAW,
	ECHO_MODE_COOKED,
} echo_mode_t;

// GIF recorder commands
typedef enum {
	RECORD_GIF_PAUSE,
	RECORD_GIF_SNAP,
	RECORD_GIF_RESUME
} gif_recorder_command_t;

// GIF recorder states
typedef enum {
	RECORD_GIF_DISABLED,
	RECORD_GIF_PAUSED,
	RECORD_GIF_SINGLE,
	RECORD_GIF_ACTIVE
} gif_recorder_state_t;


void actionEmuTogglePause();
void actionEmuStepBack();

extern UartIO* uartIo0x200;
extern UartIO* uartIo0x250;

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t* ram;
extern uint8_t* rom;

extern bool isDebuggerEnabled;
extern bool log_video;
extern bool log_keyboard;
extern echo_mode_t echo_mode;
extern bool save_on_exit;
extern gif_recorder_state_t record_gif;
extern char *gif_path;
extern uint8_t keymap;

extern void machine_dump();
extern void machine_reset();
extern void machine_paste();
extern void init_audio();

#endif
