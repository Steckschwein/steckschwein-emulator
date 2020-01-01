#ifndef _OPL2_H_
#define _OPL2_H_
#define OPL2_CLOCK_RATE 3579545
#define OPL2_SAMPLE_RATE 11025     // higher samplerates result in "out of memory"

#define FREQUENCY        3579545
#define SAMPLERATE       (FREQUENCY / 72)
#define TIMER_FREQUENCY  (4 * boardFrequency() / SAMPLERATE)

#define PORT_ADDR 0x240
#define PORT_DATA 0x241


#include <stdint.h>

#include "extern/blueMSX/Src/SoundChips/Fmopl.h"
#include "AudioMixer.h"
#include "Board.h"

/* Type definitions */
typedef struct YM3812 YM3812;

YM3812* ym3812Create();
UInt8 ym3812Read(YM3812* ym3812, UInt16 ioPort);
void ym3812Write(YM3812* ym3812, UInt16 ioPort, UInt8 value);

#endif
