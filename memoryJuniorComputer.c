#include <sys/types.h>
#include <string.h>

#include "memoryJuniorComputer.h"

typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    UInt32 romMask;
    int romMapper[4];
} MemoryJuniorComputer;

#define RAM_SIZE 64*1024
#define ROM_SIZE 8*1024

void memoryJuniorComputerCreate(void *cpuRef, RomImage* romImage) {

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
}

static void destroy() {
  free(ram);
  free(rom);
}

//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever
static uint8_t read6502(uint16_t address) {
  return real_read6502(address, false, 0);
}

static uint8_t *get_address(uint16_t address, bool debugOn){

  uint8_t *p;
  uint32_t mem_size;

  if(address >= 0xe000 ||
    (address >= 0x1c00 && address < 0x2000)
    ){  // RAM/ROM)
    p = rom;
    mem_size = ROM_SIZE;
  }else{
    p = ram;
    mem_size = RAM_SIZE;
  }

  uint32_t memoryAddr = address & (mem_size-1);

  if(!debugOn){//skip if called from debugger
      DEBUG ("address: a: 0x%4x sz: 0x%x ext: 0x%x", address, mem_size, memoryAddr);
  }

  return &p[memoryAddr & (mem_size-1)];
}

static uint8_t read(MemoryJuniorComputer *ref, uint16_t address, bool debugOn) {

  if (address >= 0x0200) {// I/O
    if (address < 0x210) // UART at $0200
    {
      return uart_read(address & 0xf);
    } else if (address < 0x0220) // VIA at $0210
    {
      return via1_read(address & 0xf);
    } else if (address < 0x0230) // VDP at $0220
    {
//      return readPort(cpu, address);
    } else if (address < 0x0250) // OPL2 at $0240
    {
  //    return readPort(cpu, address);
    // } else {
    //   return emu_read(address & 0xf);
    }
  }
  uint8_t *p = get_address(address, debugOn);

  uint8_t value = *p;

  if(!debugOn){//called from render
      DEBUG (" read v: %2x\n", value);
  }

  return value;
}

static void write(MemoryJuniorComputer *ref, uint16_t address, uint8_t value) {

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
      //writePort(cpu, address, value);
      return;
    } else if (address < 0x0250) // OPL2 at $0240
    {
//      writePort(cpu, address, value);
      return;
    }
  }

  uint8_t *p = get_address(address, false);
  *p = value;

  DEBUG (" write v: $%2x\n", value);
}

//
// saves the memory content into a file
//
static void savestate(FILE *f, bool dump_ram, bool dump_bank) {
  fwrite(ram, sizeof(uint8_t), RAM_SIZE, f);
}

///
///
///

// Control the GIF recorder
static void emu_recorder_set(gif_recorder_command_t command) {
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
static void emu_write(uint8_t reg, uint8_t value) {
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
    printf("WARN: Invalid register %x\n", reg);
  }
}

static uint8_t emu_read(uint8_t reg) {
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
  printf("WARN: Invalid register %x\n", reg);
  return -1;
}
