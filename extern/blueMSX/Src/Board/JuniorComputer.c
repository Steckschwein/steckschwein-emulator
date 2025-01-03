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

#include "Board.h"
#include "JuniorComputer.h"
#include "MOS6502.h"
#include "MOS6532.h"
#include "6551.h"
#include "Speaker.h"

#include "memoryJuniorComputer.h"
#include "jcFloppyGfxCard.h"
#include "jcIoCard.h"

UInt8* jcRam;
static UInt32          jcRamSize;
static UInt32          jcRamStart;

/* Hardware */
static Speaker *speaker;
static MOS6532 *mos6532;
static MOS6551 *mos6551;

static JcFloppyGfxCard *jcFgcCard;
static JuniorComputerIoCard *jcIoCard;

static void destroy() {

  int i;

  /*
    ioPortUnregister(0x2e);
    deviceManagerDestroy();
    */

  speakerDestroy(speaker);
  mos6532Destroy(mos6532);
  mos6502Destroy(mos6502);
  jcFloppyGfxCardDestroy(jcFgcCard);
  juniorComputerIoCardDestroy(jcIoCard);

  memoryJuniorComputerDestroy();
}

static void reset()
{
    UInt32 systemTime = boardSystemTime();

    if (mos6532Reset != NULL){
      mos6532Reset(mos6532, systemTime);
    }
    if (mos6502 != NULL) {
      mos6502Reset(mos6502, systemTime);
    }
    if (mos6551 != NULL) {
      mos6551Reset(mos6551, systemTime);
    }
    if(speaker != NULL) {
      speakerReset(speaker);
    }
    if(jcFgcCard){
      jcFloppyGfxCardReset(jcFgcCard);
    }
    if(jcIoCard){
      juniorComputerIoCardReset(jcIoCard);
    }
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
    mos6551Execute(mos6551, 1);
  }

  trace();

  if (pc == 0xffff) {
    if (save_on_exit) {
      //machine_dump();
      //doQuit = 1;
      actionEmuStop();
    }
  }
}


static UInt8 juniorComputerReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn){

  if (address >= JC_PORT_K2 && address < (JC_PORT_K2 + JC_PORT_SIZE)){
    return jcIoCardRead(address);
  }else if (address >= JC_PORT_K3 && address < (JC_PORT_K3 + JC_PORT_SIZE)){
    return ioPortRead(NULL, address);
  }else if (address >= JC_PORT_K4 && address < (JC_PORT_K4 + JC_PORT_SIZE)){
    return jcFloppyGfxCardRead(jcFgcCard, address);
  }else if (address >= JC_PORT_6532 && address < (JC_PORT_6532+JC_PORT_6532_SIZE)) // 6532 RIOT
  {
    bool ramSel = (address & 0x80) == 0;
    return mos6532Read(mos6532, ramSel, address, debugOn);
  }else if(address >= JC_PORT_6551 && address < (JC_PORT_6551+JC_PORT_6551_SIZE)){ // 6551 ACIA){
    return mos6551Read(mos6551, address);
  }
  return memoryJuniorComputerReadAddress(mos6502, address, debugOn);
}

static void juniorComputerWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value){

  if (address >= JC_PORT_K2 && address < (JC_PORT_K2 + JC_PORT_SIZE)){
    jcIoCardWrite(address, value);
  }else if (address >= JC_PORT_K3 && address < (JC_PORT_K3 + JC_PORT_SIZE)){
    ioPortWrite(NULL, address, value);
  }else if (address >= JC_PORT_K4 && address < (JC_PORT_K4 + JC_PORT_SIZE)){
    jcFloppyGfxCardWrite(jcFgcCard, address, value);
  }else  if (address >= JC_PORT_6532 && address < (JC_PORT_6532+JC_PORT_6532_SIZE)) // 6532 RIOT
  {
    bool ramSel = (address & 0x80) == 0;
    mos6532Write(mos6532, ramSel, address, value);

    if(mos6532->ddr_b & 0x01){
      speakerWriteData(speaker, mos6532->ipr_b & 0x01);
    }
  }else if(address >= JC_PORT_6551 && address < (JC_PORT_6551+JC_PORT_6551_SIZE)){ // 6551 ACIA){
      mos6551Write(mos6551, address, value);
  }else{
    memoryJuniorComputerWriteAddress(mos6502, address, value);
  }
}

int juniorComputerCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo){

  int success;

  int i;

  mos6502 = mos6502create(juniorComputerReadAddress, juniorComputerWriteAddress, boardTimerCheckTimeout, machine->cpu.freqCPU);

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

  // use socat to loopback serial device
  // $> socat -d -d pty,link=/tmp/ttyJC0,raw,echo=0 pty,link=/tmp/ttyJC1,raw,echo=0
  // $> screen /tmp/ttyJC0 19200
  mos6551 = mos6551Create(mos6502, ACIA_TYPE_COM);
  mos6532 = mos6532Create();

  for(int i=0;i<machine->slotInfoCount;i++){
    if (0 == strcmp(machine->slotInfo[i].name, "jcFloppyGfxCard")){
      jcFgcCard = jcFloppyGfxCardCreate(machine, &machine->slotInfo[i]);
    }else if (0 == strcmp(machine->slotInfo[i].name, "jcIoCard")){
      jcIoCard = juniorComputerIoCardCreate(machine->slotInfo[i]);
    }
  }

  speaker = speakerCreate(boardGetMixer());
  //actionVolumeSetIo(0);

  vdpCreate(VDP_JC, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000, JC_PORT_K4+0x08);

  //register cpu hook
  hookexternal(jcInstructionCb);

  success = machineInitialize(machine, &jcRam, &jcRamSize, &jcRamStart);
  if (success) {
  //         success = boardInsertExternalDevices();
  }

  mos6532Reset(mos6532, 0);
  mos6502Reset(mos6502, 0);
  mos6551Reset(mos6551, 0);
  jcFloppyGfxCardReset(jcFgcCard);
  juniorComputerIoCardReset(jcIoCard);

  if (!success) {
    destroy();
  }

  return success;
}
