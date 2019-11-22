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

#include "cpu/fake6502.h"
#include "disasm.h"
#include "memory.h"
#include "video.h"
#include "via.h"
#include "opl2.h"
#include "uart.h"
#include "spi.h"
#include "sdcard.h"
#include "glue.h"
//#include "debugger.h"
#include "Adapter.h"
#include "Properties.h"

#define AUDIO_SAMPLES 4096
#define SAMPLERATE 22050

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <pthread.h>
#endif

#define MHZ 8

void *emulator_loop(void *param);
void emscripten_main_loop(void);

// This must match the KERNAL's set!
char *keymaps[] = {
	"en-us",
	"en-gb",
	"de",
	"nordic",
	"it",
	"pl",
	"hu",
	"es",
	"fr",
	"de-ch",
	"fr-be",
	"pt-br",
};

bool debugger_enabled = false;
bool headless = false;
char *paste_text = NULL;
char paste_text_data[65536];
bool pasting_bas = false;

uint16_t num_ram_banks = 64; // 512 KB default

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
int window_scale = 1;
char *scale_quality = "best";
char window_title[30];
int32_t last_perf_update = 0;
int32_t perf_frame_count = 0;

#ifdef TRACE
bool trace_mode = false;
uint16_t trace_address = 0;
#endif

FILE *prg_file ;
int prg_override_start = -1;
bool run_after_load = false;

#ifdef TRACE
#include "rom_labels.h"
char *
label_for_address(uint16_t address)
{
	for (int i = 0; i < sizeof(addresses) / sizeof(*addresses); i++) {
		if (address == addresses[i]) {
			return labels[i];
		}
	}
	return NULL;
}
#endif

static Video* video;
static Properties* properties;

static int enableSynchronousUpdate = 1;
static int           emuMaxSpeed = 0;
static int           emuMaxEmuSpeed = 0; // Max speed issued by emulation
static int           emuPlayReverse = 0;

static volatile int      emuSuspendFlag;
static volatile EmuState emuState = EMU_STOPPED;
static volatile int      emuSingleStep = 0;

static void*  emuSyncEvent;
static void*  emuStartEvent;
static void*  emuTimer;
static void*  emuThread;

static int    emuExitFlag;
static UInt32 emuSysTime = 0;
static UInt32 emuFrequency = 3579545;

static int emulationStartFailure = 0;
static int pendingDisplayEvents = 0;
static void* dpyUpdateAckEvent = NULL;

static SDL_Surface *surface;
static int   bitDepth;
static int   zoom = 1;
static char* displayData[2] = { NULL, NULL };
static int   curDisplayData = 0;
static int   displayPitch = 0;

static UInt32 emuTimeIdle       = 0;
static UInt32 emuTimeTotal      = 1;
static UInt32 emuTimeOverflow   = 0;
static UInt32 emuUsageCurrent   = 0;

static int doQuit = 0;

static UInt32 boardFreq = boardFrequency();

#define WIDTH  320
#define HEIGHT 240

#define EVENT_UPDATE_DISPLAY 2
#define EVENT_UPDATE_WINDOW  3


