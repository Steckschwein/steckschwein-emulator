#ifndef MOS6502_H
#define MOS6502_H
#include "MsxTypes.h"

#define ENABLE_BREAKPOINTS

typedef UInt32 SystemTime;

typedef void (*MOS6502TimerCb)(void*);

typedef struct{
	SystemTime    	systemTime;       /* Current system time             */
	UInt32        	vdpTime;          /* Time of last access to VDP  */
    int           	terminate;        /* Termination flag                */
    SystemTime    	timeout;          /* User scheduled timeout          */
    MOS6502TimerCb	timerCb;
    int           intState;         /* Sate of interrupt line          */

#ifdef ENABLE_BREAKPOINTS
    int           breakpointCount;  /* Number of breakpoints set       */
    char          breakpoints[0x10000];
#endif

} MOS6502;

#define INT_LOW   0
#define INT_HIGH  1

MOS6502* mos6502create(MOS6502TimerCb timerCb);

void mos6502Reset(MOS6502* mos6502, UInt32 cpuTime);
void mos6502SetInt(MOS6502* mos6502);
void mos6502Execute(MOS6502* mos6502);
void mos6502ClearInt(MOS6502* mos6502);
void mos6502SetTimeoutAt(MOS6502* mos6502, SystemTime time);
void mos6502SetBreakpoint(MOS6502* mos6502, UInt16 address);
void mos6502ClearBreakpoint(MOS6502* mos6502, UInt16 address);
void mos6502StopExecution(MOS6502* mos6502);
UInt32 mos6502GetTimeTrace(MOS6502* mos6502, int offset);
void mos6502Destroy(MOS6502* mos6502);

#endif
