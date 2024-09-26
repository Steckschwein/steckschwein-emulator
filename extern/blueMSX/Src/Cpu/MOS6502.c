#include "MOS6502.h"
#include "EmulatorDebugger.h"
#include "cpu/fake6502.h"
#include "glue.h"

#include "Board.h"

#include <stddef.h>

#define VDP_DELAY 2 // wait states aka clockticks

unsigned char vdp_delay = VDP_DELAY;

#define delayVdpIO(mos6502, port) {                             \
  if ((port & 0xf20) == 0x220) {                                \
    mos6502->systemTime = 6 * ((mos6502->systemTime + 5) / 6);  \
    if (mos6502->systemTime - mos6502->vdpTime < vdp_delay)     \
        mos6502->systemTime = mos6502->vdpTime + vdp_delay;     \
    mos6502->vdpTime = mos6502->systemTime;                     \
  }                                                             \
}


void write6502(UInt16 address, UInt8 value) {
  mos6502->writeAddress(mos6502, address, value);
}

UInt8 read6502Debug(UInt16 address, bool dbg, UInt16 bank){
  return mos6502->readAddress(mos6502, address, dbg);
}

UInt8 read6502(UInt16 address){
  return mos6502->readAddress(mos6502, address, false);
}

MOS6502* mos6502create(
    MOS6502ReadCb readAddress,
    MOS6502WriteCb writeAddress,
    MOS6502TimerCb timerCb) {

  MOS6502 *mos6502 = calloc(1, sizeof(MOS6502));
  mos6502->systemTime = 0;
  mos6502->terminate = 0;
  mos6502->readAddress = readAddress;
  mos6502->writeAddress = writeAddress;
  mos6502->timerCb = timerCb;
  mos6502->intState = INT_HIGH;
  mos6502->nmiState = INT_HIGH;
  mos6502->nmiEdge = 0;
  mos6502->frequency = 3579545 * 3; // ~10.7 Mhz

  return mos6502;
}


UInt8 readAddress(MOS6502* mos6502, UInt16 port) {
    UInt8 value;

//    delayPreIo(mos6502);

    delayVdpIO(mos6502, port);

    value = ioPortRead(mos6502, port);
    // delayPostIo(mos6502);

    return value;
}

void writeAddress(MOS6502* mos6502, UInt16 port, UInt8 value) {
//    delayPreIo(mos6502);

    delayVdpIO(mos6502, port);
    //mos6502->writeIoPort(mos6502->ref, port, value);
    ioPortWrite(mos6502, port, value);
//    delayPostIo(mos6502);

#ifdef ENABLE_WATCHPOINTS
    if (mos6502->watchpointIoCb != NULL) {
        mos6502->watchpointIoCb(mos6502->ref, port, value);
    }
#endif

}

void mos6502Reset(MOS6502 *mos6502, UInt32 cpuTime) {
  reset6502();
}

void mos6502Execute(MOS6502 *mos6502) {

  static SystemTime lastRefreshTime = 0;

  //6 * 3579545 / 10.0000 = 2 - msx emulator impl. clock speed is bound to the 21.47 Mhz VDP clock, so we have to calculate clockticks (delay) upon board freq. and CPU frequ.
  int freqAdjust = boardFrequency() / (mos6502->frequency - 1);

  vdp_delay = freqAdjust * VDP_DELAY;

  while (!mos6502->terminate) {

    if ((Int32) (mos6502->timeout - mos6502->systemTime) <= 0) {
      if (mos6502->timerCb != NULL) {
        mos6502->timerCb(mos6502->ref);
      }
    }
    if (mos6502->systemTime - lastRefreshTime > 102 * freqAdjust) {
        lastRefreshTime = mos6502->systemTime;
        mos6502->systemTime += 15 * freqAdjust;
    }

#ifdef ENABLE_BREAKPOINTS
    if (mos6502->breakpointCount > 0) {
            if (mos6502->breakpoints[pc]) {
              DEBUGBreakToDebugger();
//                if (mos6502->breakpointCb != NULL) {
//                  mos6502->breakpointCb(mos6502->ref, pc);
//                    if (mos6502->terminate) {
//                        break;
//                    }
//                }
      }
    }
#endif
    /* If it is NMI... */
    if (mos6502->nmiEdge) {
      mos6502->nmiEdge = 0;
      nmi6502();
    }else if(mos6502->intState == INT_LOW){
      irq6502();
    }
    uint32_t cycles = step6502();
    mos6502->systemTime += (cycles * freqAdjust);
    DEBUG ("mos6502Execute %p %x\n", mos6502, mos6502->systemTime);
  }
}

void mos6502SetInt(MOS6502 *mos6502) {
  DEBUG ("mos6502SetInt %p\n", mos6502);
  mos6502->intState = INT_LOW;
}

void mos6502ClearInt(MOS6502 *mos6502) {
  DEBUG ("mos6502ClearInt %p\n", mos6502);
  mos6502->intState = INT_HIGH;
}

void mos6502SetNmi(MOS6502* mos6502){
  DEBUG ("mos6502SetNmi %p\n", mos6502);
  if (mos6502->nmiState == INT_HIGH) {
    mos6502->nmiEdge = 1;
  }
  mos6502->nmiState = INT_LOW;
}

void mos6502ClearNmi(MOS6502* mos6502){
  DEBUG ("mos6502ClearNmi %p\n", mos6502);
  mos6502->nmiState = INT_HIGH;
}

void mos6502SetTimeoutAt(MOS6502 *mos6502, SystemTime time) {
//  DEBUG ("mos6502SetTimeoutAt %p\n", mos6502);
  mos6502->timeout = time;
}

void mos6502SetBreakpoint(MOS6502 *mos6502, UInt16 address) {
  DEBUG ("mos6502SetBreakpoint %p\n", mos6502);
#ifdef ENABLE_BREAKPOINTS
  if (mos6502->breakpoints[address] == 0) {
    mos6502->breakpoints[address] = 1;
    mos6502->breakpointCount++;
  }
#endif
}
void mos6502ClearBreakpoint(MOS6502 *mos6502, UInt16 address) {
#ifdef ENABLE_BREAKPOINTS
  if (mos6502->breakpoints[address] != 0) {
    mos6502->breakpointCount--;
    mos6502->breakpoints[address] = 0;
  }
#endif
}

void mos6502StopExecution(MOS6502 *mos6502) {
  mos6502->terminate = 1;
}

UInt32 mos6502GetTimeTrace(MOS6502 *mos6502, int offset) {
  return mos6502->systemTime;
}

void mos6502Destroy(MOS6502 *mos6502) {
  free(mos6502);
}
