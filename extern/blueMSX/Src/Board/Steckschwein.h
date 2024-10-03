#ifndef STECKSCHWEIN_H
#define STECKSCHWEIN_H

#include "Board.h"
#include "VDP.h"

// i/o related
#define STECKSCHWEIN_PORT_VDP 0x220
#define STECKSCHWEIN_PORT_OPL 0x240

int steckSchweinCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo);

#endif
