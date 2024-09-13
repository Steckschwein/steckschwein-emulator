/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/Board.c,v $
**
** $Revision: 1.79 $
**
** $Date: 2009-07-18 14:35:59 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "Board.h"
/*
#include "MSX.h"
#include "SVI.h"
#include "SG1000.h"
#include "Coleco.h"
#include "Adam.h"
*/
#include "Steckschwein.h"
#include "JuniorComputer.h"
#include "AudioMixer.h"
/*
#include "YM2413.h"
#include "Y8950.h"
#include "Moonsound.h"
#include "SaveState.h"
#include "ziphelper.h"
#include "ArchNotifications.h"
*/
#include "VideoManager.h"
#include "VideoRender.h"
#include "DebugDeviceManager.h"
/*
#include "MegaromCartridge.h"
#include "Disk.h"
#include "Casette.h"
#include "MediaDb.h"
#include "RomLoader.h"
#include "JoystickPort.h"
*/
#include "Machine.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

static int skipSync;
static int pendingInt;
static int pendingNmi;
static int boardType;

static Mixer* boardMixer = NULL;
static int (*syncToRealClock)(int, int) = NULL;

UInt32* boardSysTime;

static UInt64 boardSysTime64;
static UInt32 oldTime;
static UInt32 boardFreq = boardFrequency();
static int fdcTimingEnable = 1;
static int fdcActive       = 0;
static BoardTimer* fdcTimer;
static BoardTimer* syncTimer;
static BoardTimer* mixerTimer;
static BoardTimer* stateTimer;
static BoardTimer* breakpointTimer;

static UInt32 boardRamSize;
static UInt32 boardVramSize;
static int boardRunning = 0;

static int     ramMaxStates;
static int     ramStateCur;
static int     ramStateCount;
static int     stateFrequency;
static int     enableSnapshots;
static int     useRom;
static int     useMegaRom;
static int     useMegaRam;
static int     useFmPac;

static char saveStateVersion[32] = "blueMSX - state  v 8";

static BoardTimerCb periodicCb;
static void*        periodicRef;
static UInt32       periodicInterval;
static BoardTimer*  periodicTimer;

void boardTimerCleanup();

//extern void PatchReset(BoardType boardType);

#define HIRES_CYCLES_PER_LORES_CYCLE (UInt64)100000
#define boardFrequency64() (HIRES_CYCLES_PER_LORES_CYCLE * boardFrequency())


static void boardPeriodicCallback(void* ref, UInt32 time)
{
    if (periodicCb != NULL) {
        periodicCb(periodicRef, time);
        boardTimerAdd(periodicTimer, time + periodicInterval);
    }
}

//------------------------------------------------------
// Board supports one external periodic timer (can be
// used to sync external components with the emulation)
// The callback needs to be added before the emulation
// starts in order to be called.
//------------------------------------------------------
void boardSetPeriodicCallback(BoardTimerCb cb, void* ref, UInt32 freq)
{
    periodicCb       = cb;
    periodicRef      = ref;
    if (periodicCb != NULL && freq > 0) {
        periodicInterval = boardFrequency() / freq;
    }
}

//------------------------------------------------------
// Capture stuff
//------------------------------------------------------

#define CAPTURE_VERSION     3

typedef struct {
    UInt8  index;
    UInt8  value;
    UInt16 count;
} RleData;

static RleData* rleData;
static int      rleDataSize;
static int      rleIdx;
static UInt8    rleCache[256];

static void rleEncStartEncode(void* buffer, int length, int startOffset)
{
    rleIdx = startOffset - 1;
    rleDataSize = length / sizeof(RleData) - 1;
    rleData = (RleData*)buffer;

    if (startOffset == 0) {
        memset(rleCache, 0, sizeof(rleCache));
    }
}

static void rleEncAdd(UInt8 index, UInt8 value)
{
    if (rleIdx < 0 || rleCache[index] != value || rleData[rleIdx].count == 0) {
        rleIdx++;
        rleData[rleIdx].value = value;
        rleData[rleIdx].count = 1;
        rleData[rleIdx].index = index;
        rleCache[index] = value;
    }
    else {
        rleData[rleIdx].count++;
    }
}

static int rleEncGetLength()
{
    return rleIdx + 1;
}

static void rleEncStartDecode(void* encodedData, int encodedSize)
{
    rleIdx = 0;
    rleDataSize = encodedSize;
    rleData = (RleData*)encodedData;

    memset(rleCache, 0, sizeof(rleCache));

    rleCache[rleData[rleIdx].index] = rleData[rleIdx].value;
}

