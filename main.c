// Steckschwein Emulator based on Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include <errno.h>

#include <SDL.h>
#include "cpu/fake6502.h"
#include "disasm.h"
#include "memory.h"
#include "via.h"
#include "uart.h"
#include "spi.h"
#include "sdcard.h"
#include "ds1306.h"
#include "glue.h"
#include "joystick.h"
#include "Board.h"
#include "Machine.h"
#include "Emulator.h"
#include "Debugger.h"
#include "EmulatorDebugger.h"
#include "Properties.h"
#include "Actions.h"
#include "ArchSound.h"
#include "ArchNotifications.h"
#include "ArchEvent.h"
#include "ArchThread.h"
#include "ArchTimer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <pthread.h>
#endif

void* emulator_loop(void *param);
void emscripten_main_loop(void);

// This must match the KERNAL's set!
char *keymaps[] = { "en-us", "en-gb", "de", "nordic", "it", "pl", "hu", "es", "fr", "de-ch", "fr-be", "pt-br", };

bool isDebuggerEnabled = false;
char *paste_text = NULL;
char paste_text_data[65536];
bool pasting_bas = false;

#define RAM_SIZE (64*1024)
#define ROM_SIZE (32*1024)

uint32_t ram_size = RAM_SIZE; // 64 KB default

extern int errno;

bool log_video = false;
bool log_speed = false;
bool log_keyboard = false;
bool dump_cpu = false;
bool dump_ram = true;
bool dump_bank = true;
bool dump_vram = false;
echo_mode_t echo_mode;
bool save_on_exit = true;
gif_recorder_state_t record_gif = RECORD_GIF_DISABLED;
char *gif_path = NULL;
uint8_t keymap = 0; // KERNAL's default
char window_title[30];
int32_t last_perf_update = 0;
int32_t perf_frame_count = 0;

#ifdef TRACE
bool trace_mode = false;
uint16_t trace_address = 0;
#endif

static Mixer *mixer;

extern unsigned char *prg_path; //extern - also used in uart

bool checkUploadLmf = false; //check lmf of the upload file, if not changed no recurring uploads
bool run_after_load = false;

#ifdef TRACE
#include "rom_labels.h"
char*
label_for_address(uint16_t address) {
	for (int i = 0; i < sizeof(addresses) / sizeof(*addresses); i++) {
		if (address == addresses[i]) {
			return labels[i];
		}
	}
	return NULL;
}
#endif

static Video *video;
static Properties *properties;

static int enableSynchronousUpdate = 1;
static int emuMaxSpeed = 0;
static int emuMaxEmuSpeed = 0; // Max speed issued by emulation
static int emuPlayReverse = 0;

static volatile int emuSuspendFlag;
static volatile EmuState emuState = EMU_STOPPED;
static volatile int emuSingleStep = 0;

#ifndef WII
static void *emuSyncEvent = NULL;
#endif
static void *emuStartEvent = NULL;
#ifndef WII
static void *emuTimer;
#endif
static void *emuThread;

static int emuExitFlag;
static UInt32 emuSysTime = 0;
static UInt32 emuFrequency;

static int emulationStartFailure = 0;
static int pendingDisplayEvents = 0;
static void *dpyUpdateAckEvent = NULL;

static SDL_Surface *surface;
static int bitDepth;
static int zoom = 1;
static char *displayData[2] = { NULL, NULL };
static int curDisplayData = 0;
static int displayPitch = 0;

static UInt32 emuTimeIdle = 0;
static UInt32 emuTimeTotal = 1;
static UInt32 emuTimeOverflow = 0;
static UInt32 emuUsageCurrent = 0;

static int doQuit = 0;

//int screenWidth=240;//320;
//int screenHeight=320;//240;
int screenWidth = 320;
int screenHeight = 240;

#define EVENT_UPDATE_DISPLAY 2
#define EVENT_UPDATE_WINDOW  3

void machine_dump() {
	int index = 0;
	char filename[22];
	for (;;) {
		if (!index) {
			strcpy(filename, "dump.bin");
		} else {
			sprintf(filename, "dump-%i.bin", index);
		}
		if (access(filename, F_OK) == -1) {
			break;
		}
		index++;
	}
	FILE *f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Cannot write to %s!\n", filename);
		return;
	}

	if (dump_cpu) {
		fwrite(&a, sizeof(uint8_t), 1, f);
		fwrite(&x, sizeof(uint8_t), 1, f);
		fwrite(&y, sizeof(uint8_t), 1, f);
		fwrite(&sp, sizeof(uint8_t), 1, f);
		fwrite(&status, sizeof(uint8_t), 1, f);
		fwrite(&pc, sizeof(uint16_t), 1, f);
	}
	memory_save(f, dump_ram, dump_bank);

	if (dump_vram) {
		//video_save(f);
	}

	fclose(f);
	printf("Dumped system to %s.\n", filename);
}

void machine_reset(int prg_override_start) {
	spi_rtc_init();
	spi_init();
	uart_init(prg_path, prg_override_start, checkUploadLmf);
	via1_init();
	reset6502();
}

void machine_paste(char *s) {
	if (s) {
		paste_text = s;
		pasting_bas = true;
	}
}

