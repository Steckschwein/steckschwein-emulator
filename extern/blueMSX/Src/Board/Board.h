/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/Board.h,v $
**
** $Revision: 1.40 $
**
** $Date: 2007-03-20 02:30:31 $
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
#ifndef BOARD_H
#define BOARD_H

#include "MsxTypes.h"
//#include "MediaDb.h"
//#include "Machine.h"
#include "VDP.h"
//#include "AudioMixer.h"
#include <stdio.h>

typedef struct {
   /*
    int  cartridgeCount;
    int  diskdriveCount;
    int  casetteCount;
    */
    void* cpuRef;
    
    void   (*destroy)();
    /*
    void   (*softReset)();
    void   (*loadState)();
    void   (*saveState)();
*/
    int    (*getRefreshRate)();
    UInt8* (*getRamPage)(int);
/*
    void   (*setDataBus)(void*, UInt8, UInt8, int);

    */
    void   (*run)(void*);
    void   (*stop)(void*);

    void   (*setInt)(void*);
    void   (*clearInt)(void*);
    void   (*setCpuTimeout)(void*, UInt32);

    void   (*setBreakpoint)(void*, UInt16);
    void   (*clearBreakpoint)(void*, UInt16);
/*
    void   (*changeCartridge)(void*, int, int);
*/
    UInt32   (*getTimeTrace)(int);

} BoardInfo;

static BoardInfo boardInfo;

void boardInit(UInt32* systemTime);

UInt64 boardSystemTime64();

void boardSetFrequency(int frequency);
int  boardGetRefreshRate();

void boardSetBreakpoint(UInt16 address);
void boardClearBreakpoint(UInt16 address);

void   boardSetInt(UInt32 irq);
void   boardClearInt(UInt32 irq);
UInt32 boardGetInt(UInt32 irq);

UInt8* boardGetRamPage(int page);
UInt32 boardGetRamSize();
UInt32 boardGetVramSize();

int boardUseRom();
int boardUseMegaRom();
int boardUseMegaRam();
int boardUseFmPac();

void boardSetNoSpriteLimits(int enable);
int boardGetNoSpriteLimits();

typedef enum { HD_NONE, HD_SUNRISEIDE, HD_BEERIDE, HD_GIDE, HD_RSIDE,
               HD_MEGASCSI, HD_WAVESCSI, HD_GOUDASCSI, HD_NOWIND } HdType;
HdType boardGetHdType(int hdIndex);

const char* boardGetBaseDirectory();

#define boardFrequency() (6 * 3579545)	//

static UInt32 boardSystemTime() {
    extern UInt32* boardSysTime;
    return *boardSysTime;
}

UInt64 boardSystemTime64();

typedef void (*BoardTimerCb)(void* ref, UInt32 time);

typedef struct BoardTimer BoardTimer;

BoardTimer* boardTimerCreate(BoardTimerCb callback, void* ref);
void boardTimerDestroy(BoardTimer* timer);
void boardTimerAdd(BoardTimer* timer, UInt32 timeout);
void boardTimerRemove(BoardTimer* timer);
void boardTimerCheckTimeout(void* usrDefinedCb);
UInt32 boardCalcRelativeTimeout(UInt32 timerFrequency, UInt32 nextTimeout);

void   boardOnBreakpoint(UInt16 pc);

void boardSetVideoAutodetect(int value);
int  boardGetVideoAutodetect();

void boardSetPeriodicCallback(BoardTimerCb cb, void* reference, UInt32 frequency);

#endif /* BOARD_H */
