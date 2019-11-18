#ifndef STECKSCHWEIN_H
#define STECKSCHWEIN_H

#include "Board.h"

typedef UInt32 SystemTime;

typedef struct{
   SystemTime systemTime;
} MOS6502;

int steckSchweinCreate(VdpSyncMode vdpSyncMode,
              BoardInfo* boardInfo);

#endif
