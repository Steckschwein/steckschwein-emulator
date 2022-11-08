#include "Machine.h"
#include <stdlib.h>

Machine* machineCreate(const char* machineName)
{
    char configIni[512];
    int success;
    // FILE *file;

    Machine* machine = (Machine *)malloc(sizeof(Machine));
    if (machine == NULL)
      return NULL;

    machine->zipFile = NULL;
    machine->isZipped = 0;
    machine->video.vdpVersion == VDP_V9958;
    machine->video.vramSize = 192*1024;

    // TODO
    // sprintf(configIni, "%s/%s/config.ini", machinesDir, machineName);
    // file = fopen(configIni, "rb");

    // if (file != NULL)
    // {
    //     fclose(file);
    // }
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

    // success = readMachine(machine, machineName, configIni);
    // if (!success)
    // {
  	// 	  machineDestroy(machine);
    //     return NULL;
    // }

    machineUpdate(machine);

    return machine;
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