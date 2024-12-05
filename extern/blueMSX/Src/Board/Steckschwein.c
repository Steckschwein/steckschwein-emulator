// MIT License
//
// Copyright (c) 2018 Thomas Woinke, Marko Lauke, www.steckschwein.de
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Steckschwein.h"
#include "memorySteckschwein.h"
#include "MOS6502.h"
#include "MOS6502Debug.h"
#include "ym3812.h"
#include "SN76489.h"
#include "Board.h"

// Hardware
//static MOS6502* mos6502;
//extern MOS6502* mos6502;

static YM3812 *ym3812;
static SN76489 *sn76489;
// TODO static DS1306 *ds1306;

UInt8* steckschweinRam;
static UInt32          steckschweinRamSize;
static UInt32          steckschweinRamStart;

static void destroy() {

  int i;

  for(i=STECKSCHWEIN_PORT_OPL;i<STECKSCHWEIN_PORT_OPL+STECKSCHWEIN_PORT_SIZE;i++){
    ioPortUnregister(i);
  }
  for(i=STECKSCHWEIN_PORT_SLOT0;i<STECKSCHWEIN_PORT_SLOT0+STECKSCHWEIN_PORT_SIZE;i++){
    ioPortUnregister(i);
  }
  for(i=STECKSCHWEIN_PORT_SLOT1;i<STECKSCHWEIN_PORT_SLOT1+STECKSCHWEIN_PORT_SIZE;i++){
    ioPortUnregister(i);
  }
  ym3812Destroy(ym3812);

	// rtcDestroy(ds1306);

   mos6502DebugDestroy();
  /*
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    */
  ds1306Destroy();

  mos6502Destroy(mos6502);
}

static void reset()
{
    UInt32 systemTime = boardSystemTime();

//    slotManagerReset();
  ds1306Reset();

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
        fprintf(stderr, "invalid program start address 0x%4x, will override kernel!\n", start);
      } else {
//        uint16_t end = start + fread(ram + start, 1, RAM_SIZE - start, prg_file);
      }
      fclose(prg_file);
      prg_file = NULL;
    }
  }
}

//called after each 6502 instruction
void steckschweinInstructionCb(uint32_t cycles) {

  for (uint8_t i = 0; i < cycles; i++) {
    spi_step();
    joystick_step();
  }

  trace();

  if (!isDebuggerEnabled) {
    hookCharOut();
  }

//  hookKernelPrgLoad(prg_file, prg_override_start);

  if (pc == 0xffff) {
    if (save_on_exit) {
      machine_dump();
      //doQuit = 1;
      actionEmuStop();
    }
  }
}

static bool isPort(UInt16 address, UInt16 port){
  return (address >= port && address < port+STECKSCHWEIN_PORT_SIZE);
}

static UInt8 steckschweinReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn){

  if (isPort(address, STECKSCHWEIN_PORT_UART)) {  // UART 0x200
    return uart_read(address & 0xf);
  }else if (isPort(address, STECKSCHWEIN_PORT_VIA)){    // VIA 0x210
    return via1_read(address & 0xf);
  }else if (isPort(address, STECKSCHWEIN_PORT_VDP)){    // VDP 0x220
    return ioPortRead(NULL, address);
  }else if (isPort(address, STECKSCHWEIN_PORT_OPL)){    // OPL2 0x240
    return ioPortRead(ym3812, address);
  }else if (isPort(address, STECKSCHWEIN_PORT_SLOT0)){  // slot 0 at 0x250
    return 0xff;
  }else{
    return memorySteckschweinReadAddress(mos6502, address, debugOn);
  }
}

static void steckschweinWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value){

  if (isPort(address, STECKSCHWEIN_PORT_UART)) {  // UART 0x200
    if (log_uart_writes)
    {
      fprintf(stdout, "write $%02x to %s at $%04x \n", value, "UART", address);
    }
    uart_write(address & 0xf, value);
  }else if (isPort(address, STECKSCHWEIN_PORT_VIA)){    // VIA 0x210
    if (log_via_writes)
    {
      log_write(address, value, "VIA");
    }
    via1_write(address & 0xf, value);
  }else if (isPort(address, STECKSCHWEIN_PORT_VDP)){    // VDP 0x220
    if (log_vdp_writes)
    {
      log_write(address, value, "VDP");
    }
    ioPortWrite(NULL, address, value);
  }else if (isPort(address, STECKSCHWEIN_PORT_OPL)){    // OPL2 0x240
    if (log_opl_writes)
    {
      log_write(address, value, "OPL2");
    }
    ioPortWrite(ym3812, address, value);
  }else if (isPort(address, STECKSCHWEIN_PORT_SLOT0)){  // slot 0 at 0x250
    ioPortWrite(sn76489, address, value);
  }else{
    memorySteckschweinWriteAddress(mos6502, address, value);
  }
}

int steckSchweinCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

  int success;

  int i;

  mos6502 = mos6502create(steckschweinReadAddress, steckschweinWriteAddress, boardTimerCheckTimeout, machine->cpu.freqCPU);

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
  if(machine->board.type == BOARD_STECKSCHWEIN ){
    //TODO old hardware support
    return success;
  }else {
    memorySteckschweinCreate(mos6502, machine->romImage);
  }

  mixerReset(boardGetMixer());

  ym3812 = ym3812Create(boardGetMixer());
  for(i=STECKSCHWEIN_PORT_OPL;i<STECKSCHWEIN_PORT_OPL+STECKSCHWEIN_PORT_SIZE;i++){
   ioPortRegister(i, ym3812Read, ym3812Write, ym3812);
  }

  sn76489 = sn76489Create(boardGetMixer());
  for(i=STECKSCHWEIN_PORT_SLOT0;i<STECKSCHWEIN_PORT_SLOT0+STECKSCHWEIN_PORT_SIZE;i++){
   ioPortRegister(i, NULL, sn76489WriteData, sn76489);
  }

  //msxPPICreate(machine->board.type == BOARD_MSX_FORTE_II);
  //slotManagerCreate();

  mos6502DebugCreate(mos6502);

  //ioPortRegister(0x2e, testPort, NULL, NULL);

  //sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
  //rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);

  vdpCreate(VDP_STECKSCHWEIN, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000, STECKSCHWEIN_PORT_VDP);

  //register cpu hook
  hookexternal(steckschweinInstructionCb);

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
