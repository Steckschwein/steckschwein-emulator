#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include "ds1306.h"
#include "glue.h"

static const char *SW_DIR = ".sw";
static const char *SW_NVRAM = "nvram.dat";


DS130x* ds130xCreate(){

  return malloc(sizeof(DS130x));
}

char* swHomeDir(char *swDir) {
  #if __MINGW32_NO__
		snprintf(swDir, FILENAME_MAX, "%s%s/%s", getenv("HOMEDRIVE"), getenv("HOME"), SW_DIR);
	#else
	  snprintf(swDir, FILENAME_MAX, "%s/%s", getenv("HOME"), SW_DIR);
  #endif

	DIR *dir = opendir(swDir);
	if (dir == NULL || errno == ENOENT) {
		#if defined(_WIN32)
		dir = _mkdir(swDir);
		#else
		dir = mkdir(swDir, 0755);
		#endif
	}
	if (closedir(dir)) {
		fprintf(stderr, "error close dir %s\n", strerror(errno));
    return NULL;
	}
  return swDir;
}

char* nvramFilePath(char *nvramFile) {
	char swDir[FILENAME_MAX];
	char *homeDirStr = swHomeDir(&swDir);
  if(homeDirStr == NULL)
    return NULL;
	snprintf(nvramFile, FILENAME_MAX, "%s/%s", homeDirStr, SW_NVRAM);
	return nvramFile;
}

void ds130xReset(DS130x *device) {

	char nvramFile[FILENAME_MAX];
	char *nvramFileStr = nvramFilePath(&nvramFile);
  if(nvramFileStr != NULL){
  	FILE *f = fopen(nvramFileStr, "rb");
    if (f != NULL) {
      size_t r = fread(device->nvram, 1, sizeof(device->nvram), f);
      if (ferror(f) || r != sizeof(device->nvram)) {
        fprintf(stderr, "error read nvram state %s\n", strerror(errno));
      }
      fclose(f);
    }
  }
  device->regs[RTC_CONTROL] = 1<<6; //set WP (write protect enabled after reset)
}

void ds130xDestroy(DS130x *device) {

	char nvramFile[FILENAME_MAX];
	char *nvramFileStr = nvramFilePath(&nvramFile);
  if(nvramFileStr != NULL){
    FILE *f = fopen(nvramFileStr, "wb");
    if (f != NULL) {
      size_t r = fwrite(device->nvram, 1, sizeof(device->nvram), f);
      if (ferror(f)) {
        fprintf(stderr, "error fwrite %s\n", strerror(errno));
      }
      fclose(f);
    }
  }
}