static void usage() {
	printf("\nSteckschwein Emulator (C)2019 Michael Steil, Thomas Woinke, Marko Lauke\n");
	printf("All rights reserved. License: 2-clause BSD\n\n");
	printf("Usage: steckschwein-emu [option] ...\n\n");

	/*
	 printf("-prg <app.prg>[,<load_addr>]\n");
	 printf("\tLoad application from the local disk into RAM\n");
	 printf("\t(.PRG file with 2 byte start address header)\n");
	 printf("\tThe override load address is hex without a prefix.\n");
	 */
	printf("-bas <app.txt>\n");
	printf("\tInject a BASIC program in ASCII encoding through the\n");
	printf("\tkeyboard.\n");
	/*
	 printf("-run\n");
	 printf("\tStart the -prg/-bas program using RUN or SYS, depending\n");
	 printf("\ton the load address.\n");
	 */
	printf("-debug [<address>]\n");
	printf("\tEnable debugger. Optionally, set a breakpoint\n");
	printf("-dump {C|R|B|V}...\n");
	printf("\tConfigure system dump: (C)PU, (R)AM, (B)anked-RAM, (V)RAM\n");
	printf("\tMultiple characters are possible, e.g. -dump CV ; Default: RB\n");
	printf("-echo [{iso|raw}]\n");
	printf("\tPrint all KERNAL output to the host's stdout.\n");
	printf("\tBy default, everything but printable ASCII characters get\n");
	printf("\tescaped. \"iso\" will escape everything but non-printable\n");
	printf("\tISO-8859-1 characters and convert the output to UTF-8.\n");
	printf("\t\"raw\" will not do any substitutions.\n");
	printf("\tWith the BASIC statement \"LIST\", this can be used\n");
	printf("\tto detokenize a BASIC program.\n");
	printf("-gif <file.gif>[,wait]\n");
	printf("\tRecord a gif for the video output.\n");
	printf("\tUse ,wait to start paused.\n");
	printf("\tPOKE $9FB5,2 to start recording.\n");
	printf("\tPOKE $9FB5,1 to capture a single frame.\n");
	printf("\tPOKE $9FB5,0 to pause.\n");
	printf("-joy1 {NES | SNES}\n");
	printf("\tChoose what type of joystick to use, e.g. -joy1 SNES\n");
	printf("-joy2 {NES | SNES}\n");
	printf("\tChoose what type of joystick to use, e.g. -joy2 SNES\n");
	printf("-keymap <keymap>\n");
	printf("\tEnable a specific keyboard layout decode table.\n");
	printf("-lmf\n");
  printf("\tcheck last modified of file to upload\n");
	printf("-log {K|S|V}...\n");
	printf("\tEnable logging of (K)eyboard, (S)peed, (V)ideo.\n");
	printf("\tMultiple characters are possible, e.g. -log KS\n");
	printf("-quality {linear (default) | best}\n");
	printf("\tScaling algorithm quality\n");
	printf("-ram <ramsize>\n");
	printf("\tSpecify RAM size in KB (8, 16, 32, ..., 64).\n");
	printf("\tThe default is 64.\n");
	printf("-rom <rom.bin>[,<load_addr>]\n");
	printf("\tbios ROM file.\n");
	printf("-rotate\n");
	printf("\trotate screen 90 degree clockwise\n");
	printf("-sdcard <sdcard.img>\n");
	printf("\tSpecify SD card image (partition map + FAT32)\n");
	printf("-scale {1|2|full} - use ALT_L+F to toggle fullscreen\n");
	printf("\tScale output to an integer multiple of 640x480\n");
#ifdef TRACE
	printf("-trace [<address>]\n");
	printf("\tPrint instruction trace. Optionally, a trigger address\n");
	printf("\tcan be specified.\n");
#endif
	printf("-upload <file>[,<load_addr>]\n");
	printf("\tEmulate serial upload of <file> \n");
	printf("\t(.PRG file with 2 byte start address header)\n");
	printf("\tThe override load address is hex without a prefix.\n");
	printf("\n");
	exit(1);
}

void usage_keymap() {
	printf("The following keymaps are supported:\n");
	for (int i = 0; i < sizeof(keymaps) / sizeof(*keymaps); i++) {
		printf("\t%s\n", keymaps[i]);
	}
	exit(1);
}

void archUpdateWindow() {
	SDL_Event event;

	event.type = SDL_USEREVENT;
	event.user.code = EVENT_UPDATE_WINDOW;
	event.user.data1 = NULL;
	event.user.data2 = NULL;
	SDL_PushEvent(&event);
}

void archVideoOutputChange() {

}

