#ifndef MOS6502_H
#define MOS6502_H
#include "MsxTypes.h"

#define ENABLE_BREAKPOINTS

typedef UInt32 SystemTime;

/*
 * Callback types
 */
typedef UInt8 (*MOS6502ReadCb)(void*, UInt16, bool);
typedef void  (*MOS6502WriteCb)(void*, UInt16, UInt8);
typedef void (*MOS6502TimerCb)(void*);
typedef void  (*MOS6502BreakptCb)(void*, UInt16);
typedef void  (*MOS6502DebugCb)(void*, int, const char*);
typedef void  (*MOS6502TrapCb)(void*, UInt8);

typedef struct {
    UInt8 A;
    UInt8 X;
    UInt8 Y;
    UInt8 P;

    UInt16 SP;
    UInt16 PC;
} CpuRegs;

typedef struct{
  SystemTime        systemTime;       /* Current system time             */
  UInt32            frequency;        /* CPU Frequency of 65C02 (in Hz)  */
  UInt32            vdpTime;          /* Time of last access to VDP  */
  int               terminate;        /* Termination flag                */
  SystemTime        timeout;          /* User scheduled timeout          */
  MOS6502ReadCb     readAddress;
  MOS6502WriteCb    writeAddress;
  MOS6502TimerCb    timerCb;
  MOS6502BreakptCb     breakpointCb;
  MOS6502DebugCb       debugCb;
  MOS6502WriteCb       watchpointMemCb;
  MOS6502WriteCb       watchpointIoCb;
  MOS6502TrapCb        trapCb;


  int               intState;         /* Sate of interrupt line          */
  int               nmiState;
  int               nmiEdge;
  CpuRegs           regs;
  void*             ref;              /* User defined pointer which is   */
                                    /* passed to the callbacks         */

#ifdef ENABLE_BREAKPOINTS
    int           breakpointCount;  /* Number of breakpoints set       */
    char          breakpoints[0x10000];
#endif

} MOS6502;

#define INT_LOW   0
#define INT_HIGH  1

UInt8 read6502Debug(UInt16 address, bool dbg, UInt16 bank);

MOS6502* mos6502create(MOS6502ReadCb readAddress, MOS6502WriteCb writeAddress, MOS6502TimerCb timerCb, UInt32 frequency);

void mos6502Reset(MOS6502* mos6502, UInt32 cpuTime);
void mos6502SetInt(MOS6502* mos6502);
void mos6502Execute(MOS6502* mos6502);
void mos6502ClearInt(MOS6502* mos6502);
void mos6502SetNmi(MOS6502* mos6502);
void mos6502ClearNmi(MOS6502* mos6502);
void mos6502SetTimeoutAt(MOS6502* mos6502, SystemTime time);
void mos6502SetBreakpoint(MOS6502* mos6502, UInt16 address);
void mos6502ClearBreakpoint(MOS6502* mos6502, UInt16 address);
void mos6502StopExecution(MOS6502* mos6502);
UInt32 mos6502GetTimeTrace(MOS6502* mos6502, int offset);
void mos6502Destroy(MOS6502* mos6502);

#endif