static UInt8 rleEncGet(UInt8 index)
{
    UInt8 value = rleCache[index];

    rleData[rleIdx].count--;
    if (rleData[rleIdx].count == 0) {
        rleIdx++;
        rleCache[rleData[rleIdx].index] = rleData[rleIdx].value;
    }

    return value;
}

static int rleEncEof()
{
    return rleIdx > rleDataSize;
}


typedef enum
{
    CAPTURE_IDLE = 0,
    CAPTURE_REC  = 1,
    CAPTURE_PLAY = 2,
} CaptureState;

typedef struct Capture {
    BoardTimer* timer;

    UInt8  initState[0x100000];
    int    initStateSize;
    UInt32 endTime;
    UInt64 endTime64;
    UInt64 startTime64;
    CaptureState state;
    UInt8  inputs[0x100000];
    int    inputCnt;
    char   filename[512];
} Capture;

static Capture cap;

int boardCaptureHasData() {
    return cap.endTime != 0 || cap.endTime64 != 0 || boardCaptureIsRecording();
}

int boardCaptureIsRecording() {
    return cap.state == CAPTURE_REC;
}

int  boardCaptureIsPlaying() {
    return cap.state == CAPTURE_PLAY;
}

int boardCaptureCompleteAmount() {
    UInt64 length = (cap.endTime64 - cap.startTime64) / 1000;
    UInt64 current = (boardSysTime64 - cap.startTime64) / 1000;
    // Return complete if almost complete
    if (cap.endTime64 - boardSysTime64 < HIRES_CYCLES_PER_LORES_CYCLE * 100) {
        return 1000;
    }
    if (length == 0) {
        return 1000;
    }
    return (int)(1000 * current / length);
}

extern void actionEmuTogglePause();

static void boardTimerCb(void* dummy, UInt32 time)
{
    if (cap.state == CAPTURE_PLAY) {
        // If we reached the end time +/- 2 seconds we know we should stop
        // the capture. If not, we restart the timer 1/4 of max time into
        // the future. (Enventually we'll hit the real end time
        // This is an ugly workaround for the internal timers short timespan
        // (~3 minutes). Using the 64 bit timer we can extend the capture to
        // 90 days.
        // Will work correct with 'real' 64 bit timers
        boardSystemTime64(); // Sync clock
        if (boardCaptureCompleteAmount() < 1000) {
            boardTimerAdd(cap.timer, time + 0x40000000);
        }
        else {
            //actionEmuTogglePause();
            cap.state = CAPTURE_IDLE;
        }
    }

    if (cap.state == CAPTURE_REC) {
        cap.state = CAPTURE_IDLE;
//        boardCaptureStart(cap.filename);
    }
}

void boardCaptureInit()
{
    cap.timer = boardTimerCreate(boardTimerCb, NULL);
    if (cap.state == CAPTURE_REC) {
        boardTimerAdd(cap.timer, boardSystemTime() + 1);
    }
}

//------------------------------------------------------


int boardGetNoSpriteLimits() {
    return vdpGetNoSpritesLimit();
}

void boardSetNoSpriteLimits(int enable) {
    vdpSetNoSpriteLimits(enable);
}

int boardGetFdcTimingEnable() {
    return fdcTimingEnable;
}

void boardSetFdcTimingEnable(int enable) {
    fdcTimingEnable = enable;
}

void boardSetFdcActive() {
    if (!fdcTimingEnable) {
        boardTimerAdd(fdcTimer, boardSystemTime() + (UInt32)((UInt64)500 * boardFrequency() / 1000));
        fdcActive = 1;
    }
}

void boardSetBreakpoint(UInt16 address) {
    if (boardRunning) {
        boardInfo.setBreakpoint(boardInfo.cpuRef, address);
    }
}

void boardClearBreakpoint(UInt16 address) {
    if (boardRunning) {
        boardInfo.clearBreakpoint(boardInfo.cpuRef, address);
    }
}

static void onFdcDone(void* ref, UInt32 time)
{
    fdcActive = 0;
}

static void doSync(UInt32 time, int breakpointHit)
{
    int execTime = 10;
    if (!skipSync) {
        execTime = syncToRealClock(fdcActive, breakpointHit);
    }
    if (execTime == -99) {
        boardInfo.stop(boardInfo.cpuRef);
        return;
    }

    boardSystemTime64();

    if (execTime == 0) {
        boardTimerAdd(syncTimer, boardSystemTime() + 1);
    }
    else if (execTime < 0) {
        execTime = -execTime;
        boardTimerAdd(syncTimer, boardSystemTime() + (UInt32)((UInt64)execTime * boardFreq / 1000));
    }
    else {
        boardTimerAdd(syncTimer, time + (UInt32)((UInt64)execTime * boardFreq / 1000));
    }
}

