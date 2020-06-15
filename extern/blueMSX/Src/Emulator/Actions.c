/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Actions.c,v $
**
** $Revision: 1.80 $
**
** $Date: 2008-05-14 12:55:31 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "Actions.h"
#include "MsxTypes.h"
#include "AudioMixer.h"
#include "Board.h"
#include "Debugger.h"
#include "VideoManager.h"
#include "VDP.h"

#include "ArchVideoIn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

static struct {
    Properties* properties;
    Video* video;
    Mixer* mixer;
    int mouseLock;
    int windowedSize;
} state;

static char audioDir[PROP_MAXPATH]  = "";
static char audioPrefix[64]         = "";
static char videoDir[PROP_MAXPATH]  = "";
static char videoPrefix[64]         = "";
char stateDir[PROP_MAXPATH]  = "";
char statePrefix[64]         = "";



void actionSetAudioCaptureSetDirectory(char* dir, char* prefix)
{
    strcpy(audioDir, dir);
    strcpy(audioPrefix, prefix);
}

void actionSetVideoCaptureSetDirectory(char* dir, char* prefix)
{
    strcpy(videoDir, dir);
    strcpy(videoPrefix, prefix);
}

void actionSetQuickSaveSetDirectory(char* dir, char* prefix)
{
    strcpy(stateDir, dir);
    strcpy(statePrefix, prefix);
}

void actionInit(Video* video, Properties* properties, Mixer* mixer)
{
    memset(&state, 0, sizeof(state));

    state.properties = properties;
    state.video      = video;
    state.mixer      = mixer;

    state.windowedSize = properties->video.windowSize != P_VIDEO_SIZEFULLSCREEN ?
                         properties->video.windowSize : P_VIDEO_SIZEX2;
}

void actionToggleSpriteEnable() {
    vdpSetSpritesEnable(!vdpGetSpritesEnable());
}

void actionToggleNoSpriteLimits() {
    vdpSetNoSpriteLimits(!vdpGetNoSpritesLimit());
}


void actionToggleFdcTiming() {
    state.properties->emulation.enableFdcTiming = !state.properties->emulation.enableFdcTiming;
    boardSetFdcTimingEnable(state.properties->emulation.enableFdcTiming);
}

void actionEmuStep() {
    if (emulatorGetState() == EMU_PAUSED) {
        emulatorSetState(EMU_STEP);
    }
}

void actionEmuStepBack() {
    if (emulatorGetState() == EMU_PAUSED) {
        emulatorSetState(EMU_STEP_BACK);
    }
}

void actionEmuTogglePause() {
    if (emulatorGetState() == EMU_STOPPED) {
        emulatorStart(NULL);
    }
    else if (emulatorGetState() == EMU_PAUSED) {
        emulatorSetState(EMU_RUNNING);
        debuggerNotifyEmulatorResume();
    }
    else {  
        emulatorSetState(EMU_PAUSED);
        debuggerNotifyEmulatorPause();
    }
//    archUpdateMenu(0);
}

void actionEmuStop() {
    if (emulatorGetState() != EMU_STOPPED) {
        emulatorStop();
    }
//    archUpdateMenu(0);
}


void actionWindowSizeSmall() {
    state.windowedSize = P_VIDEO_SIZEX1;
    if (state.properties->video.windowSize != P_VIDEO_SIZEX1) {
        state.properties->video.windowSize = P_VIDEO_SIZEX1;
        state.properties->video.windowSizeChanged = 1;
//        archUpdateWindow();
    }
}

void actionWindowSizeNormal() {
    state.windowedSize = P_VIDEO_SIZEX2;
    if (state.properties->video.windowSize != P_VIDEO_SIZEX2) {
        state.properties->video.windowSize = P_VIDEO_SIZEX2;
        state.properties->video.windowSizeChanged = 1;
//        archUpdateWindow();
    }
}