void archEmulationStopNotification() {
	DEBUG("archEmulationStopNotification\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

void archEmulationStartFailure() {
	DEBUG("archEmulationStartFailure\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

int archUpdateEmuDisplay(int syncMode) {
	if (pendingDisplayEvents > 1) {
		return 1;
	}

	pendingDisplayEvents++;

	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = EVENT_UPDATE_DISPLAY;
	event.user.data1 = NULL;
	event.user.data2 = NULL;
	SDL_PushEvent(&event);

#ifndef SINGLE_THREADED
	if (properties->emulation.syncMethod == P_EMU_SYNCFRAMES) {
		archEventWait(dpyUpdateAckEvent, 500);
	}
#endif
	return 1;
}

int WaitReverse() {
//	boardEnableSnapshots(0);

	for (;;) {
		UInt32 sysTime = archGetSystemUpTime(1000);
		UInt32 diffTime = sysTime - emuSysTime;
		if (diffTime >= 50) {
			emuSysTime = sysTime;
			break;
		}
		archEventWait(emuSyncEvent, -1);
	}
	boardRewind();

	return -60;
}

int updateEmuDisplay(int updateAll) {

	int bytesPerPixel = bitDepth / 8;

	char *dpyData = displayData[curDisplayData];
	int width = zoom * screenWidth;
	int height = zoom * screenHeight;

	FrameBuffer *frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
	if (frameBuffer == NULL) {
		frameBuffer = frameBufferGetWhiteNoiseFrame();
	}

	int borderOffset =
			screenWidth > screenHeight ?
					(screenWidth - frameBuffer->maxWidth) * zoom / 2 :
					(screenHeight - frameBuffer->lines) * zoom / 2 * screenWidth; //y offset

	videoRender(video, frameBuffer, bitDepth, zoom, dpyData + borderOffset * bytesPerPixel, 0, displayPitch, -1);

	if (borderOffset > 0) {
		int h = height;
		while (h--) {
			memset(dpyData, 0, borderOffset * bytesPerPixel);
//			memset(dpyData + (width - borderOffset) * bytesPerPixel, 0, borderOffset * bytesPerPixel);
//			dpyData += displayPitch;
		}
	}
	DEBUGRenderDisplay(width, height);

	if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) {
		return 0;
	}
	SDL_UpdateRect(surface, 0, 0, width, height);
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}

	return 0;
}

void createSdlSurface(int width, int height, int fullscreen) {

	int flags = SDL_SWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0);

	// try default bpp
	surface = SDL_SetVideoMode(width, height, 0, flags);
	int bytepp = (surface ? surface->format->BytesPerPixel : 0);
	if (bytepp != 2 && bytepp != 4) {
		SDL_FreeSurface(surface);
		surface = NULL;
	}

	if (!surface) {
		bitDepth = 32;
		surface = SDL_SetVideoMode(width, height, bitDepth, flags);
	}
	if (!surface) {
		bitDepth = 16;
		surface = SDL_SetVideoMode(width, height, bitDepth, flags);
	}

	if (surface != NULL) {
		displayData[0] = (char*) surface->pixels;
		curDisplayData = 0;
		displayPitch = surface->pitch;

		DEBUGInitUI(surface);
	}
}

int createSdlWindow() {
	const char *title = "Steckschwein Emulator - blueMSX";
	int fullscreen = properties->video.windowSize == P_VIDEO_SIZEFULLSCREEN;

	if (fullscreen) {
		zoom = properties->video.fullscreen.width / screenWidth;
		bitDepth = properties->video.fullscreen.bitDepth;
	} else {
		if (properties->video.windowSize == P_VIDEO_SIZEX1) {
			zoom = 1;
		} else if (properties->video.windowSize == P_VIDEO_SIZEX2) {
			zoom = 2;
		} else {
			zoom = 4;
		}
		bitDepth = 32;
	}

	createSdlSurface(zoom * screenWidth, zoom * screenHeight, fullscreen);

	// Set the window caption
	SDL_WM_SetCaption(title, NULL);

	return 1;
}

static void handleEvent(SDL_Event *event) {

	if (DEBUGHandleEvent(event)) {
		return; //event was handled
	}

	switch (event->type) {
	case SDL_USEREVENT:
		switch (event->user.code) {
		case EVENT_UPDATE_DISPLAY:
			updateEmuDisplay(0);
			archEventSet(dpyUpdateAckEvent);
			pendingDisplayEvents--;
			break;
		case EVENT_UPDATE_WINDOW:
			if (!createSdlWindow()) {
				exit(0);
			}
			break;
		}
		break;
	case SDL_ACTIVEEVENT:
		if (event->active.state & SDL_APPINPUTFOCUS) {
//            keyboardSetFocus(1, event->active.gain);
		}
		if (event->active.state == SDL_APPMOUSEFOCUS) {
//            sdlMouseSetFocus(event->active.gain);
		}
		break;
	case SDL_KEYDOWN:
//        shortcutCheckown(shortcuts, HOTKEY_TYPE_KEYBOARD, keyboardGetModifiers(), event->key.keysym.sym);
	{
		int keyNum;
		Uint8 *keyBuf = SDL_GetKeyState(&keyNum);
		if (keyBuf != NULL) {
			if (keyBuf[SDLK_LALT]) {
				if (event->key.keysym.sym == SDLK_f) {
					actionFullscreenToggle();
					break;
				} else if (event->key.keysym.sym == SDLK_p) {
					actionEmuTogglePause();
					break;
				} else if (event->key.keysym.sym == SDLK_x) {
					doQuit = 1;
					break;
				}
			}

		}
		spi_handle_keyevent(&event->key);
		break;
	}
	case SDL_KEYUP:
//        shortcutCheckUp(shortcuts, HOTKEY_TYPE_KEYBOARD, keyboardGetModifiers(), event->key.keysym.sym);
		spi_handle_keyevent(&event->key);
		break;
	case SDL_VIDEOEXPOSE:
		updateEmuDisplay(1);
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
//        sdlMouseButton(event->button.button, event->button.state);
		break;
	case SDL_MOUSEMOTION:
//        sdlMouseMove(event->motion.x, event->motion.y);
		break;
	case SDL_JOYAXISMOTION: /**< Joystick axis motion */
		DEBUG("joystick axis event %x b:%x t:%x s:%x\n", event->jaxis.which, event->jaxis.axis, event->jaxis.type,
				event->jaxis.value);
		break;
	case SDL_JOYBALLMOTION: /**< Joystick trackball motion */
		DEBUG("joystick ball event %x b:%x t:%x s:%x\n", event->jball.which, event->jball.ball, event->jball.type,
				event->jball.xrel);
		break;
	case SDL_JOYHATMOTION:
		/**< Joystick hat position change */
		handle_event(event);
		DEBUG("joystick hat event %x b:%x t:%x s:%x\n", event->jhat.which, event->jhat.hat, event->jbutton.type,
				event->jbutton.state);
		break;
	case SDL_JOYBUTTONUP:
		/**< Joystick button released */
	case SDL_JOYBUTTONDOWN: /**< Joystick button pressed */
		DEBUG("joystick button event %x b:%x t:%x s:%x\n", event->jbutton.which, event->jbutton.button,
				event->jbutton.type, event->jbutton.state);
		break;
	default:
		DEBUG("%x \n", event->type);
	}
}

#ifdef SINGLE_THREADED
int archPollEvent() {

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			doQuit = 1;
		} else if (!doQuit) { //empty queue
			handleEvent(&event);
		}
	}
	return doQuit;
}
#endif

