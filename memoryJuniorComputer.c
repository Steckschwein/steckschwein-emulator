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

#define RAM_SIZE 128*1024
#define ROM_SIZE 32*1024

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

static uint8_t read(UInt16 address, bool debugOn) {

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

void memoryJuniorComputerWriteAddress(MOS6502* mos6502, UInt16 address, UInt8 value) {

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


UInt8 memoryJuniorComputerReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn) {
  return read(address, debugOn);
}
