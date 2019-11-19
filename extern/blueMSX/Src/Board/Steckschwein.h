#ifndef STECKSCHWEIN_H
#define STECKSCHWEIN_H

#include "Board.h"

typedef UInt32 SystemTime;

typedef void (*MOS6502TimerCb)(void*);

typedef struct{
	SystemTime    systemTime;       /* Current system time             */
	UInt32        vdpTime;          /* Time of last access to MSX vdp  */
    int           terminate;        /* Termination flag                */
    SystemTime    timeout;          /* User scheduled timeout          */
    MOS6502TimerCb	timerCb;

} MOS6502;

int steckSchweinCreate(VdpSyncMode vdpSyncMode,
              BoardInfo* boardInfo);

#endif
