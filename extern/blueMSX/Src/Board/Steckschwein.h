#ifndef STECKSCHWEIN_H
#define STECKSCHWEIN_H

#include "Board.h"
#include "VDP.h"

#define STECKSCHWEIN_PORT_OPL 0x240

int steckSchweinCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo);

#endif
