#include "Board.h"
#include "JuniorComputer.h"
#include "MOS6502.h"
#include "MOS6532.h"
#include "SN76489.h"
#include "Speaker.h"
#include "VDP.h"

#include "memoryJuniorComputer.h"

UInt8* jcRam;
static UInt32          jcRamSize;
static UInt32          jcRamStart;

/* Hardware */
static SN76489* sn76489;
static Speaker* speaker;
static MOS6532* mos6532;

//extern MOS6502* mos6502;

static void destroy() {

  int i;
	// rtcDestroy(ds1306);

  /*
   r800DebugDestroy();
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    */

  for(i=0xe0;i<=0xff;i++){
    ioPortUnregister(i);
  }

  speakerDestroy(speaker);
  sn76489Destroy(sn76489);
  mos6532Destroy(mos6532);
  mos6502Destroy(mos6502);
  memoryJuniorComputerDestroy();
}

static void reset()
{
    UInt32 systemTime = boardSystemTime();

//    slotManagerReset();

    if (mos6532Reset != NULL){
      mos6532Reset(mos6532, systemTime);
    }
    if (mos6502 != NULL) {
      mos6502Reset(mos6502, systemTime);
    }
    if (sn76489 != NULL) {
      sn76489Reset(sn76489);
    }
    if(speaker != NULL)
      speakerReset(speaker);
//    deviceManagerReset();
}


static int getRefreshRate(){
    return vdpGetRefreshRate();
}

/*
static UInt8* getRamPage(int page) {

    int start;

    start = page * 0x2000 - (int)jcRamStart;
    if (page < 0) {
        start += jcRamSize;
    }

    if (start < 0 || start >= (int)jcRamSize) {
        return NULL;
    }

	return jcRam + start;
}
*/
static UInt32 getTimeTrace(int offset) {
    return mos6502GetTimeTrace(mos6502, offset);
}


//called after each 6502 instruction
void jcInstructionCb(uint32_t cycles) {

  for (uint8_t i = 0; i < cycles; i++) {
    spi_step();
//    joystick_step();
    mos6532Execute(mos6532);
  }

  trace();

//  hookKernelPrgLoad(prg_file, prg_override_start);

  if (pc == 0xffff) {
    if (save_on_exit) {
      machine_dump();
      //doQuit = 1;
      actionEmuStop();
    }
  }
}

static UInt8 juniorComputerReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn){

  if (address >= JC_PORT_6532 && address < (JC_PORT_6532+JC_PORT_6532_SIZE)) // 6532 RIOT
  {
    bool ramSel = (address & 0x80) == 0;
    return mos6532Read(mos6532, ramSel, address, debugOn);
  }
  return memoryJuniorComputerReadAddress(mos6502, address, debugOn);
}

static void juniorComputerWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value){

  if (address >= JC_PORT_6532 && address < (JC_PORT_6532+JC_PORT_6532_SIZE)) // 6532 RIOT
  {
    bool ramSel = (address & 0x80) == 0;
    mos6532Write(mos6532, ramSel, address, value);

    if(mos6532->ddr_b & 0x01 && mos6532->ipr_b & 0x01){
      speakerWriteData(speaker, 0, 1);
    }
  }
  memoryJuniorComputerWriteAddress(mos6502, address, value);

}

int juniorComputerCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

  int success;

  int i;

  mos6502 = mos6502create(juniorComputerReadAddress, juniorComputerWriteAddress, boardTimerCheckTimeout);

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
  memoryJuniorComputerCreate(mos6502, machine->romImage);

  boardInit(&mos6502->systemTime);

  ioPortReset();

  mixerReset(boardGetMixer());

  mos6532 = mos6532Create();
  for(i=JC_PORT_6532;i<JC_PORT_6532+128;i++){
    ioPortRegister(i, mos6532Read, mos6532Write, mos6532);
  }

  sn76489 = sn76489Create(boardGetMixer());
  for(i=0xe0;i<=0xff;i++){
    ioPortRegister(i, NULL, sn76489WriteData, sn76489);
  }

  speaker = speakerCreate(boardGetMixer());

  //sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
  //rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);

  vdpCreate(VDP_JC, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000);

  //register cpu hook
  hookexternal(jcInstructionCb);

  success = machineInitialize(machine, &jcRam, &jcRamSize, &jcRamStart);
  if (success) {
  //         success = boardInsertExternalDevices();
  }

  mos6532Reset(mos6532, 0);
  mos6502Reset(mos6502, 0);

  if (!success) {
      destroy();
  }

  return success;
}
