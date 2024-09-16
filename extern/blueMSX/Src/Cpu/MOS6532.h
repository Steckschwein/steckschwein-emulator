#include "MsxTypes.h"

// Define MOS 6532 RIOT memory layout
#define MOS6532_RAM_SIZE 128

// Define structure for MOS 6532 RIOT chip
typedef struct {
    UInt8 ram[MOS6532_RAM_SIZE];  // 128 bytes of internal RAM
    UInt8 port_a;         // Port A I/O
    UInt8 port_b;         // Port B I/O
    UInt16 timer;         // Timer
    int timer_running;    // Timer running flag
} MOS6532;


MOS6532* mos6532Create();

UInt8 mos6532Read(MOS6532* mos6532, UInt16 port);

void mos6532Write(MOS6532* mos6532, UInt16 port, UInt8 value);

UInt8 mos6532Destroy(MOS6532* mos6532);

void mos6532Reset(MOS6532 *mos6532, UInt32 cpuTime);

void mos6532Execute(MOS6532 *mos6532);