static int emuUseSynchronousUpdate() {
	if (properties->emulation.syncMethod == P_EMU_SYNCIGNORE) {
		return properties->emulation.syncMethod;
	}

	if (properties->emulation.speed == 50 && enableSynchronousUpdate && emuMaxSpeed == 0) {
		return properties->emulation.syncMethod;
	}
	return P_EMU_SYNCAUTO;
}

static int WaitForSync(int maxSpeed, int breakpointHit) {
	UInt32 li1;
	UInt32 li2;
	static UInt32 tmp = 0;
	static UInt32 cnt = 0;
	UInt32 sysTime;
	UInt32 diffTime;
	UInt32 syncPeriod;
	static int overflowCount = 0;
	static UInt32 kbdPollCnt = 0;

	if (emuPlayReverse && properties->emulation.reverseEnable) {
		return WaitReverse();
	}

	//boardEnableSnapshots(1);

	emuMaxEmuSpeed = maxSpeed;

	syncPeriod = emulatorGetSyncPeriod();
	li1 = archGetHiresTimer();

	emuSuspendFlag = 1;

	if (emuSingleStep) {
		debuggerNotifyEmulatorPause();
		emuSingleStep = 0;
		emuState = EMU_PAUSED;
	}

	if (breakpointHit) {
		debuggerNotifyEmulatorPause();
		emuState = EMU_PAUSED;
	}

	if (emuState != EMU_RUNNING) {
		archEventSet(emuStartEvent);
		emuSysTime = 0;
	}

#ifdef SINGLE_THREADED
	emuExitFlag |= archPollEvent();
#endif
	DEBUG(stdout, "WaitForSync %x\n", emuExitFlag);

	if (((++kbdPollCnt & 0x03) >> 1) == 0) {
//       archPollInput();
	}

	if (emuUseSynchronousUpdate() == P_EMU_SYNCTOVBLANK) {
		overflowCount += emulatorSyncScreen() ? 0 : 1;
		while ((!emuExitFlag && emuState != EMU_RUNNING) || overflowCount > 0) {
			archEventWait(emuSyncEvent, -1);
#ifdef NO_TIMERS
            while (timerCallback(NULL) == 0) emuExitFlag |= archPollEvent();
#endif
			overflowCount--;
		}
	} else {
		do {
#ifdef NO_TIMERS
            while (timerCallback(NULL) == 0) emuExitFlag |= archPollEvent();
#endif
			archEventWait(emuSyncEvent, -1);
			if (((emuMaxSpeed || emuMaxEmuSpeed) && !emuExitFlag) || overflowCount > 0) {
#ifdef NO_TIMERS
                while (timerCallback(NULL) == 0) emuExitFlag |= archPollEvent();
#endif
				archEventWait(emuSyncEvent, -1);
			}
			overflowCount = 0;
		} while (!emuExitFlag && emuState != EMU_RUNNING);
	}

	emuSuspendFlag = 0;
	li2 = archGetHiresTimer();

	emuTimeIdle += li2 - li1;
	emuTimeTotal += li2 - tmp;
	tmp = li2;

	sysTime = archGetSystemUpTime(1000);
	diffTime = sysTime - emuSysTime;
	emuSysTime = sysTime;

	if (emuSingleStep) {
		diffTime = 0;
	}

	if ((++cnt & 0x0f) == 0) {
		//emuCalcCpuUsage(NULL);
	}

	overflowCount = emulatorGetCpuOverflow() ? 1 : 0;
#ifdef NO_HIRES_TIMERS
	if (diffTime > 50U) {
		overflowCount = 1;
		diffTime = 0;
	}
#else
	if (diffTime > 100U) {
		overflowCount = 1;
		diffTime = 0;
	}
#endif
	if (emuMaxSpeed || emuMaxEmuSpeed) {
		diffTime *= 10;
		if (diffTime > 20 * syncPeriod) {
			diffTime = 20 * syncPeriod;
		}
	}

	emuUsageCurrent += diffTime;

	return emuExitFlag ? -99 : diffTime;
}

int emulatorGetCpuOverflow() {
	int overflow = emuTimeOverflow;
	emuTimeOverflow = 0;
	return overflow;
}

void emulatorSuspend() {
	if (emuState == EMU_RUNNING) {
		emuState = EMU_SUSPENDED;
		do {
			archThreadSleep(10);
		} while (!emuSuspendFlag);
		archSoundSuspend();
//        archMidiEnable(0);
	}
}

void emulatorSetFrequency(int logFrequency, int *frequency) {
	emuFrequency = (int) (EMU_FREQUENCY * pow(2.0, (logFrequency - 50) / 15.0515));

	if (frequency != NULL) {
		*frequency = emuFrequency;
	}

	boardSetFrequency(emuFrequency);
}

static void emulatorThread() {
	int frequency;
	int success = 1;
	int reversePeriod = 0;
	int reverseBufferCnt = 0;

	emulatorSetFrequency(properties->emulation.speed, &frequency);

	if (properties->emulation.reverseEnable && properties->emulation.reverseMaxTime > 0) {
		reversePeriod = 50;
		reverseBufferCnt = properties->emulation.reverseMaxTime * 1000 / reversePeriod;
	}

  Machine* machine = machineCreate(properties->emulation.machineName);
  if (machine == NULL) {
    // archShowStartEmuFailDialog();
    archEmulationStopNotification();
    emuState = EMU_STOPPED;
    archEmulationStartFailure();
    return;
  }

  boardSetMachine(machine);

	success = boardRun(machine, mixer, frequency, reversePeriod, reverseBufferCnt, WaitForSync);

	//the emu loop
	//ledSetAll(0);
	emuState = EMU_STOPPED;

#ifndef WII
	archTimerDestroy(emuTimer);
#endif

	if (!success) {
		emulationStartFailure = 1;
	}

	archEventSet(emuStartEvent);
}

