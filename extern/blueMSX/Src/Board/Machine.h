#ifndef MACHINE_H
#define MACHINE_H

#include "VDP.h"
#include "glue.h"

typedef enum {
    BOARD_UNKNOWN           = -1,
    BOARD_1_0               = 0x0100 + 0x00,
    BOARD_2_0               = 0x0200 + 0x00,
    BOARD_STECKSCHWEIN      = 0x0300,     // 6502 / TMS9918 / 64k Ram
    BOARD_STECKSCHWEIN_2_0  = 0x0400,     // 6502 / V9958 / 64/512k RAM
    BOARD_JC                = 0x0500, // junior computer
    BOARD_MASK              = 0xff00
} BoardType;

typedef struct {
    char name[64];
    struct {
        BoardType type;
    } board;
    struct {
        VdpVersion vdpVersion;
        int vramSize;
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

    RomImage *romImage;

} Machine;

Machine* machineCreate(RomImage* romImage, const char* machineName);
int machineInitialize(Machine* machine, UInt8** mainRam, UInt32* mainRamSize, UInt32* mainRamStart);
void machineDestroy(Machine* machine);
void machineUpdate(Machine* machine);

#endif
