/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Sdl/bluemsxlite.c,v $
**
** $Revision: 1.29 $
**
** $Date: 2008-04-18 04:09:54 $
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
#ifndef __APPLE__ || _WIN32 || _WIN64
//#define ENABLE_OPENGL
#endif

#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>

#include "Adapter.h"
//#include "CommandLine.h"
#include "Properties.h"
//#include "ArchFile.h"
//#include "VideoRender.h"
//#include "AudioMixer.h"
//#include "Casette.h"
//#include "PrinterIO.h"
//#include "UartIO.h"
//#include "MidiIO.h"
//#include "Machine.h"
#include "Board.h"
//#include "Emulator.h"
//#include "FileHistory.h"
//#include "Actions.h"
//#include "Language.h"
//#include "LaunchFile.h"
//#include "ArchEvent.h"
//#include "ArchSound.h"
#include "ArchThread.h"
#include "ArchTimer.h"
//#include "ArchNotifications.h"
//#include "JoystickPort.h"
//#include "SdlShortcuts.h"
//#include "SdlMouse.h"

#include "VideoRender.h"
#include "VideoManager.h"

#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#endif

UInt32* boardSysTime;
static BoardTimer* timerList = NULL;
static void handleEvent(SDL_Event* event);

static Properties* properties;
static Video* video;

static void*  emuSyncEvent;
static void*  emuStartEvent;
static void*  emuTimer;
static void*  emuThread;

static int    emuExitFlag;
static UInt32 emuSysTime = 0;
static UInt32 emuFrequency = 3579545;

static volatile int      emuSuspendFlag;
static volatile EmuState emuState = EMU_STOPPED;
static volatile int      emuSingleStep = 0;

static int enableSynchronousUpdate = 1;
static int           emuMaxSpeed = 0;
static int           emuMaxEmuSpeed = 0; // Max speed issued by emulation
static int           emuPlayReverse = 0;

static int emulationStartFailure = 0;
static UInt32 emuTimeIdle       = 0;
static UInt32 emuTimeTotal      = 1;
static UInt32 emuTimeOverflow   = 0;
static UInt32 emuUsageCurrent   = 0;
static UInt32 emuCpuSpeed       = 0;
static UInt32 emuCpuUsage       = 0;

static int doQuit = 0;

static int skipSync;
static int pendingInt;
static int pendingDisplayEvents = 0;
static void* dpyUpdateAckEvent = NULL;

static SDL_Surface *surface;
static int   bitDepth;
static int   zoom = 1;
static char* displayData[2] = { NULL, NULL };
static int   curDisplayData = 0;
static int   displayPitch = 0;
#ifdef ENABLE_OPENGL
static GLfloat texCoordX;
static GLfloat texCoordY;
static GLuint textureId;
#endif

static UInt32 boardFreq = boardFrequency();

#define WIDTH  320
#define HEIGHT 240

#define EVENT_UPDATE_DISPLAY 2
#define EVENT_UPDATE_WINDOW  3

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

static int roundUpPow2(int val) {
    int rv = 1;
    while (rv < val) rv *= 2;
    return rv;
}

#ifdef ENABLE_OPENGL
void createSdlGlSurface(int width, int height, int fullscreen)
{
    unsigned texW = roundUpPow2(width);
	unsigned texH = roundUpPow2(height);

    int flags = SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	surface = SDL_SetVideoMode(width, height, fullscreen ? bitDepth : 0, flags);

    if (surface != NULL) {
        bitDepth = surface->format->BytesPerPixel * 8;
    }

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	displayData[0]  = (char*)calloc(1, bitDepth / 8 * texW * texH);
	displayData[1]  = (char*)calloc(1, bitDepth / 8 * texW * texH);
	displayPitch = width * bitDepth / 8;

	texCoordX = (GLfloat)width  / texW;
	texCoordY = (GLfloat)height / texH;

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (bitDepth == 16) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0,
			            GL_RGB, GL_UNSIGNED_SHORT_5_6_5, displayData[0]);
	}
    else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0,
			            GL_RGBA, GL_UNSIGNED_BYTE, displayData[0]);
	}
}
#endif