int emulatorGetSyncPeriod() {
#ifdef NO_HIRES_TIMERS
	return 10;
#else
	return properties->emulation.syncMethod == P_EMU_SYNCAUTO
			|| properties->emulation.syncMethod == P_EMU_SYNCNONE ? 2 : 1;
#endif
}

void RefreshScreen(int screenMode) {

//    lastScreenMode = screenMode;
	if (emuUseSynchronousUpdate() == P_EMU_SYNCFRAMES && emuState == EMU_RUNNING) {
		emulatorSyncScreen();
	}
}

int timerCallback(void *timer) {

	if (properties == NULL) {
		return 1;
	} else {
		static UInt32 frameCount = 0;
		static UInt32 oldSysTime = 0;
		static UInt32 refreshRate = 50;
		UInt32 framePeriod = (properties->video.frameSkip + 1) * 1000;
		UInt32 syncPeriod = emulatorGetSyncPeriod();
		UInt32 sysTime = archGetSystemUpTime(1000);
		UInt32 diffTime = sysTime - oldSysTime;
		int syncMethod = emuUseSynchronousUpdate();

		if (diffTime == 0) {
			return 0;
		}
		oldSysTime = sysTime;

		// Update display
		frameCount += refreshRate * diffTime;
		if (frameCount >= framePeriod) {
			frameCount %= framePeriod;
			if (emuState == EMU_RUNNING) {
				refreshRate = boardGetRefreshRate();

				if (syncMethod == P_EMU_SYNCAUTO || syncMethod == P_EMU_SYNCNONE) {
					archUpdateEmuDisplay(0);
				}
			}
		}

		if (syncMethod == P_EMU_SYNCTOVBLANKASYNC) {
			archUpdateEmuDisplay(syncMethod);
		}
		// Update emulation
		archEventSet(emuSyncEvent);
	}

	return 1;
}

EmuState emulatorGetState() {
	return emuState;
}

int isEmuSingleStep() {
	return emuSingleStep;
}

void emulatorResume() {
	if (emuState == EMU_SUSPENDED) {
		emuSysTime = 0;

		emuState = EMU_RUNNING;
		archUpdateEmuDisplay(0);
	}
}

void emulatorSetState(EmuState state) {
	if (state == EMU_RUNNING) {
		archSoundResume();
	} else {
		archSoundSuspend();
	}
	if (state == EMU_STEP) {
		state = EMU_RUNNING;
		emuSingleStep = 1;
	}
	if (state == EMU_STEP_BACK) {
		EmuState oldState = state;
		state = EMU_RUNNING;
		if (!boardRewindOne()) {
			state = oldState;
		}
	}
	emuState = state;
}

void emulatorStart(const char *stateName) {

	dbgEnable();

//	archEmulationStartNotification();

	emulatorResume();

	emuExitFlag = 0;

	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MOONSOUND, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_YAMAHA_SFG, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MSXAUDIO, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MSXMUSIC, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_SCC, 1);

	properties->emulation.pauseSwitch = 0;
//	switchSetPause(properties->emulation.pauseSwitch);

#ifndef NO_TIMERS
#ifndef WII
	emuSyncEvent = archEventCreate(0);
#endif
	emuStartEvent = archEventCreate(0);
#ifndef WII
	emuTimer = archCreateTimer(emulatorGetSyncPeriod(), timerCallback);
#endif
#endif
	//setDeviceInfo(&deviceInfo);

//    inputEventReset();

	archSoundResume();
//    archMidiEnable(1);

	emuState = EMU_PAUSED;
	emulationStartFailure = 0;
	//strcpy(emuStateName, stateName ? stateName : "");

	//clearlog();

#if SINGLE_THREADED
	emuState = EMU_RUNNING;
	emulatorThread();

	if (emulationStartFailure) {
		archEmulationStopNotification();
		emuState = EMU_STOPPED;
		archEmulationStartFailure();
	}
#else
	emuThread = archThreadCreate(emulatorThread, THREAD_PRIO_HIGH);

	archEventWait(emuStartEvent, 1000);

	if (emulationStartFailure) {
		archEmulationStopNotification();
		emuState = EMU_STOPPED;
		archEmulationStartFailure();
	}
	if (emuState != EMU_STOPPED) {
		/*
		 getDeviceInfo(&deviceInfo);

		 boardSetYm2413Oversampling(properties->sound.chip.ym2413Oversampling);
		 boardSetY8950Oversampling(properties->sound.chip.y8950Oversampling);
		 boardSetMoonsoundOversampling(properties->sound.chip.moonsoundOversampling);

		 strcpy(properties->emulation.machineName, machine->name);
		 */
		debuggerNotifyEmulatorStart();

		emuState = EMU_RUNNING;
	}
#endif
}

void emulatorStop() {
	if (emuState == EMU_STOPPED) {
		return;
	}
	debuggerNotifyEmulatorStop();

	emuState = EMU_STOPPED;

	do {
		archThreadSleep(10);
	} while (!emuSuspendFlag);

	emuExitFlag = 1;

	archSoundSuspend();
	archThreadJoin(emuThread, 3000);
	archThreadDestroy(emuThread);
//    archMidiEnable(0);
//    machineDestroy(machine);
#ifndef WII
	archEventDestroy(emuSyncEvent);
#endif

	archEventDestroy(emuStartEvent);

	// Reset active indicators in mixer
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MOONSOUND, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_YAMAHA_SFG, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MSXAUDIO, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_MSXMUSIC, 1);
	mixerIsChannelTypeActive(mixer, MIXER_CHANNEL_SCC, 1);

	archEmulationStopNotification();

	dbgDisable();dbgPrint();
//    savelog();
}

static int emuFrameskipCounter = 0;

