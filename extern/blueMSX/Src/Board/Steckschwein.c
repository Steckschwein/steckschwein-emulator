#include "Steckschwein.h"
#include "Adapter.h"

UInt8* steckschweinRam;
static UInt32          steckschweinRamSize;
static UInt32          steckschweinRamStart;

static MOS6502* mos6502;

static void destroy() {
/*
   rtcDestroy(rtc);

   r800DebugDestroy();
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    r800Destroy(r800);
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

void mos6502SetInt(MOS6502* mos6502) {

}

void mos6502Execute(MOS6502* mos6502) {

}

void mos6502ClearInt(MOS6502* mos6502) {
//    mos6502->intState = INT_HIGH;
}

void mos6502SetTimeoutAt(MOS6502* mos6502, SystemTime time){
    //r800->timeout = time;
}

void mos6502SetBreakpoint(MOS6502* mos6502, UInt16 address){
   /*
#ifdef ENABLE_BREAKPOINTS
    if (r800->breakpoints[address] == 0) {
        r800->breakpoints[address] = 1;
        r800->breakpointCount++;
    }
#endif
*/
}
void mos6502ClearBreakpoint(MOS6502* mos6502, UInt16 address)
{
   /*
#ifdef ENABLE_BREAKPOINTS
    if (r800->breakpoints[address] != 0) {
        r800->breakpointCount--;
        r800->breakpoints[address] = 0;
    }
#endif
*/
}

void mos6502StopExecution(MOS6502* mos6502) {
//    mos6502->terminate = 1;
}

static UInt32 mos6502GetTimeTrace(MOS6502* mos6502, int offset){
   return mos6502->systemTime;
}

static UInt32 getTimeTrace(int offset) {
    return mos6502GetTimeTrace(mos6502, offset);
}

int steckSchweinCreate(VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

     int success;
     int i;

     steckschweinRam = NULL;

     //r800 = r800Create(cpuFlags, slotRead, slotWrite, ioPortRead, ioPortWrite, PatchZ80, boardTimerCheckTimeout, NULL, NULL, NULL, NULL, NULL, NULL);
     mos6502 = malloc(sizeof(MOS6502));

     boardInfo->cpuRef           = NULL;//r800;

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

     //ioPortReset();
     //ramMapperIoCreate();

     //r800Reset(mos6502, 0);
     //mixerReset(boardGetMixer());

     //msxPPICreate(machine->board.type == BOARD_MSX_FORTE_II);
     //slotManagerCreate();

     //r800DebugCreate(r800);

 	  //ioPortRegister(0x2e, testPort, NULL, NULL);

     //sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
     //rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);

     //msxRam = NULL;

     VdpVersion vdpVersion = VDP_V9958;
     int vramSize = 0x20000;
     vdpCreate(VDP_MSX, vdpVersion, vdpSyncMode, vramSize / 0x4000);

     success = 1;//machineInitialize(machine, &msxRam, &msxRamSize, &msxRamStart);

     if (success) {
//         success = boardInsertExternalDevices();
     }

     if (!success) {
         destroy();
     }

     return success;
}