static void onMixerSync(void* ref, UInt32 time)
{
    mixerSync(boardMixer);

    boardTimerAdd(mixerTimer, boardSystemTime() + boardFrequency() / 50);
}

static void onStateSync(void* ref, UInt32 time)
{
    if (enableSnapshots) {
        char memFilename[8];
        ramStateCur = (ramStateCur + 1) % ramMaxStates;
        if (ramStateCount < ramMaxStates) {
            ramStateCount++;
        }

        sprintf(memFilename, "mem%d", ramStateCur);

        //boardSaveState(memFilename, 0);
    }

    boardTimerAdd(stateTimer, boardSystemTime() + stateFrequency);
}

static void onSync(void* ref, UInt32 time)
{
    doSync(time, 0);
}

void boardOnBreakpoint(UInt16 pc)
{
    doSync(boardSystemTime(), 1);
}

static void onBreakpointSync(void* ref, UInt32 time) {
    skipSync = 0;
    doSync(time, 1);
}

int boardRewindOne() {
    UInt32 rewindTime;
    if (stateFrequency <= 0) {
        return 0;
    }
    rewindTime = boardInfo.getTimeTrace(1);
    if (rewindTime == 0 || !boardRewind()) {
        return 0;
    }
    boardTimerAdd(breakpointTimer, rewindTime);
    skipSync = 1;
    return 1;
}

int boardRewind()
{
    char stateFile[8];

    if (ramStateCount < 2) {
        return 0;
    }

    ramStateCount--;
    sprintf(stateFile, "mem%d", ramStateCur);
    ramStateCur = (ramStateCur + ramMaxStates - 1) % ramMaxStates;

    boardTimerCleanup();

    //saveStateCreateForRead(stateFile);

//    boardType = boardLoadState();
//    machineLoadState(boardMachine);

//    boardInfo.loadState();
    //boardCaptureLoadState();

#if 1
    if (stateFrequency > 0) {
        boardTimerAdd(stateTimer, boardSystemTime() + stateFrequency);
    }
    //boardTimerAdd(syncTimer, boardSystemTime() + 1);
    boardTimerAdd(mixerTimer, boardSystemTime() + boardFrequency() / 50);

    if (boardPeriodicCallback != NULL) {
        boardTimerAdd(periodicTimer, boardSystemTime() + periodicInterval);
    }
#endif
    return 1;
}

void boardSetFrequency(int frequency)
{
   boardFreq = frequency * (boardFrequency() / 3579545);

   mixerSetBoardFrequency(frequency);
}

int boardGetRefreshRate()
{
    if (boardRunning) {
        return boardInfo.getRefreshRate();
    }
    return 0;
}

void   boardSetNmi(UInt32 nmi){
    pendingNmi |= nmi;
    boardInfo.setNmi(boardInfo.cpuRef);
}

void   boardClearNmi(UInt32 nmi){
    pendingNmi &= ~nmi;
    if (pendingNmi == 0) {
        boardInfo.clearNmi(boardInfo.cpuRef);
    }
}

UInt32 boardGetNmi(UInt32 nmi){
 return pendingNmi & nmi;
}

void boardSetInt(UInt32 irq)
{
    pendingInt |= irq;
    boardInfo.setInt(boardInfo.cpuRef);
}

void boardClearInt(UInt32 irq)
{
    pendingInt &= ~irq;
    if (pendingInt == 0) {
        boardInfo.clearInt(boardInfo.cpuRef);
    }
}

UInt32 boardGetInt(UInt32 irq)
{
    return pendingInt & irq;
}

UInt8* boardGetRamPage(int page)
{
    if (boardInfo.getRamPage == NULL) {
        return NULL;
    }
    return boardInfo.getRamPage(page);
}

UInt32 boardGetRamSize()
{
    return boardRamSize;
}

UInt32 boardGetVramSize()
{
    return boardVramSize;
}

int boardUseRom()
{
    return useRom;
}

int boardUseMegaRom()
{
    return useMegaRom;
}

int boardUseMegaRam()
{
    return useMegaRam;
}

int boardUseFmPac()
{
    return useFmPac;
}

UInt32 boardCalcRelativeTimeout(UInt32 timerFrequency, UInt32 nextTimeout)
{
    UInt64 currentTime = boardSystemTime64();
    UInt64 frequency   = boardFrequency64() / timerFrequency;

    currentTime = frequency * (currentTime / frequency);

    return (UInt32)((currentTime + nextTimeout * frequency) / HIRES_CYCLES_PER_LORES_CYCLE);
}

