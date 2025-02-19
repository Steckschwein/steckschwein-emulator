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

    UInt16 value;

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

    saveStateClose(state);
    */
}

void speakerSaveState(Speaker* speaker)
{
  /*
    SaveState* state = saveStateOpenForWrite("speaker");
    int i;

    saveStateSet(state, "value",      speaker->value);

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
    speaker->value = 0;
}


typedef struct {
  float current_step;
  float step_size;
  float volume;
} oscillator;

#define	PI 3.14159265358979323846

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

Speaker* speakerCreate(Mixer* mixer)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Speaker* speaker = (Speaker*)calloc(1, sizeof(Speaker));

    speaker->mixer = mixer;

    speaker->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_IO, 0, speakerSync, NULL, speaker);
    speaker->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, "Speaker", &dbgCallbacks, speaker);

    speakerReset(speaker);

    return speaker;
}

void speakerWriteData(Speaker* speaker, UInt8 data)
{
    Speaker* p = speaker;
    p->value = (data & 0x01);

    mixerSync(p->mixer);
}

static Int32* speakerSync(void* ref, UInt32 count)
{
    Speaker* p = (Speaker*)ref;

    UInt32 j;

    for(j = 0; j < count; j++) {
      p->buffer[j] = p->value;
    }

    return p->buffer;
}