int emulatorSyncScreen() {
	int rv = 0;
	emuFrameskipCounter--;
	if (emuFrameskipCounter < 0) {
		rv = archUpdateEmuDisplay(properties->emulation.syncMethod);
		if (rv) {
			emuFrameskipCounter = properties->video.frameSkip;
		}
	}
	return rv;
}

void emulatorRestartSound() {
	emulatorSuspend();
	archSoundDestroy();
	archSoundCreate(mixer, 44100, properties->sound.bufSize, properties->sound.stereo ? 2 : 1);
	emulatorResume();
}

void trace() {
#ifdef TRACE
	if (pc == trace_address && trace_address != 0) {
		trace_mode = true;
	}
	if (trace_mode) {
		DEBUG ("\t\t\t\t[%6d] ", mos6502instructions());

		char *label = label_for_address(pc);
		int label_len = label ? strlen(label) : 0;
		if (label) {
			DEBUG ("%s", label);
		}
		for (int i = 0; i < 10 - label_len; i++) {
			DEBUG (" ");
		}
		DEBUG (" .,%04x ", pc);
		char disasm_line[15];
		int len = disasm(pc, RAM, disasm_line, sizeof(disasm_line), false, 0);
		for (int i = 0; i < len; i++) {
			DEBUG ("%02x ", read6502(pc + i));
		}
		for (int i = 0; i < 9 - 3 * len; i++) {
			DEBUG (" ");
		}
		DEBUG ("%s", disasm_line);
		for (int i = 0; i < 15 - strlen(disasm_line); i++) {
			DEBUG (" ");
		}

		DEBUG ("a=$%02x x=$%02x y=$%02x s=$%02x p=", a, x, y, sp);
		for (int i = 7; i >= 0; i--) {
			DEBUG ("%c", (status & (1 << i)) ? "czidb.vn"[i] : '-');
		}
//			DEBUG (" --- %04x", RAM[0xae]  | RAM[0xaf]  << 8);
		DEBUG ("\n");
	}
#endif
}

void hookCharOut() {
	if (echo_mode != ECHO_MODE_NONE && (pc == 0xfff7 || pc == 0xffb3)) {
		//@see jumptables, bios fff0, kernel $ffb3
		uint8_t c = a;
		if (echo_mode == ECHO_MODE_COOKED) {
			if (c == 0x0d) {
				printf("\n");
			} else if (c == 0x0a) {
				// skip
			} else if (c < 0x20 || c >= 0x80) {
				printf("\\X%02X", c);
			} else {
				printf("%c", c);
			}
		} else {
			printf("%c", c);
		}
		fflush(stdout);
	}
}

void hookKernelPrgLoad(FILE *prg_file, int prg_override_start) {
	if (prg_file) {
		if (pc == 0xff00) {
			// ...inject the app into RAM
			uint8_t start_lo = fgetc(prg_file);
			uint8_t start_hi = fgetc(prg_file);
			uint16_t start;
			if (prg_override_start >= 0) {
				start = prg_override_start;
			} else {
				start = start_hi << 8 | start_lo;
			}
			if (start >= 0xe000) {
				fprintf(stderr, "invalid program start address %x, will override kernel!\n", start);
			} else {
				uint16_t end = start + fread(ram + start, 1, RAM_SIZE - start, prg_file);
			}
			fclose(prg_file);
			prg_file = NULL;
		}
	}
}

//called after each 6502 instruction
void instructionCb(uint32_t cycles) {

	for (uint8_t i = 0; i < cycles; i++) {
		spi_step();
		joystick_step();
	}

	trace();

	if (!isDebuggerEnabled) {
		hookCharOut();
	}

//	hookKernelPrgLoad(prg_file, prg_override_start);

	if (pc == 0xffff) {
		if (save_on_exit) {
			machine_dump();
			doQuit = 1;
		}
	}
}

int nextArg(int *argc, char ***argv, char *arg) {
	int n = argc && !strncmp(*argv[0], arg, strlen(*argv[0]));
	if (n) {
		(*argc)--;
		(*argv)++;
	}
	return n;
}

void assertParam(int argc, char **argv) {
	if (!argc || argv[0][0] == '-') {
		usage();
	}
}

bool parseBoolean(unsigned char *s) {
	bool b = false;
	char *comma = strchr(s, ',');
	if (comma) {
		b = !strcasecmp(comma + 1, "true");
		if (b) {
			*comma = 0;
		}
	}
	return false;
}

int parseNumber(unsigned char *s) {
	char *comma = strchr(s, ',');
	if (comma) {
		int a = (uint16_t) strtol(comma + 1, NULL, 16);
		if (errno != EINVAL) {
			*comma = 0;	//terminate string
			return a;
		}
	}
	return -1;
}

