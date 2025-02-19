#include "Machine.h"
#include "Properties.h"
#include "TokenExtract.h"
#include "IniFileParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char machinesDir[PROP_MAXPATH]  = "";

int toint(char* buffer)
{
    int i;

    if (buffer == NULL) {
        return -1;
    }

    for (i = 0; buffer[i]; i++) {
        if (!isdigit(buffer[i])) return -1;
    }

    return atoi(buffer);
}


int tohex(char* buffer)
{
    int i;

    if (buffer == NULL) {
        return -1;
    }

    if (sscanf(buffer, "%x", &i) != 1) return -1;

    return i;
}

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

   // Read slots
    if(iniFileGetSection(configIni, "Slots", buffer, 10000)){

      slotBuf = buffer;

      for (i = 0; i < sizeof(machine->slotInfo) / sizeof(SlotInfo) && *slotBuf; i++) {
          char* arg;
          char slotInfoName[512];
          char *slotFilename;

          while(*slotBuf == '#'){
            slotBuf += strlen(slotBuf) + 1;//next entry
          }

          machine->slotInfo[i].slot = i;
          machine->slotInfo[i].address = tohex(extractToken(slotBuf, 0));
          machine->slotInfo[i].size = tohex(extractToken(slotBuf, 1));

          arg = extractToken(slotBuf, 2);
          strcpy(machine->slotInfo[i].name, arg ? arg : "");

          arg = extractToken(slotBuf, 3);
          if(arg){
            machine->slotInfo[i].romPath = arg;
          }

          arg = extractToken(slotBuf, 4);
          strcpy(machine->slotInfo[i].inZipName, arg ? arg : "");

          if (machine->slotInfo[i].slot < 0 || machine->slotInfo[i].slot >= 4) { iniFileClose(configIni); return 0; }
          if (machine->slotInfo[i].address < 0) { iniFileClose(configIni); return 0; }
          if (machine->slotInfo[i].size < 0) { iniFileClose(configIni); return 0; }

          slotBuf += strlen(slotBuf) + 1;//next entry

          strcpy(slotInfoName, machine->slotInfo[i].name);

          slotFilename = strrchr(slotInfoName, '/');
          if (slotFilename == NULL)
              slotFilename = strrchr(slotInfoName, '\\');
          if (slotFilename == NULL)
              slotFilename = slotInfoName;
          else
              slotFilename++;

          if (!machine->isZipped)
          {
              // Convert the relative path into absolute

              if (strcasestr(machine->slotInfo[i].name, "Machines/") == machine->slotInfo[i].name ||
                  strcasestr(machine->slotInfo[i].name, "Machines\\") == machine->slotInfo[i].name)
              {
                  char expandedPath[1024];
                  sprintf(expandedPath, "%s/%s", machinesDir, machine->slotInfo[i].name + 9);
                  strcpy(machine->slotInfo[i].name, expandedPath);
              }
          }
          else if (*machine->slotInfo[i].name)
          {
              char iniFilePath[512];
              char *parentDir;

              strcpy(iniFilePath, iniFileGetFilePath(configIni));

              parentDir = strrchr(iniFilePath, '/');
              if (parentDir == NULL)
                  parentDir = strrchr(iniFilePath, '\\');
              if (parentDir == NULL)
                  parentDir = iniFilePath;
              else
                  *(parentDir + 1) = '\0';

              strcpy(machine->slotInfo[i].name, machine->zipFile);
              sprintf(machine->slotInfo[i].inZipName, "%s%s",
                      iniFilePath, slotFilename);
          }

  #ifdef __APPLE__
          // On OS X, replace all backslashes with slashes
          for (char *ch = machine->slotInfo[i].name; *ch; ch++)
              if (*ch == '\\') *ch = '/';
  #endif
      }

      machine->slotInfoCount = i;
    }

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