#include "Steckschwein.h"
#include "MOS6502.h"
//test
#include "VDP.h"
#include "ym3812.h"

UInt8* steckschweinRam;
static UInt32          steckschweinRamSize;
static UInt32          steckschweinRamStart;

static MOS6502* mos6502;
static YM3812* ym3812;

static void destroy() {
	ym3812Destroy(ym3812);
/*
   rtcDestroy(rtc);

   r800DebugDestroy();
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    mos6502Destroy(mos6502);
    */
}

static int getRefreshRate(){
    return vdpGetRefreshRate();
}

static UInt8* getRamPage(int page) {

    int start;

    start = page * 0x2000 - (int)steckschweinRamStart;
    if (page < 0) {
        start += steckschweinRamSize;
    }

    if (start < 0 || start >= (int)steckschweinRamSize) {
        return NULL;
    }

	return steckschweinRam + start;
}

static UInt32 getTimeTrace(int offset) {
    return mos6502GetTimeTrace(mos6502, offset);
}

int steckSchweinCreate(VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

     int success;
     int i;

     steckschweinRam = NULL;

     //r800 = r800Create(cpuFlags, slotRead, slotWrite, ioPortRead, ioPortWrite, PatchZ80, boardTimerCheckTimeout, NULL, NULL, NULL, NULL, NULL, NULL);
     mos6502 = mos6502create(boardTimerCheckTimeout);

     boardInfo->cpuRef           = mos6502;//r800;

     boardInfo->destroy          = destroy;
     boardInfo->getRefreshRate   = getRefreshRate;
     boardInfo->getRamPage       = getRamPage;

     boardInfo->run              = mos6502Execute;
     boardInfo->stop             = mos6502StopExecution;

     boardInfo->setInt           = mos6502SetInt;
     boardInfo->clearInt         = mos6502ClearInt;
     boardInfo->setCpuTimeout    = mos6502SetTimeoutAt;

     boardInfo->setBreakpoint    = mos6502SetBreakpoint;
     boardInfo->clearBreakpoint  = mos6502ClearBreakpoint;

     boardInfo->getTimeTrace     = getTimeTrace;

     //deviceManagerCreate();

     boardInit(&mos6502->systemTime);

     ioPortReset();
     //ramMapperIoCreate();

     mos6502Reset(mos6502, 0);

     mixerReset(boardGetMixer());

     ym3812 = ym3812Create(boardGetMixer());
     //msxPPICreate(machine->board.type == BOARD_MSX_FORTE_II);
     //slotManagerCreate();

     //r800DebugCreate(r800);

 	  //ioPortRegister(0x2e, testPort, NULL, NULL);

     //sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
     //rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);


     int vramSize = 0x20000;
     vdpCreate(VDP_STECKSCHWEIN, VDP_V9958, vdpSyncMode, vramSize / 0x4000);

     success = 1;//machineInitialize(machine, &msxRam, &msxRamSize, &msxRamStart);
     if (success) {
//         success = boardInsertExternalDevices();
     }

     if (!success) {
         destroy();
     }

     return success;
}
