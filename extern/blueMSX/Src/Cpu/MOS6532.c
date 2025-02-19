#include "MOS6532.h"
#include "EmulatorDebugger.h"
#include "glue.h"
#include "Board.h"

#define IO_PORT_A 0
#define IO_PORT_DDRA 1
#define IO_PORT_B 1
#define IO_PORT_DDRB 3

#define TIMER 4

static uint16_t TIMER_CNT[] = {1, 8, 64, 1024};

// Initialize the MOS 6532
MOS6532* mos6532Create(UInt8 ipr_a_input, UInt8 ipr_b_input){

  MOS6532* mos6532 = (MOS6532*)calloc(1, sizeof(MOS6532));
  mos6532->ipr_a_input = ipr_a_input;
  mos6532->ipr_b_input = ipr_b_input;

  return mos6532;
}


// Emulate a read operation
UInt8 mos6532Read(MOS6532* mos6532, bool ramSel, UInt16 address, bool debugOn) {

  UInt8 addr = address & 0x7f;

  if (ramSel) { // RS Pin high
    return mos6532->ram[addr & (MOS6532_RAM_SIZE-1)];
  } else if ((addr & 0x04) == 0) { // register access
    UInt8 reg = addr & 0x3;
    if (reg == 0){ // A
      return (mos6532->ipr_a & mos6532->ddr_a) | (mos6532->ipr_a_input & (mos6532->ddr_a ^ 0xff));
    } else if (reg == 1){ // A DDR
      return mos6532->ddr_a;
    } else if (reg == 2){ // B
      return (mos6532->ipr_b & mos6532->ddr_b) | (mos6532->ipr_b_input & (mos6532->ddr_b ^ 0xff));
    } else if (reg == 3){ // B DDR
     return mos6532->ddr_b;
    }
  } else if ((addr & 0x5) == 0x4) { // timer access
    if(!debugOn){
      mos6532->icr = (mos6532->icr & 0x7f) | (addr & 0x8 << 4);// update icr upon A3
      mos6532->ifr &= 0x7f;
    }
    return mos6532->timer;
  } else if ((addr & 0x5) == 0x5) { // ifr
    UInt8 v = mos6532->ifr;
    if(!debugOn){
      mos6532->ifr &= 0xbf; // clear PA7
    }
    return v;
  }
  // Invalid read port
  return 0xff;
}

// Emulate a write operation
void mos6532Write(MOS6532* mos6532, bool ramSel, UInt16 address, UInt8 value) {

    UInt8 addr = address & 0x7f;

    if (ramSel) {  // RS Pin high
      mos6532->ram[addr & (MOS6532_RAM_SIZE-1)] = value;
    } else if ((addr & 0x14) == 0x14) { // timer access
      mos6532->timerDiv = TIMER_CNT[addr & 0x03];
      mos6532->cnt = mos6532->timerDiv;
      mos6532->timer = value;
      mos6532->ifr &= 0x7f;
      mos6532->icr = (mos6532->icr & 0x7f) | (addr & 0x8 << 4);// update icr upon A3
    } else if ((addr & 0x04) == 0) { // register access
      UInt8 reg = addr & 0x3;
      if (reg == 0){ // A
        mos6532->ipr_a = value;
      } else if (reg == 1){ // A DDR
        mos6532->ddr_a = value;
      } else if (reg == 2){ // B
        mos6532->ipr_b = value;
      } else if (reg == 3){ // B DDR
        mos6532->ddr_b = value;
      }
    } else if ((addr & 0x14) == 0x04) { // edge detect
        mos6532->edc = value;
        mos6532->icr = mos6532->icr | (addr & 0x2 << 5);// bit 6
    }
}

UInt8 mos6532Destroy(MOS6532* mos6532) {
  free(mos6532);
}

void mos6532Reset(MOS6532 *mos6532, UInt32 cpuTime) {
  mos6532->ipr_a = 0;
  mos6532->ipr_b = 0;
  mos6532->ddr_a = 0;
  mos6532->ddr_b = 0;
  mos6532->icr = 0;
  mos6532->ifr = 0;
  mos6532->cnt = 0;
  mos6532->timer = 0;
  mos6532->timerDiv = 0;
  mos6532->timer_running = false;
}


void mos6532Execute(MOS6532 *mos6532) {

  if(mos6532->cnt > 0){
    mos6532->cnt--;
  }else{
    if(mos6532->timer == 0){
      mos6532->ifr |= 0x80;
      if(mos6532->icr & 0x80){
        boardSetInt(0x4);
      }
    }
    if(mos6532->ifr & 0x80 == 0){
      mos6532->cnt = mos6532->timerDiv;
    }
    mos6532->timer--;
  }
}
