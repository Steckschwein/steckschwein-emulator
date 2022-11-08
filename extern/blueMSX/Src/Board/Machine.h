#ifndef MACHINE_H
#define MACHINE_H

#include "VDP.h"

typedef enum {
    BOARD_UNKNOWN       = -1,
    BOARD_1_0           = 0x0100 + 0x00,
    BOARD_2_0           = 0x0200 + 0x00,
    BOARD_MASK          = 0xff00
} BoardType;

typedef struct {
    char name[64];
    struct {
        BoardType type;
    } board;
    struct {
        VdpVersion vdpVersion;
        int vramSize;
        int extVramSize;
    } video;
    struct {
        int enable;
        int batteryBacked;
    } cmos;
    struct {
        UInt32 freqCPU;
        UInt32 freqSPI;
    } cpu;
    int isZipped;
    char *zipFile;
} Machine;

Machine* machineCreate(const char* machineName);
void machineDestroy(Machine* machine);
void machineUpdate(Machine* machine);

#endif
