#include "Steckschwein.h"
#include "MOS6502.h"
#include "VDP.h"
#include "ym3812.h"

UInt8* steckschweinRam;
static UInt32          steckschweinRamSize;
static UInt32          steckschweinRamStart;

static MOS6502* mos6502;
static YM3812* ym3812;
// TODO static DS1306 *ds1306;

static void destroy() {
  ym3812Destroy(ym3812);

	// rtcDestroy(ds1306);

  /*
   r800DebugDestroy();
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    */
  mos6502Destroy(mos6502);
}

static void reset()
{
    UInt32 systemTime = boardSystemTime();

//    slotManagerReset();

    if (mos6502 != NULL) {
      mos6502Reset(mos6502, systemTime);
    }
//    deviceManagerReset();
}


static int getRefreshRate(){
    return vdpGetRefreshRate();
}

/*
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
*/
static UInt32 getTimeTrace(int offset) {
    return mos6502GetTimeTrace(mos6502, offset);
}

int steckSchweinCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

     int success;

     int i;

     mos6502 = mos6502create(boardTimerCheckTimeout);

     boardInfo->cpuRef           = mos6502;

     boardInfo->destroy          = destroy;
     boardInfo->softReset        = reset;
     boardInfo->getRefreshRate   = getRefreshRate;
     boardInfo->getRamPage       = NULL;

     boardInfo->run              = mos6502Execute;
     boardInfo->stop             = mos6502StopExecution;

     boardInfo->setInt           = mos6502SetInt;
     boardInfo->clearInt         = mos6502ClearInt;

     boardInfo->setNmi           = mos6502SetNmi;
     boardInfo->clearNmi         = mos6502ClearNmi;

     boardInfo->setCpuTimeout    = mos6502SetTimeoutAt;

     boardInfo->setBreakpoint    = mos6502SetBreakpoint;
     boardInfo->clearBreakpoint  = mos6502ClearBreakpoint;

     boardInfo->getTimeTrace     = getTimeTrace;

     //deviceManagerCreate();

     boardInit(&mos6502->systemTime);

     ioPortReset();

     //ramMapperIoCreate();
     memoryCreate(mos6502, machine->romImage);

     mixerReset(boardGetMixer());

     ym3812 = ym3812Create(boardGetMixer());

     //msxPPICreate(machine->board.type == BOARD_MSX_FORTE_II);
     //slotManagerCreate();

//     cpuDebugCreate(mos6502);

     //ioPortRegister(0x2e, testPort, NULL, NULL);

     //sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
     //rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);

     vdpCreate(VDP_STECKSCHWEIN, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000);

     success = machineInitialize(machine, &steckschweinRam, &steckschweinRamSize, &steckschweinRamStart);
     if (success) {
//         success = boardInsertExternalDevices();
     }

     mos6502Reset(mos6502, 0);

     if (!success) {
         destroy();
     }

     return success;
}
