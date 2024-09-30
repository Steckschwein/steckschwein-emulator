#include "Speaker.h"
#include "IoPort.h"
//#include "SaveState.h"
#include "DebugDeviceManager.h"
//#include "Language.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <limits.h>


#define FB_BBCMICRO  0x0005
#define FB_SC3000    0x0006
#define FB_SEGA      0x0009
#define FB_COLECO    0x0003

#define SRW_SEGA     16
#define SRW_COLECO   15

#define VOL_TRUNC    0
#define VOL_FULL     0

#define SR_INIT       0x4000
#define PSG_CUTOFF    0x6

#define DELTA_CLOCK  ((float)3579545 / 16 / 44100)

struct Speaker {
    /* Framework params */
    Mixer* mixer;
    Int32  handle;
    Int32  debugHandle;

    /* Audio buffer */
    float buffer[AUDIO_MONO_BUFFER_SIZE];
};


static Int32* speakerSync(void* ref, UInt32 count);


void speakerLoadState(Speaker* speaker)
{
  /*
    SaveState* state = saveStateOpenForRead("speaker");
    char tag[32];
    int i;

    speaker->latch            = saveStateGet(state, "latch",           0);
    speaker->shiftReg         = saveStateGet(state, "shiftReg",        0);
    speaker->noiseFreq        = saveStateGet(state, "noiseFreq",       1);

    speaker->ctrlVolume       = saveStateGet(state, "ctrlVolume",      0);
    speaker->oldSampleVolume  = saveStateGet(state, "oldSampleVolume", 0);
    speaker->daVolume         = saveStateGet(state, "daVolume",        0);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        speaker->regs[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "toneFrequency%d", i);
        speaker->toneFrequency[i] = saveStateGet(state, tag, 0);

        sprintf(tag, "toneFlipFlop%d", i);
        speaker->toneFlipFlop[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
    */
}

void speakerSaveState(Speaker* speaker)
{
  /*
    SaveState* state = saveStateOpenForWrite("speaker");
    char tag[32];
    int i;

    saveStateSet(state, "latch",           speaker->latch);
    saveStateSet(state, "shiftReg",        speaker->shiftReg);
    saveStateSet(state, "noiseFreq",       speaker->noiseFreq);

    saveStateSet(state, "ctrlVolume",      speaker->ctrlVolume);
    saveStateSet(state, "oldSampleVolume", speaker->oldSampleVolume);
    saveStateSet(state, "daVolume",        speaker->daVolume);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        saveStateSet(state, tag, speaker->regs[i]);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "toneFrequency%d", i);
        saveStateSet(state, tag, speaker->toneFrequency[i]);

        sprintf(tag, "toneFlipFlop%d", i);
        saveStateSet(state, tag, speaker->toneFlipFlop[i]);

        speaker->toneInterpol[i] = 0;
    }

    speaker->clock = 0;

    saveStateClose(state);
    */
}

static void getDebugInfo(Speaker* speaker, DbgDevice* dbgDevice)
{
    DbgRegisterBank* regBank;
    int i;

    regBank = dbgDeviceAddRegisterBank(dbgDevice, "Speaker"/* langDbgRegs()*/, 8);
}

void speakerDestroy(Speaker* speaker)
{
    debugDeviceUnregister(speaker->debugHandle);
    mixerUnregisterChannel(speaker->mixer, speaker->handle);
    free(speaker);
}

void speakerReset(Speaker* speaker)
{
    Speaker* p = speaker;
}


typedef struct {
  float current_step;
  float step_size;
  float volume;
} oscillator;

#define	PI 3.14159265358979323846
const int SAMPLE_RATE = 44100;

oscillator* oscillate(float rate, float volume) {
  oscillator* o = calloc(1, sizeof(oscillator));
  o->current_step = 0,
  o->volume = volume,
  o->step_size = (2 * PI) / rate;
  return o;
}

float next(oscillator *os) {
  os->current_step += os->step_size;
  return sinf(os->current_step) * os->volume;
}

oscillator* A4_oscillator;

Speaker* speakerCreate(Mixer* mixer)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Speaker* speaker = (Speaker*)calloc(1, sizeof(Speaker));

    speaker->mixer = mixer;

    speaker->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_MSXAUDIO, 0, speakerSync, NULL, speaker);
    speaker->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, "Speaker", &dbgCallbacks, speaker);

    speakerReset(speaker);

    float A4_freq = (float)SAMPLE_RATE / 440.00f;
    A4_oscillator = oscillate(A4_freq, 0.8f);

    return speaker;
}

UInt8 pin;

void speakerWriteData(Speaker* speaker, UInt8 data)
{
    Speaker* p = speaker;

    pin = data;
    mixerSync(p->mixer);
}

static Int32* speakerSync(void* ref, UInt32 count)
{
    Speaker* p = (Speaker*)ref;

    UInt32 j;

    for(j = 0; j < count; j++) {

      float v = next(A4_oscillator);

      p->buffer[j] = pin<<5;
    }

    return p->buffer;
}
