#ifndef JUNIOR_COMPUTER_H
#define JUNIOR_COMPUTER_H

#include "Board.h"
#include "VDP.h"

#define JC_PORT_K2 0x0800
#define JC_PORT_K3 0x0c00
#define JC_PORT_K4 0x1000

#define JC_PORT_SIZE 1024

#define JC_PORT_6551 0x1600
#define JC_PORT_6551_SIZE 512

#define JC_PORT_6532_SIZE 1024
#define JC_PORT_6532 0x1800

int juniorComputerCreate(Machine* machine, VdpSyncMode vdpSyncMode, BoardInfo* boardInfo);

#endif