int main(int argc, char **argv) {
	char rom_path_data[PATH_MAX];
	char *rom_path = rom_path_data;
	char *bas_path = NULL;
	char *sdcard_path = NULL;

	run_after_load = false;

#if defined(WIN32)
	freopen("CON", "w", stdout); //http://sdl.beuc.net/sdl.wiki/FAQ_Console
	freopen("CON", "w", stderr);
#endif

	rom_path = getcwd(rom_path_data, PATH_MAX);
	if (rom_path == NULL) {
		fprintf(stderr, "could not determine current work directory\n");
		return 1;
	}
	// This causes the emulator to load ROM data from the executable's directory when
	// no ROM file is specified on the command line.
	strncpy(rom_path + strlen(rom_path), "/rom.bin", PATH_MAX - strlen(rom_path));

	//read default properties
//	properties = propCreate(0, 0, P_EMU_SYNCTOVBLANK, "Steckschwein");
//	properties->emulation.vdpSyncMode = P_VDP_SYNCAUTO;
	properties = propCreate(0, 0, P_EMU_SYNCNONE, "Steckschwein");
	properties->emulation.vdpSyncMode = P_VDP_SYNCAUTO;

	argc--;
	argv++;

	while (argc > 0) {
		if (nextArg(&argc, &argv, "-rom")) {
			assertParam(argc, argv);
			rom_path = argv[0];
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-ram")) {
			assertParam(argc, argv);
			int kb = atoi(argv[0]);
			bool found = false;
			for (int cmp = 8; cmp <= 512; cmp <<= 2) {
				if (kb == cmp) {
					found = true;
					break;
				}
			}
			if (!found) {
				usage();
			}
			ram_size = kb / 8;
		} else if (nextArg(&argc, &argv, "-keymap")) {
			if (!argc || argv[0][0] == '-') {
				usage_keymap();
			}
			bool found = false;
			for (int i = 0; i < sizeof(keymaps) / sizeof(*keymaps); i++) {
				if (!strcmp(argv[0], keymaps[i])) {
					found = true;
					keymap = i;
					break;
				}
			}
			if (!found) {
				usage_keymap();
			}
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-upload")) {
			assertParam(argc, argv);
			prg_path = argv[0];
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-lmf")) {
			checkUploadLmf = true;
		} /*else if (!strcmp(argv[0], "-run")) {
		 run_after_load = true;
		 } */else if (nextArg(&argc, &argv, "-bas")) {
			assertParam(argc, argv);
			bas_path = argv[0];
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-sdcard")) {
			assertParam(argc, argv);
			sdcard_path = argv[0];
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-echo")) {
			if (argc && argv[0][0] != '-') {
				if (!strcmp(argv[0], "raw")) {
					echo_mode = ECHO_MODE_RAW;
				} else {
					usage();
				}
				argc--;
				argv++;
			} else {
				echo_mode = ECHO_MODE_COOKED;
			}
		} else if (nextArg(&argc, &argv, "-log")) {
			assertParam(argc, argv);
			for (char *p = argv[0]; *p; p++) {
				switch (tolower(*p)) {
				case 'k':
					log_keyboard = true;
					break;
				case 's':
					log_speed = true;
					break;
				case 'v':
					log_video = true;
					break;
				default:
					usage();
				}
			}
		} else if (nextArg(&argc, &argv, "-dump")) {
			assertParam(argc, argv);
			dump_cpu = false;
			dump_ram = false;
			dump_bank = false;
			dump_vram = false;
			for (char *p = argv[0]; *p; p++) {
				switch (tolower(*p)) {
				case 'c':
					dump_cpu = true;
					break;
				case 'r':
					dump_ram = true;
					break;
				case 'b':
					dump_bank = true;
					break;
				case 'v':
					dump_vram = true;
					break;
				default:
					usage();
				}
			}
			argc--;
			argv++;
		} else if (nextArg(&argc, &argv, "-gif")) {
			assertParam(argc, argv);
			// set up for recording
			record_gif = RECORD_GIF_PAUSED;
			gif_path = argv[0];
			argv++;
			argc--;
		} else if (nextArg(&argc, &argv, "-debug")) {
			isDebuggerEnabled = true;
			if (argc && argv[0][0] != '-') {
				DEBUGSetBreakPoint((uint16_t) strtol(argv[0], NULL, 16));
				argc--;
				argv++;
			}
		} else if (nextArg(&argc, &argv, "-joy1")) {
			if (nextArg(&argc, &argv, "NES")) {
				joy1_mode = NES;
			} else if (nextArg(&argc, &argv, "SNES")) {
				joy1_mode = SNES;
			}else{
				usage();
			}
		} else if (nextArg(&argc, &argv, "-joy2")) {
			if (nextArg(&argc, &argv, "NES")) {
				joy2_mode = NES;
			} else if (nextArg(&argc, &argv, "SNES")) {
				joy2_mode = SNES;
			}else{
				usage();
			}
#ifdef TRACE
		} else if (nextArg(&argc, &argv, "-trace")) {
			if (argc && argv[0][0] != '-') {
				trace_mode = false;
				trace_address = (uint16_t) strtol(argv[0], NULL, 16);
				argc--;
				argv++;
			} else {
				trace_mode = true;
				trace_address = 0;
			}
#endif
		} else if (nextArg(&argc, &argv, "-rotate")) {
			int t = screenWidth;
			screenWidth = screenHeight;
			screenHeight = t;
			properties->video.rotate = 1;
		} else if (nextArg(&argc, &argv, "-scale")) {
			if (nextArg(&argc, &argv, "1")) {
				properties->video.windowSize = P_VIDEO_SIZEX1;
			} else if (nextArg(&argc, &argv, "2")) {
				properties->video.windowSize = P_VIDEO_SIZEX2;
			} else if (nextArg(&argc, &argv, "full")) {
				properties->video.windowSize = P_VIDEO_SIZEFULLSCREEN;
			} else {
				usage();
			}
		} else if (nextArg(&argc, &argv, "-quality")) {
			if (nextArg(&argc, &argv, "best")) {
				properties->video.monitorType = P_VIDEO_PALHQ2X;
			} else if (nextArg(&argc, &argv, "linear")) {
				properties->video.monitorType = P_VIDEO_PALMON;
			} else {
				usage();
			}
		} else {
			usage();
		}
	}

	memory_init();
	mixer = mixerCreate();

	int rom_override_start = parseNumber(rom_path);
	FILE *f = fopen(rom_path, "rb");
	if (!f) {
		fprintf(stderr, "Cannot open %s!\n", rom_path);
		exit(1);
	}
	if (rom_override_start == -1) {
		int rom_size = fread(rom, 1, ROM_SIZE, f);
	} else {
		int size = fread(ram + rom_override_start, 1, RAM_SIZE - rom_override_start, f);
		write6502(0x230, 1);	//rom off
	}
	fclose(f);

	if (sdcard_path) {
		sdcard_file = fopen(sdcard_path, "r+b");
		if (!sdcard_file) {
			fprintf(stderr, "Cannot open %s!\n", sdcard_path);
			exit(1);
		}
	}

	int prg_override_start = -1;
	if (prg_path) {
		prg_override_start = parseNumber(prg_path);
	}

	if (bas_path) {
		FILE *bas_file = fopen(bas_path, "r");
		if (!bas_file) {
			fprintf(stderr, "Cannot open %s!\n", bas_path);
			exit(1);
		}
		paste_text = paste_text_data;
		size_t paste_size = fread(paste_text, 1, sizeof(paste_text_data) - 1, bas_file);
		if (run_after_load) {
			strncpy(paste_text + paste_size, "\rRUN\r", sizeof(paste_text_data) - paste_size);
		} else {
			paste_text[paste_size] = 0;
		}
		fclose(bas_file);
	}

	//register cpu hook
	hookexternal(instructionCb);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		return 1;
	}
	SDL_ShowCursor(SDL_DISABLE);
	if (SDL_EnableKeyRepeat(250, 50) < 0) {
		return 1;
	}

	video = videoCreate();
	videoSetColors(video, properties->video.saturation, properties->video.brightness, properties->video.contrast,
			properties->video.gamma);
	videoSetScanLines(video, properties->video.scanlinesEnable, properties->video.scanlinesPct);
	videoSetColorSaturation(video, properties->video.colorSaturationEnable, properties->video.colorSaturationWidth);

	bitDepth = 32;
	if (!createSdlWindow()) {
		return 0;
	}

	dpyUpdateAckEvent = archEventCreate(0);