/////////////////////////////////////////////////////////////
// Board timer

typedef struct BoardTimer {
    BoardTimer*  next;
    BoardTimer*  prev;
    BoardTimerCb callback;
    void*        ref;
    UInt32       timeout;
};

static BoardTimer* timerList = NULL;
static UInt32 timeAnchor;
static int    timeoutCheckBreak;

#define MAX_TIME  (2 * 1368 * 313)
#define TEST_TIME 0x7fffffff

BoardTimer* boardTimerCreate(BoardTimerCb callback, void* ref)
{
    BoardTimer* timer = malloc(sizeof(BoardTimer));

    timer->next     = timer;
    timer->prev     = timer;
    timer->callback = callback;
    timer->ref      = ref ? ref : timer;
    timer->timeout  = 0;

    return timer;
}

void boardTimerDestroy(BoardTimer* timer)
{
    boardTimerRemove(timer);

    free(timer);
}

void boardTimerAdd(BoardTimer* timer, UInt32 timeout)
{
    UInt32 currentTime = boardSystemTime();
    BoardTimer* refTimer;
    BoardTimer* next = timer->next;
    BoardTimer* prev = timer->prev;

    // Remove current timer
    next->prev = prev;
    prev->next = next;

    timerList->timeout = currentTime + TEST_TIME;

    refTimer = timerList->next;

    if (timeout - timeAnchor - TEST_TIME < currentTime - timeAnchor - TEST_TIME) {
        timer->next = timer;
        timer->prev = timer;

        // Time has already expired
        return;
    }

    while (timeout - timeAnchor > refTimer->timeout - timeAnchor) {
        refTimer = refTimer->next;
    }
#if 0
    {
        static int highWatermark = 0;
        int cnt = 0;
        BoardTimer* t = timerList->next;
        while (t != timerList) {
            cnt++;
            t = t->next;
        }
        if (cnt > highWatermark) {
            highWatermark = cnt;
            printf("HIGH: %d\n", highWatermark);
        }
    }
#endif

    timer->timeout       = timeout;
    timer->next          = refTimer;
    timer->prev          = refTimer->prev;
    refTimer->prev->next = timer;
    refTimer->prev       = timer;

    boardInfo.setCpuTimeout(boardInfo.cpuRef, timerList->next->timeout);
}

void boardTimerRemove(BoardTimer* timer)
{
    BoardTimer* next = timer->next;
    BoardTimer* prev = timer->prev;

    next->prev = prev;
    prev->next = next;

    timer->next = timer;
    timer->prev = timer;
}

void boardTimerCleanup()
{
    while (timerList->next != timerList) {
        boardTimerRemove(timerList->next);
    }

    timeoutCheckBreak = 1;
}

void boardTimerCheckTimeout(void* usrDefinedCb)
{
    UInt32 currentTime = boardSystemTime();
    timerList->timeout = currentTime + MAX_TIME;

    timeoutCheckBreak = 0;
    while (!timeoutCheckBreak) {
        BoardTimer* timer = timerList->next;
        if (timer == timerList) {
            return;
        }
        if (timer->timeout - timeAnchor > currentTime - timeAnchor) {
            break;
        }

        boardTimerRemove(timer);
        timer->callback(timer->ref, timer->timeout);
    }

    timeAnchor = boardSystemTime();

    boardInfo.setCpuTimeout(boardInfo.cpuRef, timerList->next->timeout);
}

UInt64 boardSystemTime64() {
    UInt32 currentTime = boardSystemTime();
    boardSysTime64 += HIRES_CYCLES_PER_LORES_CYCLE * (currentTime - oldTime);
    oldTime = currentTime;
    return boardSysTime64;
}

void boardInit(UInt32* systemTime)
{
    static BoardTimer dummy_timer;
    boardSysTime = systemTime;
    oldTime = *systemTime;
    boardSysTime64 = oldTime * HIRES_CYCLES_PER_LORES_CYCLE;

    DEBUG ("boardInit %p\n", boardSysTime);

    timeAnchor = *systemTime;

    if (timerList == NULL) {
        dummy_timer.next     = &dummy_timer;
        dummy_timer.prev     = &dummy_timer;
        dummy_timer.callback = NULL;
        dummy_timer.ref      = &dummy_timer;
        dummy_timer.timeout  = 0;
        timerList = &dummy_timer;
    }
}

