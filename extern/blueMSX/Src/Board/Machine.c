#include "Machine.h"
#include "Properties.h"
#include "IniFileParser.h"
#include <stdio.h>
#include <stdlib.h>

static char machinesDir[PROP_MAXPATH]  = "";

static int readMachine(Machine* machine, const char* machineName, const char* file)
{
    static char buffer[8192];
    char* slotBuf;
    int value;
    int i = 0;

    IniFile *configIni;

    if (machine->isZipped)
        configIni = iniFileOpenZipped(machine->zipFile, "config.ini");
    else
        configIni = iniFileOpen(file);

    if (configIni == NULL)
        return 0;

    strcpy(machine->name, machineName);
    // Read board info
    iniFileGetString(configIni, "Board", "type", "none", buffer, 10000);
    if (0 == strcmp(buffer, "Steckschwein-2.0")) machine->board.type = BOARD_STECKSCHWEIN_2_0;
    else if (0 == strcmp(buffer, "Steckschwein")) machine->board.type = BOARD_STECKSCHWEIN;
    else if (0 == strcmp(buffer, "JuniorComputer ][")) machine->board.type = BOARD_JC;
    else { iniFileClose(configIni); return 0; }

    // Read video info
    iniFileGetString(configIni, "Video", "version", "none", buffer, 10000);
    if      (0 == strcmp(buffer, "V9938"))    machine->video.vdpVersion = VDP_V9938;
    else if (0 == strcmp(buffer, "V9958"))    machine->video.vdpVersion = VDP_V9958;
    else if (0 == strcmp(buffer, "TMS9929A")) machine->video.vdpVersion = VDP_TMS9929A;
    else if (0 == strcmp(buffer, "TMS99x8A")) machine->video.vdpVersion = VDP_TMS99x8A;
    else { iniFileClose(configIni); return 0; }

    iniFileGetString(configIni, "Video", "vram size", "none", buffer, 10000);
    if (0 == strcmp(buffer, "16kB")) machine->video.vramSize = 16 * 1024;
    else if (0 == strcmp(buffer, "64kB")) machine->video.vramSize = 64 * 1024;
    else if (0 == strcmp(buffer, "128kB")) machine->video.vramSize = 128 * 1024;
    else if (0 == strcmp(buffer, "192kB")) machine->video.vramSize = 192 * 1024;
    else { iniFileClose(configIni); return 0; }

 // Read CPU info
    iniFileGetString(configIni, "CPU", "type", "6502", buffer, 10000);
    iniFileGetString(configIni, "CPU", "freq", "", buffer, 10000);
    if (0 == sscanf(buffer, "%dHz", &value)) {
        value = 3579545;
    }
    machine->cpu.freqCPU = value;


    iniFileClose(configIni);
//    machine->slotInfoCount = i;

    return 1;
}

Machine* machineCreate(RomImage* romImage, const char* machineName)
{
    char configIni[512];
    int success;
    FILE *file;

    Machine* machine = (Machine *)malloc(sizeof(Machine));
    if (machine == NULL)
      return NULL;

    machine->zipFile = NULL;
    machine->isZipped = 0;

    machine->romImage = romImage;

    sprintf(configIni, "%s/.sw/config.ini", getenv("HOME"));
    /*sprintf(configIni, "%s/%s/config.ini", machinesDir, machineName);
    file = fopen(configIni, "rb");
    if (file != NULL)
    {
        fclose(file);
    }
    */
    // else
    // {
    //     // No config.ini. Is it compressed?
    //     char zipFile[512];

    //     sprintf(zipFile, "%s/%s.zip", machinesDir, machineName);
    //     file = fopen(zipFile, "rb");

    //     if (file == NULL)
    //     {
    //         machineDestroy(machine);
    //         return NULL; // Not compressed and no config.ini
    //     }

    //     fclose(file);

    //     machine->zipFile = (char *)calloc(strlen(zipFile) + 1, sizeof(char));
    //     strcpy(machine->zipFile, zipFile);

    //     machine->isZipped = 1;
    // }

    success = readMachine(machine, machineName, configIni);
    if (!success)
    {
      machineDestroy(machine);
      return NULL;
    }

    machineUpdate(machine);

    return machine;
}

int machineInitialize(Machine* machine, UInt8** mainRam, UInt32* mainRamSize, UInt32* mainRamStart) {

  //machine->romImage

  return 1;

}

void machineUpdate(Machine* machine){
    // Check VRAM size
    if (machine->video.vdpVersion == VDP_V9938) {
        if (machine->video.vramSize >= 128 * 1024) {
            machine->video.vramSize = 128 * 1024;
        }
        else if (machine->video.vramSize >= 64 * 1024) {
            machine->video.vramSize = 64 * 1024;
        }
        else if (machine->video.vramSize >= 32 * 1024) {
            machine->video.vramSize = 32 * 1024;
        }
        else {
            machine->video.vramSize = 16 * 1024;
        }
    }
    else if (machine->video.vdpVersion == VDP_V9958) {
        if (machine->video.vramSize >= 192 * 1024) {
            machine->video.vramSize = 192 * 1024;
        }
        else {
            machine->video.vramSize = 128 * 1024;
        }
    }
    else {
        machine->video.vramSize = 16 * 1024;
    }

}


void machineDestroy(Machine* machine)
{
    if (machine->zipFile)
        free(machine->zipFile);

    free(machine);
}