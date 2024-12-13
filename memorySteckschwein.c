// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "Steckschwein.h"
#include "memorySteckschwein.h"

static UInt8 ctrl_port[] = {0,0,0,0};

static UInt8 *ram;
static UInt8 *rom;

static UInt8 rom_cmd_byte;
static UInt8 rom_cmd;
static UInt8 toggle_bit;
static UInt32 toggle_bit_cnt;

#define BANK_SIZE 14 // 14 bit 16k

#ifdef SSW2_0
  #define RAM_SIZE (512*1024)
  #define ROM_SIZE (512*1024)
#else
  #define RAM_SIZE (64*1024)
  #define ROM_SIZE (32*1024)
#endif

void memorySteckschweinCreate(void *cpuRef, RomImage* romImage) {

  ram = malloc(RAM_SIZE);
  rom = malloc(ROM_SIZE);
  memset(rom, 0xff, ROM_SIZE);
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

  if (address >= STECKSCHWEIN_PORT_CPLD && address < STECKSCHWEIN_PORT_CPLD+STECKSCHWEIN_PORT_SIZE){ // latch/cpld regs at $0230
    return memory_get_ctrlport(address) & 0x9f;
  }

#ifdef SSW2_0

  UInt8 *p;
  UInt32 mem_size;

  UInt8 reg = (address >> BANK_SIZE) & sizeof(ctrl_port)-1;
  if((ctrl_port[reg] & 0x80) == 0){  // RAM/ROM ?
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

  if(p == rom && rom_cmd){
    switch(rom_cmd){
      case 0x90:
        UInt32 romAddress = address & 0x3fff | (ctrl_port[reg] & 0x1f) << BANK_SIZE;
        return romAddress & 0x01 ? 0x86 : 0x37; // TODO from config, externalize ROM logic
      case 0x80:
      case 0xa0:
        if(toggle_bit_cnt){
          toggle_bit_cnt--;
          toggle_bit^=1<<6;
        }else{
          toggle_bit|=1<<5;  // I/O5 indicate timeout
          rom_cmd = 0;
          rom_cmd_byte = 0;
        }
        return (p[extaddr & (mem_size-1)] & ~(1<<5|1<<6)) | toggle_bit;
      case 0xf0:
      default:
        rom_cmd = 0;
        rom_cmd_byte = 0;
    }
  }

  UInt8 value = p[extaddr & (mem_size-1)];

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

  if (address >= STECKSCHWEIN_PORT_CPLD && address < STECKSCHWEIN_PORT_CPLD+STECKSCHWEIN_PORT_SIZE){ // latch/cpld regs at $0230
    if (log_ctrl_port_writes)
    {
      log_write(address, value, "CTRL");
    }
    ctrl_port[address & 0x3] = value;
    return;
  }

#ifdef SSW2_0

  UInt8 reg = (address >> BANK_SIZE) & sizeof(ctrl_port)-1;// register upon address
  if((ctrl_port[reg] & 0x80) == 0x80){  // RAM/ROM ?

    UInt32 romAddress = ((ctrl_port[reg] & ((ROM_SIZE >> BANK_SIZE)-1)) << BANK_SIZE) | (address & ((1<<BANK_SIZE)-1));
    if (log_rom_writes)
    {
      fprintf(stdout, "ROM write at $%04x $%02x (rom address: $%05x) - ctrl reg $%04x $%2x, ignore\n", address, value, romAddress, STECKSCHWEIN_PORT_CPLD + reg, ctrl_port[reg]);
    }

    if(rom_cmd == 0xa0){// write command active?
      rom[romAddress] = value;
      toggle_bit = 0;
      toggle_bit_cnt = 0x1e; // set toggle counter to n times
      // TODO implement different rom write behaviour
    }else if(rom_cmd == 0x80 && value == 0x30 && rom_cmd_byte == 0x02){ // last cmd was 0x80 (program) and now (0x20) sector erase?
      if (log_rom_writes)
      {
        fprintf(stdout, "ROM sector erase address: $%06x\n", romAddress & 0x70000);
      }
      memset(rom + (romAddress & 0x70000), 0xff, 0x10000);//clear sector upon address
      toggle_bit = 0;
      toggle_bit_cnt = 0x80000; // set toggle counter
    }
    if(romAddress == 0x5555){// TODO FIXME avoids writing to 0x5555/0x2aaa
      rom_cmd_byte++;
      if(rom_cmd_byte == 3){
        if(rom_cmd == 0x80 && value == 0x10){ // chip erase command (0x10)
          if (log_rom_writes)
          {
            fprintf(stdout, "ROM chip erase.\n");
          }
          memset(rom, 0xff, ROM_SIZE);
          toggle_bit = 0;
          toggle_bit_cnt = 0x100000; // set toggle counter
          return;
        }
        rom_cmd = value;
        rom_cmd_byte = 0; // reset cmd sequence
        return;
      }
    }else if(romAddress == 0x2aaa){
      rom_cmd_byte++;
    }
    return;// valid rom access, exit
  }

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