int boardRun(Machine* machine,
            Mixer* mixer,
            int frequency,
            int reversePeriod,
            int reverseBufferCnt,
            int (*syncCallback)(int, int))
{
    int loadState = 0;
    int success = 1;

    syncToRealClock = syncCallback;

    videoManagerReset();
    debugDeviceManagerReset();

    boardMixer = mixer;

    boardType = machine->board.type;
    //PatchReset(boardType);

    pendingInt = 0;

    boardSetFrequency(frequency);

    memset(&boardInfo, 0, sizeof(boardInfo));

    boardRunning = 1;

    VdpSyncMode vdpSyncMode = VDP_SYNC_AUTO;
    switch (boardType) {
      case BOARD_STECKSCHWEIN:
        success = steckSchweinCreate(machine, vdpSyncMode, &boardInfo);
        break;
      case BOARD_JC:
        success = juniorComputerCreate(machine, vdpSyncMode, &boardInfo);
        break;
     default:
        success = 0;
    }


    boardCaptureInit();

    if (success) {
        syncTimer = boardTimerCreate(onSync, NULL);
        fdcTimer = boardTimerCreate(onFdcDone, NULL);
        mixerTimer = boardTimerCreate(onMixerSync, NULL);

        stateFrequency = boardFrequency() / 1000 * reversePeriod;

        if (stateFrequency > 0) {
            ramStateCur  = 0;
            ramMaxStates = reverseBufferCnt;
            stateTimer = boardTimerCreate(onStateSync, NULL);
            breakpointTimer = boardTimerCreate(onBreakpointSync, NULL);
            boardTimerAdd(stateTimer, boardSystemTime() + stateFrequency);
        }
        else {
            stateTimer = NULL;
        }

        boardTimerAdd(syncTimer, boardSystemTime() + 1);

        if (boardPeriodicCallback != NULL) {
            periodicTimer = boardTimerCreate(boardPeriodicCallback, periodicRef);
            boardTimerAdd(periodicTimer, boardSystemTime() + periodicInterval);
        }

        if (!skipSync) {
            syncToRealClock(0, 0);
        }

        boardInfo.run(boardInfo.cpuRef);

        if (periodicTimer != NULL) {
            boardTimerDestroy(periodicTimer);
            periodicTimer = NULL;
        }

        //boardCaptureDestroy();

        boardInfo.destroy();

        boardTimerDestroy(fdcTimer);
        boardTimerDestroy(syncTimer);
        boardTimerDestroy(mixerTimer);
        if (breakpointTimer != NULL) {
            boardTimerDestroy(breakpointTimer);
        }
        if (stateTimer != NULL) {
            boardTimerDestroy(stateTimer);
        }
    }

    boardRunning = 0;

    return success;
}

/////////////////////////////////////////////////////////////
// Not board specific stuff....

static char baseDirectory[512];
static int oversamplingYM2413    = 1;
static int oversamplingY8950     = 1;
static int oversamplingYM3812    = 1;
static int oversamplingMoonsound = 1;
static int enableYM2413          = 1;
static int enableY8950           = 1;
static int enableMoonsound       = 1;
static int videoAutodetect       = 1;

const char* boardGetBaseDirectory() {
    return baseDirectory;
}

Mixer* boardGetMixer()
{
    return boardMixer;
}

void boardSetMachine(Machine* machine)
{

}

void boardReset()
{
    if (boardRunning) {
        boardInfo.softReset();
    }
}

void boardSetDirectory(const char* dir) {
    strcpy(baseDirectory, dir);
}

void boardSetYm2413Oversampling(int value) {
    oversamplingYM2413 = value;
}

int boardGetYm2413Oversampling() {
    return oversamplingYM2413;
}

void boardSetY8950Oversampling(int value) {
    oversamplingY8950 = value;
}

int boardGetY8950Oversampling() {
    return oversamplingY8950;
}

int boardGetYM3812Oversampling() {
    return oversamplingYM3812;
}

void boardSetMoonsoundOversampling(int value) {
    oversamplingMoonsound = value;
}

int boardGetMoonsoundOversampling() {
    return oversamplingMoonsound;
}

void boardSetYm2413Enable(int value) {
    enableYM2413 = value;
}

int boardGetYm2413Enable() {
    return enableYM2413;
}

void boardSetY8950Enable(int value) {
    enableY8950 = value;
}

int boardGetY8950Enable() {
    return enableY8950;
}

void boardSetMoonsoundEnable(int value) {
    enableMoonsound = value;
}

int boardGetMoonsoundEnable() {
    return enableMoonsound;
}

void boardSetVideoAutodetect(int value) {
    videoAutodetect = value;
}

int  boardGetVideoAutodetect() {
    return videoAutodetect;
}
