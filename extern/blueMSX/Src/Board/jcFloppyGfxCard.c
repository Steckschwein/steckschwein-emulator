// MIT License
//
// Copyright (c) 2018 Thomas Woinke, Marko Lauke, www.steckschwein.de
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "jcFloppyGfxCard.h"
#include "Machine.h"
#include "8255A.h"
#include "VDP.h"
#include <error.h>

static PIA8255 *pia8255;
static UInt8 *rom;
static UInt16 fgcBaseAddress;
static UInt16 ioMask;

#define ROM_SIZE 32*1024

#define DRIVE_TYPE_FLOPPY 0
#define KBD_LANG_DE 1<<4

#define DIP_SWITCH_A DRIVE_TYPE_FLOPPY | KBD_LANG_DE
#define DIP_SWITCH_B 0

#define CONTROL_IO_PORTA 1<<4
#define CONTROL_IO_PORTB 1<<1

JcFloppyGfxCard* jcFloppyGfxCardCreate(Machine *machine, SlotInfo *slotInfo){

  if(!slotInfo->romPath){
    fprintf(stderr, "ERROR: Missing FGC rom image!\n");
    return NULL;
  }
  FILE* f = fopen(slotInfo->romPath, "rb");
  if (!f) {
    fprintf(stderr, "ERROR: Cannot open program image %s!\n", slotInfo->romPath);
    exit(1);
  }
  rom = malloc(ROM_SIZE);
  memset(rom, 0xff, ROM_SIZE);// init with 0xff
  int size = fread(rom, 1, ROM_SIZE, f);
  if(size != ROM_SIZE){
    fprintf(stdout, "ERROR: Load FGC rom image %s\n", strerror(error));
  }
  fprintf(stdout, "INFO: Load FGC rom image %s\n", slotInfo->romPath);
  fclose(f);

  JcFloppyGfxCard *card = calloc(1, sizeof(JcFloppyGfxCard));

  pia8255 = pia8255Create();

  fgcBaseAddress = slotInfo->address;
  ioMask = slotInfo->size-1;

  vdpCreate(VDP_JC, machine->video.vdpVersion, VDP_SYNC_AUTO, machine->video.vramSize / 0x4000, fgcBaseAddress+0x08); // VDP base address

  return card;
}

UInt8 jcFloppyGfxCardRead(JcFloppyGfxCard *card, UInt16 address){

  if(address >= fgcBaseAddress+0x08 && address < fgcBaseAddress+0x0c){
    return ioPortRead(card, address);
  }else if(address == fgcBaseAddress+0x0c){
    return (pia8255->ctrl & CONTROL_IO_PORTA) ? DIP_SWITCH_A : pia8255->portA;
  }else if(address == fgcBaseAddress+0x0d){
    return (pia8255->ctrl & CONTROL_IO_PORTB) ? DIP_SWITCH_B : pia8255->portB;
  } if(address == fgcBaseAddress+0x0e){
    return pia8255->portC;
  } if(address == fgcBaseAddress+0x0f){
    return pia8255->ctrl;
  }

  return rom[((pia8255->portC & 0x0f) << 10 | (address & ioMask))];
}

void jcFloppyGfxCardWrite(JcFloppyGfxCard *card, UInt16 address, UInt8 value){

  if(address >= fgcBaseAddress+0x08 && address < fgcBaseAddress+0x0c){
    ioPortWrite(card, address, value);
  }else if(address == fgcBaseAddress+0x0c){
    pia8255->portA = value;
  }else if(address == fgcBaseAddress+0x0d){
    pia8255->portB = value;
  } if(address == fgcBaseAddress+0x0e){
    pia8255->portC = value;
  } if(address == fgcBaseAddress+0x0f){
    pia8255->ctrl = value;
  }
}

void jcFloppyGfxCardDestroy(JcFloppyGfxCard* card){
  free(rom);
  free(pia8255);
}

void jcFloppyGfxCardReset(JcFloppyGfxCard* card){
  pia8255Reset(pia8255);
  pia8255->portA = 0x7f; //PA0-6 dip switches
  pia8255->portB = 0x3f; //PB0-5 pull up
  pia8255->portC = 0x0f; //PC0-3 pull up
}
