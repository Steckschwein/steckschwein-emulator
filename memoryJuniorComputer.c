#include <sys/types.h>
#include <string.h>
#include <errno.h>

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

#define JC_RAM_SIZE 128*1024
#define JC_ROM_SIZE 32*1024  // 16*1024 8*1024

#define JC_ROM_BANK_SIZE 8*1024
// #define JC_ROM_A10_A13 0B0000  // A10-A13 rom bank select 8255A
#define JC_ROM_BANK_SEL 0         // A14 low / high ROM 8k each

#define JC_ROM_BANK_MASK (JC_ROM_BANK_SIZE-1) | JC_ROM_BANK_SEL<<14


static long getFilesize(FILE *file){
	long filesize = -1L;
	if(fseek(file, 0L, SEEK_END) == EOF){
		fprintf(stderr, "error fseek %s\n", strerror(errno));
	}else{
		filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
	}
	return filesize;
}

void memoryJuniorComputerCreate(void *cpuRef, RomImage* romImage) {

  ram = calloc(1, JC_RAM_SIZE);
  rom = calloc(1, JC_ROM_SIZE);

  if(romImage && romImage->romPath){
    FILE* f = fopen(romImage->romPath, "rb");
    if (!f) {
      fprintf(stderr, "Cannot open %s!\n", romImage->romPath);
      exit(1);
    }
    int size = fread(rom, 1, JC_ROM_SIZE, f);
    fclose(f);
    if(size < JC_ROM_SIZE){
      fprintf(stderr, "WARN: Given rom image %s is smaller then installed ROM. Expected size 0x%04x, but was 0x%04x!\n", romImage->romPath, JC_ROM_SIZE, size);
    }
  }
}


void memoryJuniorComputerDestroy() {
  free(ram);
  free(rom);
}

static uint8_t *get_address(uint16_t address, bool debugOn){

  uint8_t *p;
  uint32_t addrMask;

  // RAM/ROM
  if(address >= 0xe000 ||
     address >= 0x1c00 && address < 0x2000
    ){
    p = rom;
    addrMask = JC_ROM_BANK_MASK;
  }else{
    p = ram;
    addrMask = JC_RAM_SIZE-1;
  }

  uint32_t memoryAddr = address & addrMask;

  if(!debugOn){//skip if called from debugger
      DEBUG ("address: a: 0x%4x sz: 0x%x ext: 0x%x", address, addrMask, memoryAddr);
  }

  return &p[memoryAddr & addrMask];
}

static uint8_t read(UInt16 address, bool debugOn) {

  if (address >= 0x0800) {// I/O
    if (address < 0x0c00) // I/O K2
      return 0xff;
    else if (address < 0x1000) // I/O K3
      return 0xff;
    else if (address < 0x1400) // I/O K4
      return 0xff;
    else if (address >= 0x1600 && address < 0x1800) // 6551 ACIA
    {
      return 0xff;
    } else if (address < 0x1c00) // 6532 RIOT
    {
      return ioPortRead(NULL, address);
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

  if (address >= 0x0800) {// I/O
    if (address < 0x0c00) // I/O K2
      return 0xff;
    else if (address < 0x1000) // I/O K3
      return 0xff;
    else if (address < 0x1400) // I/O K4
      return 0xff;
    else if (address >= 0x1600 && address < 0x1800) // 6551 ACIA
    {
      return 0xff;
    } else if (address < 0x1c00) // 6532 RIOT
    {
      return ioPortWrite(NULL, address);
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
  fwrite(ram, sizeof(uint8_t), JC_RAM_SIZE, f);
}


UInt8 memoryJuniorComputerReadAddress(MOS6502* mos6502, UInt16 address, bool debugOn) {
  return read(address, debugOn);
}
