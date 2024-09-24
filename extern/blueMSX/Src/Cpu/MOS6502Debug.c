#include "MOS6502Debug.h"
#include "DebugDeviceManager.h"
//#include "Language.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Board.h"


extern void debuggerTrace(const char* str);
extern void archTrap(UInt8 value);

struct MOS6502Debug {
    int debugHandle;
    MOS6502* mos6502;
};

static MOS6502Debug* mos6502Debug;

UInt8 debugReadAddress(UInt16 port){
  return mos6502Debug->mos6502->readAddress(mos6502Debug->mos6502, port, true);
}

static void getDebugInfo(MOS6502Debug* dbg, DbgDevice* dbgDevice)
{
  /*
    static UInt8 mappedRAM[0x10000];
    DbgRegisterBank* regBank;
    int freqAdjust;
    int i;

    for (i = 0; i < 0x10000; i++) {
        mappedRAM[i] = slotPeek(NULL, i);
    }

    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemVisible(), 0, 0, 0x10000, mappedRAM);

#ifdef ENABLE_CALLSTACK
    if (dbg->mos6502->callstackSize > 255) {
        static UInt16 callstack[0x100];
        int beginning = dbg->mos6502->callstackSize & 0xff;
        int reminder = 256 - beginning;
        memcpy(callstack, dbg->mos6502->callstack + beginning, reminder * sizeof(UInt16));
        memcpy(callstack + reminder, dbg->mos6502->callstack, beginning * sizeof(UInt16));
        dbgDeviceAddCallstack(dbgDevice, langDbgCallstack(), callstack, 256);
    }
    else {
        dbgDeviceAddCallstack(dbgDevice, langDbgCallstack(), dbg->mos6502->callstack, dbg->mos6502->callstackSize);
    }
#endif

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegsCpu(), 20);

    dbgRegisterBankAddRegister(regBank,  0, "A",  16, dbg->mos6502->regs.A);
    dbgRegisterBankAddRegister(regBank,  1, "X",  16, dbg->mos6502->regs.X);
    dbgRegisterBankAddRegister(regBank,  2, "Y",  16, dbg->mos6502->regs.Y);
    dbgRegisterBankAddRegister(regBank, 10, "SP",  16, dbg->mos6502->regs.SP);
    dbgRegisterBankAddRegister(regBank, 11, "PC",  16, dbg->mos6502->regs.PC.W);
*/
}


static int dbgWriteMemory(MOS6502Debug* dbg, char* name, void* data, int start, int size)
{
  /*
    UInt8* dataBuffer = data;
    int i;
    int rv = 1;

    if (strcmp(name, langDbgMemVisible()) || start + size > 0x10000) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        slotWrite(NULL, start + i, dataBuffer[i]);
        rv &= dataBuffer[i] == slotPeek(NULL, start + i);
    }

    return rv;
    */
}

static int dbgWriteRegister(MOS6502Debug* dbg, char* name, int regIndex, UInt32 value)
{
    switch (regIndex) {
    case 12: dbg->mos6502->regs.A = (UInt8)value; break;
    case 14: dbg->mos6502->regs.X = (UInt8)value; break;
    case 15: dbg->mos6502->regs.Y = (UInt8)value; break;
    case 10: dbg->mos6502->regs.SP = (UInt16)value; break;
    case 11: dbg->mos6502->regs.PC.W = (UInt16)value; break;
    }

    return 1;
}


static void breakpointCb(MOS6502Debug* dbg, UInt16 pc)
{
    boardOnBreakpoint(pc);
}

UInt8 readMem(void* ref, int address) {
  /*
    if (address < 0x10000) {
        return slotPeek(NULL, (UInt16)address);
    }
    */
    return 0xff;
}

static void watchpointMemCb(MOS6502Debug* dbg, UInt16 address, UInt8 value)
{
    tryWatchpoint(DBGTYPE_CPU, address, value, dbg, readMem);
}

static void watchpointIoCb(MOS6502Debug* dbg, UInt16 port, UInt8 value)
{
    tryWatchpoint(DBGTYPE_PORT, port, value, dbg, NULL);
}

static void debugCb(MOS6502Debug* dbg, int command, const char* data)
{
/*
    int slot, page, addr, rv;
    switch (command) {
    case ASDBG_TRACE:
        debuggerTrace(data);
        break;
    case ASDBG_SETBP:
        rv = sscanf(data, "%x %x %x", &slot, &page, &addr);
        if (rv == 3) {
            debuggerSetBreakpoint((UInt16)slot, (UInt16)page, (UInt16)addr);
        }
        break;
    }
    */
}

void trapCb(MOS6502* mos6502, UInt8 value)
{
    //archTrap(value);
}

void mos6502DebugCreate(MOS6502* mos6502)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, dbgWriteRegister, NULL };

    mos6502Debug = (MOS6502Debug*)malloc(sizeof(MOS6502Debug));
    mos6502Debug->mos6502 = mos6502;
//    dbg->debugHandle = debugDeviceRegister(DBGTYPE_CPU, langDbgDevZ80(), &dbgCallbacks, dbg);

    mos6502->debugCb           = debugCb;
    mos6502->breakpointCb      = breakpointCb;
    mos6502->trapCb            = trapCb;
    mos6502->watchpointMemCb   = watchpointMemCb;
    mos6502->watchpointIoCb    = watchpointIoCb;
}

void mos6502DebugDestroy()
{
    //debugDeviceUnregister(dbg->debugHandle);
    free(mos6502Debug);
}

