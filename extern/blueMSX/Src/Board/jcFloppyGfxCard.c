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
#include "8255A.h"
#include "VDP.h"

static PIA8255 *pia8255;
static uint8_t *rom;

#define ROM_SIZE 32*1024

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
  memset(rom, 0xff, ROM_SIZE);
  int size = fread(rom, 1, ROM_SIZE, f);
  fprintf(stdout, "INFO: Load FGC rom image %s\n", slotInfo->romPath);
  fclose(f);

  JcFloppyGfxCard *card = calloc(1, sizeof(JcFloppyGfxCard));

  pia8255 = pia8255Create();

  vdpCreate(VDP_JC, machine->video.vdpVersion, VDP_SYNC_AUTO, machine->video.vramSize / 0x4000, slotInfo->address+0x08);

  return card;
}


const UInt8 FGC_MAGIC[] = { 0x99, 0x38, 0x76, 0x5B };  // Magic number of Floppy-/Graphics-Controller

UInt8 jcFloppyGfxCardRead(JcFloppyGfxCard *card, UInt16 address){
  if((address & 0x3ff) >= 0x3fc){
    return rom[address & 0x3ff];
  }
  return ioPortRead(card, address);
}

void jcFloppyGfxCardWrite(JcFloppyGfxCard *card, UInt16 address, UInt8 value){
  ioPortWrite(card, address, value);
}

void jcFloppyGfxCardDestroy(JcFloppyGfxCard* card){
  if(rom){
    free(rom);
  }
}

void jcFloppyGfxCardReset(JcFloppyGfxCard* card){

}