void actionWindowSizeFullscreen() {
    if (state.properties->video.windowSize != P_VIDEO_SIZEFULLSCREEN) {
        state.properties->video.windowSize = P_VIDEO_SIZEFULLSCREEN;
        state.properties->video.windowSizeChanged = 1;
//        archUpdateWindow();
    }
}

void actionWindowSizeMinimized() {
//    archMinimizeMainWindow();
}

void actionMaxSpeedToggle() {
//    emulatorSetMaxSpeed(emulatorGetMaxSpeed() ? 0 : 1);
}

void actionFullscreenToggle() {
    if (state.properties->video.windowSize == P_VIDEO_SIZEFULLSCREEN) {
        if (state.windowedSize == P_VIDEO_SIZEX2) {
            actionWindowSizeNormal();
        }
        else {
            actionWindowSizeSmall();
        }
    }
    else {
        actionWindowSizeFullscreen();
    }
//    archUpdateMenu(0);
}

void actionEmuSpeedNormal() {
    state.properties->emulation.speed = 50;
    emulatorSetFrequency(state.properties->emulation.speed, NULL);
}

void actionEmuSpeedDecrease() {
    if (state.properties->emulation.speed > 0) {
        state.properties->emulation.speed--;
        emulatorSetFrequency(state.properties->emulation.speed, NULL);
    }
}

void actionEmuSpeedIncrease() {
    if (state.properties->emulation.speed < 100) {
        state.properties->emulation.speed++;
        emulatorSetFrequency(state.properties->emulation.speed, NULL);
    }
}

void actionEmuResetSoft() {
//    archUpdateMenu(0);
    if (emulatorGetState() == EMU_RUNNING) {
        emulatorSuspend();
        boardReset();
        debuggerNotifyEmulatorReset();
        emulatorResume();
    }
    else {
        emulatorStart(NULL);
    }
//    archUpdateMenu(0);
}

void actionEmuResetHard() {
//    archUpdateMenu(0);
    emulatorStop();
    emulatorStart(NULL);
//    archUpdateMenu(0);
}

void actionVolumeIncrease() {
    state.properties->sound.masterVolume += 5;
    if (state.properties->sound.masterVolume > 100) {
        state.properties->sound.masterVolume = 100;
    }
    mixerSetMasterVolume(state.mixer, state.properties->sound.masterVolume);
}

void actionVolumeDecrease() {
    state.properties->sound.masterVolume -= 5;
    if (state.properties->sound.masterVolume < 0) {
        state.properties->sound.masterVolume = 0;
    }
    mixerSetMasterVolume(state.mixer, state.properties->sound.masterVolume);
}
 
void actionMuteToggleMaster() {
    state.properties->sound.masterEnable = !state.properties->sound.masterEnable;
    mixerEnableMaster(state.mixer, state.properties->sound.masterEnable);
}

void actionMuteTogglePsg() {
    int channel = MIXER_CHANNEL_PSG;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteTogglePcm() {
    int channel = MIXER_CHANNEL_PCM;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleIo() {
    int channel = MIXER_CHANNEL_IO;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleScc() {
    int channel = MIXER_CHANNEL_SCC;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleKeyboard() {
    int channel = MIXER_CHANNEL_KEYBOARD;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleMsxMusic() {
    int channel = MIXER_CHANNEL_MSXMUSIC;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleMsxAudio() {
    int channel = MIXER_CHANNEL_MSXAUDIO;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleMoonsound() {
    int channel = MIXER_CHANNEL_MOONSOUND;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleYamahaSfg() {
    int channel = MIXER_CHANNEL_YAMAHA_SFG;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionMuteToggleMidi() {
    int channel = MIXER_CHANNEL_MIDI;
    int newEnable = !state.properties->sound.mixerChannel[channel].enable;
    state.properties->sound.mixerChannel[channel].enable = newEnable;
    mixerEnableChannelType(state.mixer, channel, newEnable);
}

void actionVolumeToggleStereo() {
    state.properties->sound.stereo = !state.properties->sound.stereo;

    emulatorRestartSound();
}

void actionEmuSpeedSet(int value) {
    state.properties->emulation.speed = value;
    emulatorSetFrequency(state.properties->emulation.speed, NULL);
}

void actionVolumeSetMaster(int value) {
    state.properties->sound.masterVolume = value;
    mixerSetMasterVolume(state.mixer, value);
}

void actionVolumeSetPsg(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_PSG].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_PSG, value);
}

void actionVolumeSetPcm(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_PCM].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_PCM, value);
}

void actionVolumeSetIo(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_IO].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_IO, value);
}

void actionVolumeSetScc(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_SCC].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_SCC, value);
}

void actionVolumeSetMsxMusic(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MSXMUSIC].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_MSXMUSIC, value);
}

