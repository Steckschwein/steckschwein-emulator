#include "ym3812.h"
#include "memory.h"

struct YM3812 {
	Mixer *mixer;
	Int32 handle;
	UInt32 rate;

	FM_OPL *opl;
//    MidiIO* ykIo;
	BoardTimer *timer1;
	BoardTimer *timer2;
	UInt32 timerValue1;
	UInt32 timerValue2;
	UInt32 timeout1;
	UInt32 timeout2;
	UInt32 timerRunning1;
	UInt32 timerRunning2;
	UInt8 address;
	// Variables used for resampling
	Int32 off;
	Int32 s1;
	Int32 s2;
	Int32 buffer[AUDIO_MONO_BUFFER_SIZE];
};

void ym3812TimerStart(void* ptr, int timer, int start)
{
    YM3812* ym3812 = (YM3812*)ptr;

    if (timer == 0) {
        if (start != 0) {
            if (!ym3812->timerRunning1) {
                UInt32 systemTime = boardSystemTime();
                UInt32 adjust = systemTime % TIMER_FREQUENCY;
                ym3812->timeout1 = systemTime + TIMER_FREQUENCY * ym3812->timerValue1 - adjust;
                boardTimerAdd(ym3812->timer1, ym3812->timeout1);
                ym3812->timerRunning1 = 1;
            }
        }
        else {
            if (ym3812->timerRunning1) {
                boardTimerRemove(ym3812->timer1);
                ym3812->timerRunning1 = 0;
            }
        }
    }
    else {
        if (start != 0) {
            if (!ym3812->timerRunning2) {
                UInt32 systemTime = boardSystemTime();
                UInt32 adjust = systemTime % (4 * TIMER_FREQUENCY);
                ym3812->timeout2 = systemTime + TIMER_FREQUENCY * ym3812->timerValue2 - adjust;
                boardTimerAdd(ym3812->timer2, ym3812->timeout2);
                ym3812->timerRunning2 = 1;
            }
        }
        else {
            if (ym3812->timerRunning2) {
                boardTimerRemove(ym3812->timer2);
                ym3812->timerRunning2 = 0;
            }
        }
    }
}

static void onTimeout1(void* ptr, UInt32 time)
{
    YM3812* ym3812 = (YM3812*)ptr;

    ym3812->timerRunning1 = 0;
    if (OPLTimerOver(ym3812->opl, 0)) {
        ym3812TimerStart(ym3812, 0, 1);
    }
}

static void onTimeout2(void* ptr, UInt32 time)
{
    YM3812* ym3812 = (YM3812*)ptr;

    ym3812->timerRunning2 = 0;
    if (OPLTimerOver(ym3812->opl, 1)) {
        ym3812TimerStart(ym3812, 1, 1);
    }
}

void ym3812TimerSet(void* ref, int timer, int count){
    YM3812 * ym3812 = (YM3812*)ref;

    if (timer == 0) {
        ym3812->timerValue1 = count;
    }
    else {
        ym3812->timerValue2 = count;
    }
}

static Int32* ym3812Sync(void* ref, UInt32 count)
{
    YM3812* y3812 = (YM3812*)ref;
    UInt32 i;

    for (i = 0; i < count; i++) {
        if(SAMPLERATE > y3812->rate) {
            y3812->off -= SAMPLERATE - y3812->rate;
            y3812->s1 = y3812->s2;
            y3812->s2 = YM3812UpdateOne(y3812->opl);
            if (y3812->off < 0) {
                y3812->off += y3812->rate;
                y3812->s1 = y3812->s2;
                y3812->s2 = YM3812UpdateOne(y3812->opl);
            }
            y3812->buffer[i] = (y3812->s1 * (y3812->off / 256) + y3812->s2 * ((SAMPLERATE - y3812->off) / 256)) / (SAMPLERATE / 256);
        }
        else
            y3812->buffer[i] = YM3812UpdateOne(y3812->opl);
    }

    return y3812->buffer;
}

void ym3812SetSampleRate(void* ref, UInt32 rate)
{
    YM3812* ym3812 = (YM3812*)ref;
    ym3812->rate = rate;
}

struct YM3812* ym3812Create(Mixer* mixer) {

	YM3812* ym3812 = (YM3812*)calloc(1, sizeof(YM3812));

    ym3812->mixer = mixer;
    ym3812->timerRunning1 = 0;
    ym3812->timerRunning2 = 0;

    ym3812->timer1 = boardTimerCreate(onTimeout1, ym3812);
    ym3812->timer2 = boardTimerCreate(onTimeout2, ym3812);

//    ym3812->ykIo = ykIoCreate();

    ym3812->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_MSXAUDIO, 0, ym3812Sync, ym3812SetSampleRate, ym3812);

    ym3812->opl = OPLCreate(OPL_TYPE_YM3812, FREQUENCY, SAMPLERATE, 256, ym3812);
    OPLSetOversampling(ym3812->opl, boardGetYM3812Oversampling());
    OPLResetChip(ym3812->opl);

	ym3812->rate = mixerGetSampleRate(mixer);

	ioPortRegister(PORT_ADDR, ym3812Read, ym3812Write, ym3812);
	ioPortRegister(PORT_DATA, ym3812Read, ym3812Write, ym3812);

    return ym3812;
}

void ym3812Reset(YM3812* ym3812)
{
    ym3812TimerStart(ym3812, 0, 0);
    ym3812TimerStart(ym3812, 1, 0);
    OPLResetChip(ym3812->opl);
    ym3812->off = 0;
    ym3812->s1 = 0;
    ym3812->s2 = 0;
}

UInt8 ym3812Read(YM3812* ym3812, UInt16 ioPort) {
//	printf("opl2_read %x\n", ioPort);
    switch (ioPort & 1) {
    case 0:
        return (UInt8)OPLRead(ym3812->opl, 0);
    case 1:
        if (ym3812->opl->address == 0x14) {
            mixerSync(ym3812->mixer);
        }
        return (UInt8)OPLRead(ym3812->opl, 1);
        break;
    }
    return  0xff;
}

void ym3812Write(YM3812* ym3812, UInt16 ioPort, UInt8 value) {
//	printf("opl2_write %x %x\n", ioPort, value);
    switch (ioPort & 1) {
    case 0:
        OPLWrite(ym3812->opl, 0, value);
        break;
    case 1:
        mixerSync(ym3812->mixer);
        OPLWrite(ym3812->opl, 1, value);
        break;
    }
}

void ym3812Destroy(YM3812* ym3812) {
    mixerUnregisterChannel(ym3812->mixer, ym3812->handle);
    boardTimerDestroy(ym3812->timer1);
    boardTimerDestroy(ym3812->timer2);

    ioPortUnregister(PORT_ADDR);
    ioPortUnregister(PORT_DATA);

    OPLDestroy(ym3812->opl);

//    if (ym3812->ykIo != NULL) {
//        ykIoDestroy(ym3812->ykIo);
//    }

    free(ym3812);
}