int createSdlWindow()
{
    const char *title = "blueMSXlite";
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

    surface = NULL;

#ifdef ENABLE_OPENGL
    if (properties->video.driver != P_VIDEO_DRVGDI) {
        createSdlGlSurface(width, height, fullscreen);
        if (surface == NULL) {
            properties->video.driver = P_VIDEO_DRVGDI;
        }
    }
    // Invert 24 bit RGB in GL mode
    videoSetRgbMode(video, properties->video.driver != P_VIDEO_DRVGDI);
#endif
    if (surface == NULL) {
        createSdlSurface(width, height, fullscreen);
    }

    // Set the window caption
    SDL_WM_SetCaption( title, NULL );

    return 1;
}

static int isLineDirty(int y, int lines) {
    int width = zoom * WIDTH * bitDepth / 8 / sizeof(UInt32);

    UInt32* d0 = (UInt32*)(displayData[0]) + width * y;
    UInt32* d1 = (UInt32*)(displayData[1]) + width * y;
    UInt32  cmp = 0;
    UInt32  i = lines * width / 8;

    while (i-- && !cmp) {
        cmp |= d0[0] - d1[0];
        cmp |= d0[1] - d1[1];
        cmp |= d0[2] - d1[2];
        cmp |= d0[3] - d1[3];
        cmp |= d0[4] - d1[4];
        cmp |= d0[5] - d1[5];
        cmp |= d0[6] - d1[6];
        cmp |= d0[7] - d1[7];
        d0 += 8;
        d1 += 8;
    }
    return cmp;
}

void ioPortUnregister(int port){
}

static void emulatorPause(void)
{
    emulatorSetState(EMU_PAUSED);
    debuggerNotifyEmulatorPause();
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

#ifdef ENABLE_OPENGL
    if (properties->video.driver != P_VIDEO_DRVGDI) {
        const int linesPerBlock = 4;
        GLfloat coordX = texCoordX;
        GLfloat coordY = texCoordY;
        int y;
        int isDirty = 0;

        if (properties->video.horizontalStretch) {
            coordX = texCoordX * (width - 2 * borderWidth) / width;
            borderWidth = 0;
        }

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

        updateAll |= properties->video.driver == P_VIDEO_DRVDIRECTX;

        for (y = 0; y < height; y += linesPerBlock) {
            if (updateAll || isLineDirty(y, linesPerBlock)) {
                if (!isDirty) {
                    glEnable(GL_TEXTURE_2D);
                    glEnable(GL_ASYNC_TEX_IMAGE_SGIX);
	                glBindTexture(GL_TEXTURE_2D, textureId);

	                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                    isDirty = 1;
                }

                if (bitDepth == 16) {
		            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, linesPerBlock,
		                            GL_RGB, GL_UNSIGNED_SHORT_5_6_5, displayData[curDisplayData] + y * displayPitch);
	            }
                else {
		            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, linesPerBlock,
		                            GL_RGBA, GL_UNSIGNED_BYTE, displayData[curDisplayData] + y * displayPitch);
	            }
            }
        }

        if (isDirty) {
           glBegin(GL_QUADS);
	        glTexCoord2f(0,      coordY); glVertex2i(0,     height);
	        glTexCoord2f(coordX, coordY); glVertex2i(width, height);
	        glTexCoord2f(coordX, 0     ); glVertex2i(width, 0     );
	        glTexCoord2f(0,      0     ); glVertex2i(0,     0     );
	        glEnd();
            glDisable(GL_TEXTURE_2D);

	        SDL_GL_SwapBuffers();
        }

        curDisplayData ^= 1;

        return 0;
    }
#endif

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

