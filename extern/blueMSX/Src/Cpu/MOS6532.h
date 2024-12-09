#ifndef MOS6532_H_
#define MOS6532_H_

#include "MsxTypes.h"

// Define MOS 6532 RIOT memory layout
#define MOS6532_RAM_SIZE 128

// Define structure for MOS 6532 RIOT chip
typedef struct {
    UInt8 ram[MOS6532_RAM_SIZE];  // 128 bytes of internal RAM
    UInt8 ipr_a;         // internal peripheral register A I/O
    UInt8 ddr_a;         // data direction ipr A I/O

    UInt8 ipr_b;         // internal peripheral register B I/O
    UInt8 ddr_b;         // data direction ipr B I/O

    UInt8 icr;          // interrupt control register - bit 7 (timer), bit 6 (PA7)
    UInt8 ifr;          // interrupt flag register
    UInt8 edc;          // edge detect register

    UInt8 timer;         // Timer
    UInt8 timerLatch;    // Timer

    UInt16 timerDiv;
    UInt16 cnt;

    bool timer_running;    // Timer running flag
} MOS6532;


MOS6532* mos6532Create();

UInt8 mos6532Read(MOS6532* mos6532, bool ramSel, UInt16 address, bool debugOn);

void mos6532Write(MOS6532* mos6532, bool ramSel, UInt16 address, UInt8 value);

UInt8 mos6532Destroy(MOS6532* mos6532);

void mos6532Reset(MOS6532 *mos6532, UInt32 cpuTime);

void mos6532Execute(MOS6532 *mos6532);

#endif