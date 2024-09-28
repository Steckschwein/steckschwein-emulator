#ifndef JUNIOR_COMPUTER_H
#define JUNIOR_COMPUTER_H

#include "Board.h"
#include "VDP.h"

#define JC_PORT_VDP 0xc08
#define JC_PORT_SND 0x808

#define JC_PORT_6532_SIZE 1024
#define JC_PORT_6532 0x1800

int juniorComputerCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo);

#endif