char* langDbgMemVram()              { return "VRAM"; }
char* langDbgRegs()                 { return "Registers"; }
char* langDbgDevTms9929A()          { return "TMS9929A"; }
char* langDbgDevTms99x8A()          { return "TMS99x8A"; }
char* langDbgDevV9938()             { return "V9938"; }
char* langDbgDevV9958()             { return "V9958"; }

void archVideoOutputChange() {}

int emulatorGetSyncPeriod() {
#ifdef NO_HIRES_TIMERS
    return 10;
#else
    return properties->emulation.syncMethod == P_EMU_SYNCAUTO ||
           properties->emulation.syncMethod == P_EMU_SYNCNONE ? 2 : 1;
#endif
}

static int emuUseSynchronousUpdate()
{
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

#ifndef WII
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
            //if (emuState == EMU_RUNNING) {
                //refreshRate = boardGetRefreshRate();

                if (syncMethod == P_EMU_SYNCAUTO || syncMethod == P_EMU_SYNCNONE) {
                    archUpdateEmuDisplay(0);
                }
            //}
        }

        if (syncMethod == P_EMU_SYNCTOVBLANKASYNC) {
            archUpdateEmuDisplay(syncMethod);
        }
        // Update emulation
        archEventSet(emuSyncEvent);
    }

    return 1;
}
#endif

void emulatorSetFrequency(int logFrequency, int* frequency) {
    emuFrequency = (int)(3579545 * pow(2.0, (logFrequency - 50) / 15.0515));

    if (frequency != NULL) {
        *frequency  = emuFrequency;
    }

    boardSetFrequency(emuFrequency);
}

int emulatorGetCpuOverflow() {
    int overflow = emuTimeOverflow;
    emuTimeOverflow = 0;
    return overflow;
}

#ifndef NO_TIMERS

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

#else
#include <windows.h>

UInt32 getHiresTimer() {
    static LONGLONG hfFrequency = 0;
    LARGE_INTEGER li;

    if (!hfFrequency) {
        if (QueryPerformanceFrequency(&li)) {
            hfFrequency = li.QuadPart;
        }
        else {
            return 0;
        }
    }

    QueryPerformanceCounter(&li);

    return (DWORD)(li.QuadPart * 1000000 / hfFrequency);
}

static UInt32 busy, total, oldTime;

static int WaitForSync(int maxSpeed, int breakpointHit) {
    emuSuspendFlag = 1;

    busy += getHiresTimer() - oldTime;

    emuExitFlag |= archPollEvent();

    archPollInput();

    do {
        for (;;) {
            UInt32 sysTime = archGetSystemUpTime(1000);
            UInt32 diffTime = sysTime - emuSysTime;
            emuExitFlag |= archPollEvent();
            if (diffTime < 10) {
                continue;
            }
            emuSysTime += 10;
            if (diffTime > 30) {
                emuSysTime = sysTime;
            }
            break;
        }
    } while (!emuExitFlag && emuState != EMU_RUNNING);


    emuSuspendFlag = 0;

    total += getHiresTimer() - oldTime;
    oldTime = getHiresTimer();
#if 0
    if (total >= 1000000) {
        UInt32 pct = 10000 * busy / total;
        printf("CPU Usage = %d.%d%%\n", pct / 100, pct % 100);
        total = 0;
        busy = 0;
    }
#endif

    return emuExitFlag ? -1 : 10;
}

#endif

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

void emulatorResume() {
    if (emuState == EMU_SUSPENDED) {
        //emuSysTime = 0;

        emuState = EMU_RUNNING;
        archUpdateEmuDisplay(0);
    }
}

