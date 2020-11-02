#include "MOS6502.h"
#include "cpu/fake6502.h"
#include "glue.h"

#include <stddef.h>

MOS6502* mos6502create(MOS6502TimerCb timerCb) {
	MOS6502 *mos6502 = malloc(sizeof(MOS6502));
	mos6502->systemTime = 0;
	mos6502->terminate = 0;
	mos6502->timerCb = timerCb;

	mos6502Reset(mos6502, 0);

	return mos6502;
}

void mos6502Reset(MOS6502 *mos6502, UInt32 cpuTime) {
	reset6502();
}

void mos6502SetInt(MOS6502 *mos6502) {
	DEBUG ("mos6502SetInt %p\n", mos6502);
	mos6502->intState = INT_LOW;
	irq6502();
}

void mos6502Execute(MOS6502 *mos6502) {
    static SystemTime lastRefreshTime = 0;

	while (!mos6502->terminate) {

		if ((Int32) (mos6502->timeout - mos6502->systemTime) <= 0) {
			if (mos6502->timerCb != NULL) {
				mos6502->timerCb(NULL);
			}
		}
        if (mos6502->systemTime - lastRefreshTime > 222 * 3) {
            lastRefreshTime = mos6502->systemTime;
            mos6502->systemTime += 20 * 3;
        }

#ifdef ENABLE_BREAKPOINTS
		if (mos6502->breakpointCount > 0) {
            if (mos6502->breakpoints[pc]) {
            	DEBUGBreakToDebugger();
//                if (mos6502->breakpointCb != NULL) {
//                	mos6502->breakpointCb(mos6502->ref, pc);
//                    if (mos6502->terminate) {
//                        break;
//                    }
//                }
			}
		}
#endif

		step6502();
		mos6502->systemTime = clockticks6502;

	}
}

void mos6502ClearInt(MOS6502 *mos6502) {
	DEBUG ("mos6502ClearInt %p\n", mos6502);
	mos6502->intState = INT_HIGH;
}

void mos6502SetTimeoutAt(MOS6502 *mos6502, SystemTime time) {
//	DEBUG ("mos6502SetTimeoutAt %p\n", mos6502);
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
