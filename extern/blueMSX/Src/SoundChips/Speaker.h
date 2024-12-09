#ifndef SPEAKER_H
#define SPEAKER_H

#include "MsxTypes.h"
#include "AudioMixer.h"

/* Type definitions */
typedef struct Speaker Speaker;

/* Constructor and destructor */
Speaker* speakerCreate(Mixer* mixer);
void speekerDestroy(Speaker* speaker);

/* Reset chip */
void speakerReset(Speaker* speaker);

/* Register read/write methods */
void speakerWriteData(Speaker* speaker, UInt8 data);

void speakerLoadState(Speaker* speaker);
void speakerSaveState(Speaker* speaker);

#endif