//    keyboardInit();

//    emulatorInit(properties, mixer);
	actionInit(video, properties, mixer);
//    langInit();

//    langSetLanguage(properties->language);
//
//    joystickPortSetType(0, properties->joy1.typeId);
//    joystickPortSetType(1, properties->joy2.typeId);
	joystick_init();

//    uartIoSetType(properties->ports.Com.type, properties->ports.Com.fileName);
//    ykIoSetMidiInType(properties->sound.YkIn.type, properties->sound.YkIn.fileName);

	machine_reset(prg_override_start);

	emulatorRestartSound();

	for (int i = 0; i < MIXER_CHANNEL_TYPE_COUNT; i++) {
		mixerSetChannelTypeVolume(mixer, i, properties->sound.mixerChannel[i].volume);
		mixerSetChannelTypePan(mixer, i, properties->sound.mixerChannel[i].pan);
		mixerEnableChannelType(mixer, i, properties->sound.mixerChannel[i].enable);
	}

	mixerSetMasterVolume(mixer, properties->sound.masterVolume);
	mixerEnableMaster(mixer, properties->sound.masterEnable);

	videoUpdateAll(video, properties);

	emulatorStart("Start");
#ifndef SINGLE_THREADED	//on multi-threaded the main thread will loop here
    SDL_Event event;//While the user hasn't quit
    while(!doQuit){
        SDL_WaitEvent(&event);
		if( event.type == SDL_QUIT ) {
			doQuit = 1;
		}
		else {
			handleEvent(&event);
		}
    }
#endif

	// For stop threads before destroy.
	// Clean up.
	if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
		SDL_Quit();
	}
	videoDestroy(video);
	archSoundDestroy();
	mixerDestroy(mixer);
	propDestroy(properties);
	DEBUGFreeUI();
	memory_destroy();

	spi_rtc_destroy();

	return 0;
}

void emscripten_main_loop(void) {
	emulator_loop(NULL);
}

void*
emulator_loop(void *param) {
	while (!doQuit) {
		SDL_Event event;
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			break;
		}

//		if (isDebuggerEnabled) {
//			int dbgCmd = DEBUGGetCurrentStatus();
//			if (dbgCmd > 0)
//				continue;
//			if (dbgCmd < 0)
//				break;
//		}

		uint32_t old_clockticks6502 = clockticks6502;
		step6502();
		uint8_t clocks = clockticks6502 - old_clockticks6502;
		bool new_frame = false;
		for (uint8_t i = 0; i < clocks; i++) {
			spi_step();
//			new_frame |= vdp_step(MHZ);
		}

		if (new_frame) {
			static int frames = 0;
			frames++;
			int32_t sdlTicks = 0;	//SDL_GetTicks();
			int32_t diff_time = 1000 * frames / 60 - sdlTicks;
			if (diff_time > 0) {
				usleep(1000 * diff_time);
			}

			if (sdlTicks - last_perf_update > 5000) {
				int32_t frameCount = frames - perf_frame_count;
				int perf = frameCount / 3;

				if (perf < 100) {
					sprintf(window_title, "Steckschwein (%d%%)", perf);
					//video_update_title(window_title);
				} else {
					//video_update_title("Steckschwein");
				}

				perf_frame_count = frames;
				last_perf_update = sdlTicks;
			}

			if (log_speed) {
				float frames_behind = -((float) diff_time / 16.666666);
				int load = (int) ((1 + frames_behind) * 100);
				printf("Load: %d%%\n", load > 100 ? 100 : load);

				if ((int) frames_behind > 0) {
					printf("Rendering is behind %d frames.\n", -(int) frames_behind);
				}
			}
#ifdef __EMSCRIPTEN__
			// After completing a frame we yield back control to the browser to stay responsive
			return 0;
#endif
		}

#if 0
		if (clockticks6502 >= 5 * MHZ * 1000 * 1000) {
			break;
		}
#endif
	}

	return 0;
}

//#ifdef __EMSCRIPTEN__
//	emscripten_set_main_loop(emscripten_main_loop, 0, 1);
//#else
//	emulator_loop(NULL);
//#endif
