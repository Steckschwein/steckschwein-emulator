// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "via.h"
#include "uart.h"
#include "ym3812.h"

#include "memorySteckschwein.h"

UInt8 ctrl_port[] = {0,0,0,0};

UInt8 *ram;
UInt8 *rom;

void *cpu;

#define BANK_SIZE 14 // 14 bit $4000

#define RAM_SIZE 512*1024
#define ROM_SIZE 32*1024

void memorySteckschweinCreate(void *cpuRef, RomImage* romImage) {

  cpu = cpuRef;

  ram = malloc(RAM_SIZE);
  rom = malloc(ROM_SIZE);

  if(romImage && romImage->romPath){
    FILE *f = fopen(romImage->romPath, "rb");
    if (!f) {
      fprintf(stderr, "Cannot open %s!\n", romImage->romPath);
      exit(1);
    }
    int size = fread(rom, 1, ROM_SIZE, f);
    fclose(f);
    if(size < ROM_SIZE){
      fprintf(stderr, "WARN: Given rom image %s is smaller then installed ROM. Expected size 0x%04x, but was 0x%04x!\n", romImage->romPath, ROM_SIZE, size);
    }
  }

  ctrl_port[0] = 0x00;
#ifdef SSW2_0
  ctrl_port[1] = 0x01;
  ctrl_port[2] = 0x80;
  ctrl_port[3] = 0x81;
#endif
}

UInt8 memory_get_ctrlport(UInt16 address) {
  return ctrl_port[address & 0x03];
}

void memory_destroy() {
  free(ram);
  free(rom);
}

static UInt8 *get_address(UInt16 address, bool debugOn){

  UInt8 *p;
  UInt32 mem_size;

  UInt8 reg = (address >> BANK_SIZE) & sizeof(ctrl_port)-1;
  if((ctrl_port[reg] & 0x80) == 0){  // RAM/ROM)
    p = ram;
    mem_size = RAM_SIZE;
  }else{
    p = rom;
    mem_size = ROM_SIZE;
  }

  UInt32 extaddr = ((ctrl_port[reg] & ((mem_size >> BANK_SIZE)-1)) << BANK_SIZE) | (address & ((1<<BANK_SIZE)-1));

  if(!debugOn){//skip if called from debugger
      DEBUG ("address: a: $%4x r: $%2x/$%2x sz: $%x ext: $%x", address, reg, ctrl_port[reg], mem_size, extaddr);
  }

  return &p[extaddr & (mem_size-1)];
}

static UInt8 real_read6502(UInt16 address, bool debugOn, UInt8 bank) {

  if (address >= 0x0200) {// I/O
    if (address < 0x210) // UART at $0200
    {
      return uart_read(address & 0xf);
    } else if (address < 0x0220) // VIA at $0210
    {
      return via1_read(address & 0xf);
    } else if (address < 0x0230) // VDP at $0220
    {
      return ioPortRead(NULL, address);
    } else if (address < 0x0240) // latch/cpld regs at $0230
    {
      return memory_get_ctrlport(address);
    } else if (address < 0x0250) // OPL2 at $0240
    {
      return ioPortRead(NULL, address);
    }
  }
#ifdef SSW2_0

  UInt8 *p = get_address(address, debugOn);

  UInt8 value = *p;

  if(!debugOn){//called from render
      DEBUG (" read v: %2x\n", value);
  }

  return value;

#else
  DEBUG("read6502 %x %x\n", address, bank);

  if (address < 0xe000 || (ctrl_port[0] & 1)) {
    return ram[address]; // RAM
  }
  /* bank select upon ctrl_port[0] - see steckos/asminc/system.inc
    BANK_SEL0 = 0010
    BANK_SEL1 = 0100
    BANK0 = 0000
    BANK1 = 0010
    BANK2 = 0100
    BANK3 = 0110
    */
  return rom[(address & 0x1fff) | ((ctrl_port[0] & 0x06) << 12)];
#endif
}

void memorySteckschweinWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value){

  if (address >= 0x0200) { // I/O
    if (address < 0x210) // UART at $0200
    {
      uart_write(address & 0xf, value);
      return;
    } else if (address < 0x0220) // VIA at $0210
    {
      via1_write(address & 0xf, value);
      return;
    } else if (address < 0x0230) // VDP at $0220
    {
      ioPortWrite(NULL, address, value);
      return;
    } else if (address < 0x0240) // latch at $0x0230
    {
      ctrl_port[address &0x03] = value;
      DEBUG ("ctrl_port $%2x\n", ctrl_port[address &0x03]);
      return;
    } else if (address < 0x0250) // OPL2 at $0240
    {
      ioPortWrite(NULL, address, value);
      return;
    }
  }
#ifdef SSW2_0

  UInt8 *p = get_address(address, false);
  *p = value;

  DEBUG (" write v: $%2x\n", value);

#else
  DEBUG("write6502 %4x %2x\n", address, value);
  // Writes go to ram, regardless if ROM active or not
  ram[address] = value;
#endif
}

//
// saves the memory content into a file
//

void memory_save(FILE *f, bool dump_ram, bool dump_bank) {
  fwrite(ram, sizeof(UInt8), RAM_SIZE, f);
}

///
///
///

// Control the GIF recorder
void emu_recorder_set(gif_recorder_command_t command) {
  // turning off while recording is enabled
  if (command == RECORD_GIF_PAUSE && record_gif != RECORD_GIF_DISABLED) {
    record_gif = RECORD_GIF_PAUSED; // need to save
  }
  // turning on continuous recording
  if (command == RECORD_GIF_RESUME && record_gif != RECORD_GIF_DISABLED) {
    record_gif = RECORD_GIF_ACTIVE;    // activate recording
  }
  // capture one frame
  if (command == RECORD_GIF_SNAP && record_gif != RECORD_GIF_DISABLED) {
    record_gif = RECORD_GIF_SINGLE;    // single-shot
  }
}

//
// read/write emulator state (feature flags)
//
// 0: debugger_enabled
// 1: log_video
// 2: log_keyboard
// 3: echo_mode
// 4: save_on_exit
// 5: record_gif
// POKE $9FB3,1:PRINT"ECHO MODE IS ON":POKE $9FB3,0
void emu_write(UInt8 reg, UInt8 value) {
  bool v = value != 0;
  switch (reg) {
  case 0:
    isDebuggerEnabled = v;
    break;
  case 1:
    log_video = v;
    break;
  case 2:
    log_keyboard = v;
    break;
  case 3:
    echo_mode = v;
    break;
  case 4:
    save_on_exit = v;
    break;
  case 5:
    emu_recorder_set((gif_recorder_command_t) value);
    break;
  default:
//    printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
  }
}

UInt8 emu_read(UInt8 reg) {
  if (reg == 0) {
    return isDebuggerEnabled ? 1 : 0;
  } else if (reg == 1) {
    return log_video ? 1 : 0;
  } else if (reg == 2) {
    return log_keyboard ? 1 : 0;
  } else if (reg == 3) {
    return echo_mode;
  } else if (reg == 4) {
    return save_on_exit ? 1 : 0;
  } else if (reg == 5) {
    return record_gif;
  } else if (reg == 13) {
    return keymap;
  } else if (reg == 14) {
    return '1'; // emulator detection
  } else if (reg == 15) {
    return '6'; // emulator detection
  }
  //printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
  return -1;
}


//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever
UInt8 memorySteckschweinReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn) {
  return real_read6502(address, debugOn, 0);
}