void emulatorStart(const char* stateName) {

    dbgEnable();

    emulatorResume();

    emuExitFlag = 0;

    properties->emulation.pauseSwitch = 0;
    //switchSetPause(properties->emulation.pauseSwitch);

#ifndef NO_TIMERS
#ifndef WII
    emuSyncEvent  = archEventCreate(0);
#endif
    emuStartEvent = archEventCreate(0);
#ifndef WII
    emuTimer = archCreateTimer(emulatorGetSyncPeriod(), timerCallback);
#endif
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


EmuState emulatorGetState(){
   return emuState;
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

int archUpdateEmuDisplay(int syncMode)
{
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

void archUpdateWindow()
{
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = EVENT_UPDATE_WINDOW;
    event.user.data1 = NULL;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);
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

void archEmulationStopNotification()
{
	 printf("archEmulationStopNotification\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

void archEmulationStartFailure()
{
	 printf("archEmulationStartFailure\n");
#ifdef RUN_EMU_ONCE_ONLY
    doQuit = 1;
#endif
}

void archTrap(UInt8 value)
{
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

void RefreshScreen(int screenMode) {

//    lastScreenMode = screenMode;
    if (emuUseSynchronousUpdate() == P_EMU_SYNCFRAMES && emuState == EMU_RUNNING) {
        emulatorSyncScreen();
    }
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

int main(int argc, char **argv)
{
    SDL_Event event;
    char szLine[8192] = "";
    int resetProperties = 1;//1 - use default
    char path[512] = "";
    int i;

    SDL_Init( SDL_INIT_EVERYTHING );
#if defined(_WIN32)
	 freopen( "CON", "w", stdout );//http://sdl.beuc.net/sdl.wiki/FAQ_Console
	 freopen( "CON", "w", stderr );
#endif

    for (i = 1; i < argc; i++) {
        if (strchr(argv[i], ' ') != NULL && argv[i][0] != '\"') {
            strcat(szLine, "\"");
            strcat(szLine, argv[i]);
            strcat(szLine, "\"");
        }
        else {
            strcat(szLine, argv[i]);
        }
        strcat(szLine, " ");
    }
    //setDefaultPaths(archGetCurrentDirectory());

//    resetProperties = emuCheckResetArgument(szLine);
/*
    strcat(path, archGetCurrentDirectory());
    strcat(path, DIR_SEPARATOR "bluemsx.ini");
*/
    properties = propCreate(resetProperties, 0, /* P_KBD_EUROPEAN,*/ 0, "");

    properties->emulation.syncMethod = P_EMU_SYNCFRAMES;
//    properties->emulation.syncMethod = P_EMU_SYNCTOVBLANKASYNC;

    if (resetProperties == 2) {
        propDestroy(properties);
        return 0;
    }

    video = videoCreate();
    videoSetColors(video, properties->video.saturation, properties->video.brightness,
                  properties->video.contrast, properties->video.gamma);
    videoSetScanLines(video, properties->video.scanlinesEnable, properties->video.scanlinesPct);
    videoSetColorSaturation(video, properties->video.colorSaturationEnable, properties->video.colorSaturationWidth);

    bitDepth = 32;
    if (!createSdlWindow()) {
        return 0;
    }

/*
    dpyUpdateAckEvent = archEventCreate(0);
    keyboardInit();

    langInit();
    langSetLanguage(properties->language);
*/
    videoUpdateAll(video, properties);

    //shortcuts = shortcutsCreate();

    //mediaDbSetDefaultRomType(properties->cartridge.defaultType);

//    boardSetVideoAutodetect(properties->video.detectActiveMonitor);

/*
    i = emuTryStartWithArguments(properties, szLine, NULL);
    if (i < 0) {
        printf("Failed to parse command line\n");
        return 0;
    }
*/
//    if (i == 0) {
			emulatorStart(NULL);
//    }

    //While the user hasn't quit
    while(!doQuit) {
        SDL_WaitEvent(&event);
        do {
            if( event.type == SDL_QUIT ) {
                doQuit = 1;
            }
            else {
                handleEvent(&event);
            }
        } while(SDL_PollEvent(&event));
    }

	// For stop threads before destroy.
	// Clean up.
	if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
		SDL_Quit();
	}

    videoDestroy(video);
    propDestroy(properties);

    return 0;
}
