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

#include "juniorComputerFloppyGfxCard.h"

JuniorComputerFGCard* juniorComputerFGCardCreate(){

  JuniorComputerFGCard *card = calloc(1, sizeof(JuniorComputerFGCard));
  return card;
}


const UInt8 FGC_MAGIC[] = { 0x99, 0x38, 0x76, 0x5B };  // Magic number of Floppy-/Graphics-Controller

UInt8 jcFgcRead(JuniorComputerFGCard *card, UInt16 address){
  if((address & 0x3ff) >= 0x3fc){
    return 0xff; //FGC_MAGIC[address & 0x03];
  }
  return ioPortRead(card, address);
}

void jcFgcWrite(JuniorComputerFGCard *card, UInt16 address, UInt8 value){
  ioPortWrite(card, address, value);
}

void juniorComputerFGCardDestroy(JuniorComputerFGCard* card){

}

void juniorComputerFGCardReset(JuniorComputerFGCard* card){

}