void actionVolumeSetMsxAudio(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MSXAUDIO].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_MSXAUDIO, value);
}

void actionVolumeSetMoonsound(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MOONSOUND].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_MOONSOUND, value);
}

void actionVolumeSetYamahaSfg(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_YAMAHA_SFG].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_YAMAHA_SFG, value);
}

void actionVolumeSetKeyboard(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_KEYBOARD].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_KEYBOARD, value);
}

void actionVolumeSetMidi(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MIDI].volume = value;
    mixerSetChannelTypeVolume(state.mixer, MIXER_CHANNEL_MIDI, value);
}

void actionPanSetPsg(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_PSG].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_PSG, value);
}

void actionPanSetPcm(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_PCM].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_PCM, value);
}

void actionPanSetIo(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_IO].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_IO, value);
}

void actionPanSetScc(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_SCC].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_SCC, value);
}

void actionPanSetMsxMusic(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MSXMUSIC].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_MSXMUSIC, value);
}

void actionPanSetMsxAudio(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MSXAUDIO].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_MSXAUDIO, value);
}

void actionPanSetMoonsound(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MOONSOUND].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_MOONSOUND, value);
}

void actionPanSetYamahaSfg(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_YAMAHA_SFG].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_YAMAHA_SFG, value);
}

void actionPanSetKeyboard(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_KEYBOARD].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_KEYBOARD, value);
}

void actionPanSetMidi(int value) {
    state.properties->sound.mixerChannel[MIXER_CHANNEL_MIDI].pan = value;
    mixerSetChannelTypePan(state.mixer, MIXER_CHANNEL_MIDI, value);
}


void actionSetSpriteEnable(int value) {
    vdpSetSpritesEnable(value);
}

void actionSetNoSpriteLimits(int value) {
	vdpSetNoSpriteLimits(value);
}

void actionSetFdcTiming(int value) {
    state.properties->emulation.enableFdcTiming = value ? 1 : 0;
    boardSetFdcTimingEnable(state.properties->emulation.enableFdcTiming);
}

void actionSetFullscreen(int value) {
    if (value == 0 && state.properties->video.windowSize == P_VIDEO_SIZEFULLSCREEN) {
        if (state.windowedSize == P_VIDEO_SIZEX2) {
            actionWindowSizeNormal();
        }
        else {
            actionWindowSizeSmall();
        }
    }
    else if (state.properties->video.windowSize != P_VIDEO_SIZEFULLSCREEN) {
        actionWindowSizeFullscreen();
    }
}

void actionSetVolumeMute(int value) {
    int oldEnable = state.properties->sound.masterEnable;
    state.properties->sound.masterEnable = value ? 1 : 0;
    if (oldEnable != state.properties->sound.masterEnable) {
        mixerEnableMaster(state.mixer, state.properties->sound.masterEnable);
    }
}

void actionSetVolumeStereo(int value) {
    int oldStereo = state.properties->sound.stereo;
    state.properties->sound.stereo = value ? 1 : 0;
    if (oldStereo != state.properties->sound.stereo) {
        emulatorRestartSound();
    }
}
