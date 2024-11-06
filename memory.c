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

#include "memory.h"

uint8_t ctrl_port[] = {0,0,0,0};

uint8_t *ram;
uint8_t *rom;

void *cpu;

#define DEVICE_EMULATOR (0x9fb0)

void memoryCreate(void *cpuRef, RomImage* romImage) {

  cpu = cpuRef;

  ram = malloc(RAM_SIZE);

  if(romImage){
    rom = romImage->image;
  }else{
    rom = malloc(ROM_SIZE);
  }

  ctrl_port[0] = 0x00;
#ifdef SSW2_0
  ctrl_port[1] = 0x01;
  ctrl_port[2] = 0x80;
  ctrl_port[3] = 0x81;
#endif
}

uint8_t memory_get_ctrlport(uint16_t address) {
  return ctrl_port[address & 0x03];
}

void memory_destroy() {
  free(ram);
  free(rom);
}

//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever
uint8_t read6502(uint16_t address) {
  return real_read6502(address, false, 0);
}

uint8_t *get_address(uint16_t address, bool debugOn){

  uint8_t *p;
  uint32_t mem_size;

  uint8_t reg = (address >> BANK_SIZE) & sizeof(ctrl_port)-1;
  if((ctrl_port[reg] & 0x80) == 0){  // RAM/ROM)
    p = ram;
    mem_size = RAM_SIZE;
  }else{
    p = rom;
    mem_size = ROM_SIZE;
  }

  uint32_t extaddr = ((ctrl_port[reg] & ((mem_size >> BANK_SIZE)-1)) << BANK_SIZE) | (address & ((1<<BANK_SIZE)-1));

  if(!debugOn){//skip if called from debugger
      DEBUG ("address: a: $%4x r: $%2x/$%2x sz: $%x ext: $%x", address, reg, ctrl_port[reg], mem_size, extaddr);
  }

  return &p[extaddr & (mem_size-1)];
}

uint8_t real_read6502(uint16_t address, bool debugOn, uint8_t bank) {

  if (address >= 0x0200) {// I/O
    if (address < 0x210) // UART at $0200
    {
      return uart_read(address & 0xf);
    } else if (address < 0x0220) // VIA at $0210
    {
      return via1_read(address & 0xf);
    } else if (address < 0x0230) // VDP at $0220
    {
      //return ioPortRead(NULL, address);
      return readPort(cpu, address);
    } else if (address < 0x0240) // latch/cpld regs at $0230
    {
      return memory_get_ctrlport(address);
    } else if (address < 0x0250) // OPL2 at $0240
    {
      //return ioPortRead(NULL, address);
      return readPort(cpu, address);
    // } else {
    //   return emu_read(address & 0xf);
    }
  }
#ifdef SSW2_0

  uint8_t *p = get_address(address, debugOn);

  uint8_t value = *p;

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

void write6502(uint16_t address, uint8_t value) {

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
      //ioPortWrite(NULL, address, value);
      writePort(cpu, address, value);
      return;
    } else if (address < 0x0240) // latch at $0x0230
    {
      ctrl_port[address &0x03] = value;
      DEBUG ("ctrl_port $%2x\n", ctrl_port[address &0x03]);
      return;
    } else if (address < 0x0250) // OPL2 at $0240
    {
      //ioPortWrite(NULL, address, value);
      writePort(cpu, address, value);
      return;
    }
  }

  UInt8 reg = (address >> BANK_SIZE) & sizeof(ctrl_port)-1;// register upon address
  if((ctrl_port[reg] & 0x80) == 0x80){  // RAM/ROM ?
    fprintf(stderr, "rom write at $%4x $%2x - ctrl reg $%04x $%2x, ignore\n", address, value, 0x230 + reg, ctrl_port[reg]);
    return;
  }

#ifdef SSW2_0

  uint8_t *p = get_address(address, false);
  *p = value;

  DEBUG (" write v: $%2x\n", value);

#else
  DEBUG("write6502 %4x %2x\n", address, value);

    /*
     if (address < 0x0310) {
     via1_write(address & 0xf, value);
     } else {
     emu_write(address & 0xf, value);
     }
     */
    // } else if (address < 0xe000) { // ram
    // ram[address] = value;
  // Writes go to ram, regardless if ROM active or not
  ram[address] = value;
#endif
}

//
// saves the memory content into a file
//

void memory_save(FILE *f, bool dump_ram, bool dump_bank) {
  fwrite(ram, sizeof(uint8_t), RAM_SIZE, f);
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
void emu_write(uint8_t reg, uint8_t value) {
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
    printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
  }
}

uint8_t emu_read(uint8_t reg) {
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
  printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
  return -1;
}