void
machine_dump()
{
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
		printf("Cannot write to %s!\n", filename);
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

void
machine_reset()
{
	spi_init();
	uart_init();
	via1_init();
	opl2_init();
	reset6502();
}

void
machine_paste(char *s)
{
	if (s) {
		paste_text = s;
		pasting_bas = true;
	}
}

static void
usage()
{
	printf("\nSteckschwein Emulator  (C)2019 Michael Steil, Thomas Woinke\n");
	printf("All rights reserved. License: 2-clause BSD\n\n");
	printf("Usage: steckschwein-emu [option] ...\n\n");
	printf("-rom <rom.bin>\n");
	printf("\tOverride KERNAL/BASIC/* ROM file.\n");
	printf("-ram <ramsize>\n");
	printf("\tSpecify banked RAM size in KB (8, 16, 32, ..., 2048).\n");
	printf("\tThe default is 512.\n");
	printf("-keymap <keymap>\n");
	printf("\tEnable a specific keyboard layout decode table.\n");
	printf("-sdcard <sdcard.img>\n");
	printf("\tSpecify SD card image (partition map + FAT32)\n");
	printf("-prg <app.prg>[,<load_addr>]\n");
	printf("\tLoad application from the local disk into RAM\n");
	printf("\t(.PRG file with 2 byte start address header)\n");
	printf("\tThe override load address is hex without a prefix.\n");
	printf("-bas <app.txt>\n");
	printf("\tInject a BASIC program in ASCII encoding through the\n");
	printf("\tkeyboard.\n");
	printf("-run\n");
	printf("\tStart the -prg/-bas program using RUN or SYS, depending\n");
	printf("\ton the load address.\n");
	printf("-echo [{iso|raw}]\n");
	printf("\tPrint all KERNAL output to the host's stdout.\n");
	printf("\tBy default, everything but printable ASCII characters get\n");
	printf("\tescaped. \"iso\" will escape everything but non-printable\n");
	printf("\tISO-8859-1 characters and convert the output to UTF-8.\n");
	printf("\t\"raw\" will not do any substitutions.\n");
	printf("\tWith the BASIC statement \"LIST\", this can be used\n");
	printf("\tto detokenize a BASIC program.\n");
	printf("-log {K|S|V}...\n");
	printf("\tEnable logging of (K)eyboard, (S)peed, (V)ideo.\n");
	printf("\tMultiple characters are possible, e.g. -log KS\n");
	printf("-gif <file.gif>[,wait]\n");
	printf("\tRecord a gif for the video output.\n");
	printf("\tUse ,wait to start paused.\n");
	printf("\tPOKE $9FB5,2 to start recording.\n");
	printf("\tPOKE $9FB5,1 to capture a single frame.\n");
	printf("\tPOKE $9FB5,0 to pause.\n");
	printf("-scale {1|2|3|4}\n");
	printf("\tScale output to an integer multiple of 640x480\n");
	printf("-quality {nearest|linear|best}\n");
	printf("\tScaling algorithm quality\n");
	printf("-debug [<address>]\n");
	printf("\tEnable debugger. Optionally, set a breakpoint\n");
	printf("-dump {C|R|B|V}...\n");
	printf("\tConfigure system dump: (C)PU, (R)AM, (B)anked-RAM, (V)RAM\n");
	printf("\tMultiple characters are possible, e.g. -dump CV ; Default: RB\n");
	printf("-joy1 {NES | SNES}\n");
	printf("\tChoose what type of joystick to use, e.g. -joy1 SNES\n");
	printf("-joy2 {NES | SNES}\n");
	printf("\tChoose what type of joystick to use, e.g. -joy2 SNES\n");
	printf("-headless\n");
	printf("\tdisable video emulation\n");
#ifdef TRACE
	printf("-trace [<address>]\n");
	printf("\tPrint instruction trace. Optionally, a trigger address\n");
	printf("\tcan be specified.\n");
#endif
	printf("\n");
	exit(1);
}

void
usage_keymap()
{
	printf("The following keymaps are supported:\n");
	for (int i = 0; i < sizeof(keymaps)/sizeof(*keymaps); i++) {
		printf("\t%s\n", keymaps[i]);
	}
	exit(1);
}


char* langDbgMemVram()              { return "VRAM"; }
char* langDbgRegs()                 { return "Registers"; }
char* langDbgDevTms9929A()          { return "TMS9929A"; }
char* langDbgDevTms99x8A()          { return "TMS99x8A"; }
char* langDbgDevV9938()             { return "V9938"; }
char* langDbgDevV9958()             { return "V9958"; }

void archVideoOutputChange() {

}

void archEmulationStopNotification(){
	 printf("archEmulationStopNotification\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

void archEmulationStartFailure(){
	 printf("archEmulationStartFailure\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

int archUpdateEmuDisplay(int syncMode){
    SDL_Event event;

    if (pendingDisplayEvents > 1) {
        return 1;
    }

    pendingDisplayEvents++;

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

int WaitReverse()
{
    //boardEnableSnapshots(0);

    for (;;) {
        UInt32 sysTime = archGetSystemUpTime(1000);
        UInt32 diffTime = sysTime - emuSysTime;
        if (diffTime >= 50) {
            emuSysTime = sysTime;
            break;
        }
        archEventWait(emuSyncEvent, -1);
    }

    //boardRewind();

    return -60;
}

int updateEmuDisplay(int updateAll)
{
    FrameBuffer* frameBuffer;
    int bytesPerPixel = bitDepth / 8;
    char* dpyData  = displayData[curDisplayData];
    int width  = zoom * WIDTH;
    int height = zoom * HEIGHT;
    int borderWidth;

    frameBuffer = frameBufferFlipViewFrame(properties->emulation.syncMethod == P_EMU_SYNCTOVBLANKASYNC);
    if (frameBuffer == NULL) {
        frameBuffer = frameBufferGetWhiteNoiseFrame();
    }

    borderWidth = (320 - frameBuffer->maxWidth) * zoom / 2;

    videoRender(video, frameBuffer, bitDepth, zoom,
                dpyData + borderWidth * bytesPerPixel, 0, displayPitch, -1);

    if (borderWidth > 0) {
        int h = height;
        while (h--) {
            memset(dpyData, 0, borderWidth * bytesPerPixel);
            memset(dpyData + (width - borderWidth) * bytesPerPixel, 0, borderWidth * bytesPerPixel);
            dpyData += displayPitch;
        }
    }


    if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) {
        return 0;
    }
    SDL_UpdateRect(surface, 0, 0, width, height);
    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);

    return 0;
}

void createSdlSurface(int width, int height, int fullscreen)
{
    int flags = SDL_SWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0);
    int bytepp;

	// try default bpp
	surface = SDL_SetVideoMode(width, height, 0, flags);
	bytepp = (surface ? surface->format->BytesPerPixel : 0);
	if (bytepp != 2 && bytepp != 4) {
		surface = NULL;
	}

    if (!surface) { bitDepth = 32; surface = SDL_SetVideoMode(width, height, bitDepth, flags); }
    if (!surface) { bitDepth = 16; surface = SDL_SetVideoMode(width, height, bitDepth, flags); }

    if (surface != NULL) {
        displayData[0] = (char*)surface->pixels;
        curDisplayData = 0;
        displayPitch = surface->pitch;
    }
}


int createSdlWindow()
{
    const char *title = "Steckschwein - blueMSXlite";
    int fullscreen = properties->video.windowSize == P_VIDEO_SIZEFULLSCREEN;
    int width;
    int height;

    if (fullscreen) {
        zoom = properties->video.fullscreen.width / WIDTH;
        bitDepth = properties->video.fullscreen.bitDepth;
    }
    else {
        if (properties->video.windowSize == P_VIDEO_SIZEX1) {
            zoom = 1;
        }
        else {
            zoom = 2;
        }
        bitDepth = 32;
    }

    width  = zoom * WIDTH;
    height = zoom * HEIGHT;

	createSdlSurface(width, height, fullscreen);

    // Set the window caption
    SDL_WM_SetCaption( title, NULL );

    return 1;
}

static void handleEvent(SDL_Event* event)
{
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
/*
    case SDL_ACTIVEEVENT:
        if (event->active.state & SDL_APPINPUTFOCUS) {
            keyboardSetFocus(1, event->active.gain);
        }
        if (event->active.state == SDL_APPMOUSEFOCUS) {
            sdlMouseSetFocus(event->active.gain);
        }
        break;
    case SDL_KEYDOWN:
        shortcutCheckDown(shortcuts, HOTKEY_TYPE_KEYBOARD, keyboardGetModifiers(), event->key.keysym.sym);
        break;
    case SDL_KEYUP:
        shortcutCheckUp(shortcuts, HOTKEY_TYPE_KEYBOARD, keyboardGetModifiers(), event->key.keysym.sym);
        break;
    case SDL_VIDEOEXPOSE:
        updateEmuDisplay(1);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        sdlMouseButton(event->button.button, event->button.state);
        break;
    case SDL_MOUSEMOTION:
        sdlMouseMove(event->motion.x, event->motion.y);
        break;
*/
    }

}

#ifdef SINGLE_THREADED
int archPollEvent()
{
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        if( event.type == SDL_QUIT ) {
            doQuit = 1;
        }
        else {
            handleEvent(&event);
        }
    }
    return doQuit;
}
#endif


static int emuUseSynchronousUpdate(){
    if (properties->emulation.syncMethod == P_EMU_SYNCIGNORE) {
        return properties->emulation.syncMethod;
    }

    if (properties->emulation.speed == 50 &&
        enableSynchronousUpdate &&
        emuMaxSpeed == 0)
    {
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
    }
    else {
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

    emuTimeIdle  += li2 - li1;
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
            diffTime =  20 * syncPeriod;
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

void emulatorSetFrequency(int logFrequency, int* frequency) {
    emuFrequency = (int)(3579545 * pow(2.0, (logFrequency - 50) / 15.0515));

    if (frequency != NULL) {
        *frequency  = emuFrequency;
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
    success = boardRun(frequency,
                       reversePeriod,
                       reverseBufferCnt,
                       WaitForSync);
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
    return properties->emulation.syncMethod == P_EMU_SYNCAUTO ||
           properties->emulation.syncMethod == P_EMU_SYNCNONE ? 2 : 1;
#endif
}

void RefreshScreen(int screenMode) {

//    lastScreenMode = screenMode;
    if (emuUseSynchronousUpdate() == P_EMU_SYNCFRAMES && emuState == EMU_RUNNING) {
        emulatorSyncScreen();
    }
}

static int timerCallback(void* timer) {

    if (properties == NULL) {
        return 1;
    }
    else {
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
//                refreshRate = boardGetRefreshRate();

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

EmuState emulatorGetState(){
   return emuState;
}

void emulatorSetState(EmuState state){
   if (state == EMU_RUNNING) {
//      archSoundResume();
   }
   else {
//      archSoundSuspend();
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

void actionEmuStep() {
    if (emulatorGetState() == EMU_PAUSED) {
        emulatorSetState(EMU_STEP);
    }
}

void emulatorStart(const char* stateName) {

    dbgEnable();

    emulatorResume();

    emuExitFlag = 0;

    properties->emulation.pauseSwitch = 0;
    //switchSetPause(properties->emulation.pauseSwitch);

#ifndef NO_TIMERS
    emuSyncEvent  = archEventCreate(0);
    emuStartEvent = archEventCreate(0);
    emuTimer = archCreateTimer(emulatorGetSyncPeriod(), timerCallback);
#endif

    //setDeviceInfo(&deviceInfo);

//    inputEventReset();

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

    archEventWait(emuStartEvent, 3000);

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


void emulatorResume() {
    if (emuState == EMU_SUSPENDED) {
        //emuSysTime = 0;

        emuState = EMU_RUNNING;
        archUpdateEmuDisplay(0);
    }
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

//    archSoundSuspend();
    archThreadJoin(emuThread, 3000);
    archThreadDestroy(emuThread);
    archEventDestroy(emuStartEvent);

    archEmulationStopNotification();

    dbgDisable();
    dbgPrint();
//    savelog();
}

void actionEmuStepBack() {
    if (emulatorGetState() == EMU_PAUSED) {
        emulatorSetState(EMU_STEP_BACK);
    }
}

void actionEmuTogglePause(){
   if (emulatorGetState() == EMU_STOPPED) {
      emulatorStart(NULL);
   }
   else if (emulatorGetState() == EMU_PAUSED) {
      emulatorSetState(EMU_RUNNING);
      debuggerNotifyEmulatorResume();
   }
   else {
      emulatorSetState(EMU_PAUSED);
      debuggerNotifyEmulatorPause();
   }
   //archUpdateMenu(0);
}

static int emuFrameskipCounter = 0;

int emulatorSyncScreen()
{
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

void trace(){
#ifdef TRACE
		if (pc == trace_address && trace_address != 0) {
			trace_mode = true;
		}
		if (trace_mode) {
			printf("\t\t\t\t[%6d] ", mos6502instructions());

			char *label = label_for_address(pc);
			int label_len = label ? strlen(label) : 0;
			if (label) {
				printf("%s", label);
			}
			for (int i = 0; i < 10 - label_len; i++) {
				printf(" ");
			}
			printf(" .,%04x ", pc);
			char disasm_line[15];
			int len = disasm(pc, RAM, disasm_line, sizeof(disasm_line), false, 0);
			for (int i = 0; i < len; i++) {
				printf("%02x ", read6502(pc + i));
			}
			for (int i = 0; i < 9 - 3 * len; i++) {
				printf(" ");
			}
			printf("%s", disasm_line);
			for (int i = 0; i < 15 - strlen(disasm_line); i++) {
				printf(" ");
			}

			printf("a=$%02x x=$%02x y=$%02x s=$%02x p=", a, x, y, sp);
			for (int i = 7; i >= 0; i--) {
				printf("%c", (status & (1 << i)) ? "czidb.vn"[i] : '-');
			}
//			printf(" --- %04x", RAM[0xae]  | RAM[0xaf]  << 8);
			printf("\n");
		}
#endif
}

void instructionCb(){
	if(doQuit)
		return;

	trace();

	if (echo_mode != ECHO_MODE_NONE && (pc == 0xfff0 || pc == 0xffb3)) { //@see jumptables, bios fff0, kernel $ffb3
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

	if (pc == 0xffff) {
		if (save_on_exit) {
			machine_dump();
			doQuit = 1;
		}
	}

}

int
main(int argc, char **argv)
{
	char rom_path_data[PATH_MAX];
	char *rom_path = rom_path_data;
	char *prg_path = NULL;
	char *bas_path = NULL;
	char *sdcard_path = NULL;
	
	run_after_load = false;

	rom_path = getcwd(rom_path_data, PATH_MAX);
	if(rom_path == NULL){
		return 1;
	}	
	// This causes the emulator to load ROM data from the executable's directory when
	// no ROM file is specified on the command line.
	strncpy(rom_path + strlen(rom_path), "/rom.bin", PATH_MAX - strlen(rom_path));
	
	argc--;
	argv++;

	while (argc > 0) {
		if (!strcmp(argv[0], "-rom")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			rom_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-ram")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			int kb = atoi(argv[0]);
			bool found = false;
			for (int cmp = 8; cmp <= 2048; cmp *= 2) {
				if (kb == cmp)  {
					found = true;
				}
			}
			if (!found) {
				usage();
			}
			num_ram_banks = kb /8;
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-keymap")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage_keymap();
			}
			bool found = false;
			for (int i = 0; i < sizeof(keymaps)/sizeof(*keymaps); i++) {
				if (!strcmp(argv[0], keymaps[i])) {
					found = true;
					keymap = i;
				}
			}
			if (!found) {
				usage_keymap();
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-prg")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			prg_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-run")) {
			argc--;
			argv++;
			run_after_load = true;
		} else if (!strcmp(argv[0], "-bas")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			bas_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-sdcard")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			sdcard_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-echo")) {
			argc--;
			argv++;
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
		} else if (!strcmp(argv[0], "-log")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
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
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-dump")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
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
		} else if (!strcmp(argv[0], "-gif")) {
			argc--;
			argv++;
			// set up for recording
			record_gif = RECORD_GIF_PAUSED;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			gif_path = argv[0];
			argv++;
			argc--;
		} else if (!strcmp(argv[0], "-debug")) {
			argc--;
			argv++;
			debugger_enabled = true;
			if (argc && argv[0][0] != '-') {
				//DEBUGSetBreakPoint((uint16_t)strtol(argv[0], NULL, 16));
				argc--;
				argv++;
			}
#ifdef TRACE
		} else if (!strcmp(argv[0], "-trace")) {
			argc--;
			argv++;
			if (argc && argv[0][0] != '-') {
				trace_mode = false;
				trace_address = (uint16_t)strtol(argv[0], NULL, 16);
				argc--;
				argv++;
			} else {
				trace_mode = true;
				trace_address = 0;
			}
#endif
		} else if (!strcmp(argv[0], "-headless")) {
			headless=true;
		} else if (!strcmp(argv[0], "-scale")) {
			argc--;
			argv++;
			if(!argc || argv[0][0] == '-') {
				usage();
			}
			for(char *p = argv[0]; *p; p++) {
				switch(tolower(*p)) {
				case '1':
					window_scale = 1;
					break;
				case '2':
					window_scale = 2;
					break;
				case '3':
					window_scale = 3;
					break;
				case '4':
					window_scale = 4;
					break;
				default:
					usage();
				}
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-quality")) {
			argc--;
			argv++;
			if(!argc || argv[0][0] == '-') {
				usage();
			}
			if (!strcmp(argv[0], "nearest") ||
				!strcmp(argv[0], "linear") ||
				!strcmp(argv[0], "best")) {
				scale_quality = argv[0];
			} else {
				usage();
			}
			argc--;
			argv++;
		} else {
			usage();
		}
	}

	FILE *f = fopen(rom_path, "rb");
	if (!f) {
		printf("Cannot open %s!\n", rom_path);
		exit(1);
	}
	int rom_size = fread(ROM, 1, ROM_SIZE, f);
	(void)rom_size;
	fclose(f);

	if (sdcard_path) {
		sdcard_file = fopen(sdcard_path, "rb");
		if (!sdcard_file) {
			printf("Cannot open %s!\n", sdcard_path);
			exit(1);
		}
	}


	prg_override_start = -1;
	if (prg_path) {
		char *comma = strchr(prg_path, ',');
		if (comma) {
			prg_override_start = (uint16_t)strtol(comma + 1, NULL, 16);
			*comma = 0;
		}

		prg_file = fopen(prg_path, "rb");
		if (!prg_file) {
			printf("Cannot open %s!\n", prg_path);
			exit(1);
		}
	}

	if (bas_path) {
		FILE *bas_file = fopen(bas_path, "r");
		if (!bas_file) {
			printf("Cannot open %s!\n", bas_path);
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

	if(!headless){
		if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
			return 1;
		}
#if defined(_WIN32)
		freopen( "CON", "w", stdout );//http://sdl.beuc.net/sdl.wiki/FAQ_Console
		freopen( "CON", "w", stderr );
#endif
	}


	//register cpu hook
	hookexternal(instructionCb);

	memory_init();
	machine_reset();

    properties = propCreate(0, 0, /* P_KBD_EUROPEAN,*/ 0, "");

    properties->emulation.syncMethod = P_EMU_SYNCFRAMES;
//    properties->emulation.syncMethod = P_EMU_SYNCTOVBLANKASYNC;

    video = videoCreate();
    videoSetColors(video, properties->video.saturation, properties->video.brightness,
                  properties->video.contrast, properties->video.gamma);
    videoSetScanLines(video, properties->video.scanlinesEnable, properties->video.scanlinesPct);
    videoSetColorSaturation(video, properties->video.colorSaturationEnable, properties->video.colorSaturationWidth);

    bitDepth = 32;
    if (!createSdlWindow()) {
        return 0;
    }
    videoUpdateAll(video, properties);


	emulatorStart("Start");

//#ifdef __EMSCRIPTEN__
//	emscripten_set_main_loop(emscripten_main_loop, 0, 1);
//#else
//	emulator_loop(NULL);
//#endif

	// For stop threads before destroy.
	// Clean up.
	if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
		SDL_Quit();
	}
    videoDestroy(video);
    propDestroy(properties);

	return 0;
}

void
emscripten_main_loop(void) {
	emulator_loop(NULL);
}

void*
emulator_loop(void *param)
{
	while(!doQuit) {
		if(!headless){
			SDL_Event event;
			SDL_PollEvent(&event);
			if (event.type == SDL_QUIT) {
				break;
			}
		}

		if (debugger_enabled) {
			int dbgCmd = 1;//DEBUGGetCurrentStatus();
			if (dbgCmd > 0) continue;
			if (dbgCmd < 0) break;
		}

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
			int32_t sdlTicks = 0;//SDL_GetTicks();
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
				float frames_behind = -((float)diff_time / 16.666666);
				int load = (int)((1 + frames_behind) * 100);
				printf("Load: %d%%\n", load > 100 ? 100 : load);

				if ((int)frames_behind > 0) {
					printf("Rendering is behind %d frames.\n", -(int)frames_behind);
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
