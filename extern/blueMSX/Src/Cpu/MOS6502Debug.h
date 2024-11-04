#ifndef MOS6502_DEBUG_H
#define MOS6502_DEBUG_H

#include "MsxTypes.h"
#include "MOS6502.h"

typedef struct MOS6502Debug MOS6502Debug;

void mos6502DebugCreate(MOS6502* mos6502);
void mos6502DebugDestroy();

UInt8 debugReadAddress(UInt16 port);

#endif
