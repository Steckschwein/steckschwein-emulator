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
static struct tm *timestamp;
static bool chip_select = false;
static uint8_t nvram[96] = { 0x42, 'L', 'O', 'A', 'D', 'E', 'R', ' ', ' ', 'B', 'I', 'N', 1, 3, 0x37 };

static uint8_t regs[0x20];

int _1HzTimeHanler(void* timer) {
  sprintf(stderr, ".");

  return 20;//continue
}

char* swHomeDir() {
	char swDir[FILENAME_MAX];
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
	}

	return strdup(swDir);
}

char* nvramFile() {
	char nvramFile[FILENAME_MAX];
	char *homeDirStr = swHomeDir();
	snprintf(nvramFile, FILENAME_MAX, "%s/%s", homeDirStr, SW_NVRAM);
	free(homeDirStr);
	return strdup(nvramFile);
}

void spi_rtc_reset() {

	char *nvramFileStr = nvramFile();
	FILE *f = fopen(nvramFileStr, "rb");
	if (f != NULL) {
		size_t r = fread(nvram, 1, sizeof(nvram), f);
		if (ferror(f) || r != sizeof(nvram)) {
			fprintf(stderr, "error read nvram state %s\n", strerror(errno));
		}
		fclose(f);
	}
	free(nvramFileStr);

  regs[RTC_CONTROL] = 1<<6; //set WP (write protect enabled after reset)
}

void spi_rtc_destroy() {
	char *nvramFileStr = nvramFile();
	FILE *f = fopen(nvramFileStr, "wb");
	if (f != NULL) {
		size_t r = fwrite(nvram, 1, sizeof(nvram), f);
		if (ferror(f)) {
			fprintf(stderr, "error fwrite %s\n", strerror(errno));
		}
		fclose(f);
	}
	free(nvramFileStr);
}

void spi_rtc_deselect() { // chip select /CE high
	chip_select = false;
#ifdef TRACE_RTC
	printf("rtc deselect()\n");
#endif
}

void spi_rtc_select() {
	time_t ts = time(NULL);
	timestamp = localtime(&ts); //update timestamp with systime
#ifdef TRACE_RTC
	printf("rtc select() %s\n", asctime(timestamp));
#endif
	chip_select = false;
}

uint8_t toBCD(uint8_t v) {
	return (v / 10) << 4 | (v % 10);
}

uint8_t spi_rtc_handle(uint8_t inbyte) {

	uint8_t outbyte = 0xff;

	static uint8_t addr = 0;

	if (!chip_select) {
		addr = inbyte;
		chip_select = true;
	} else {
#ifdef TRACE_RTC
    printf("RTC access %x %x\n", (addr & 0x7f) - 0x20, inbyte);
#endif
		if (addr & 0x80 && ((addr & 0x1f) == RTC_CONTROL || (regs[RTC_CONTROL] & 0x40) == 0)) { //write, if WP = 0
			if ((addr & 0x7f) >= 0x20) { //nvram access?
				nvram[(addr & 0x7f) - 0x20] = inbyte;
			}else{
        regs[addr & 0x1f] = inbyte;
      }
		} else { //read
			if (addr >= 0x20) { //nvram access
				outbyte = nvram[addr - 0x20];
			} else
				switch (addr) {
				case RTC_SECONDS:
					outbyte = toBCD(timestamp->tm_sec);
					break;
				case RTC_MINUTES:
					outbyte = toBCD(timestamp->tm_min);
					break;
				case RTC_HOURS:
					outbyte = toBCD(timestamp->tm_hour);
					break;
				case RTC_DAY_OF_WEEK:
					outbyte = toBCD(timestamp->tm_wday);
					break;
				case RTC_DATE_OF_MONTH:
					outbyte = toBCD(timestamp->tm_mday);
					break;
				case RTC_MONTH:
					outbyte = toBCD(timestamp->tm_mon) + 1;
					break;
				case RTC_YEAR:
					outbyte = toBCD(timestamp->tm_year - 100);
					break;
        default:
          outbyte = regs[addr & 0x1f] ;
          break;
				}
		}
    addr++;
#ifdef TRACE_RTC
		printf("rtc %x %x %x %x\n", inbyte, outbyte, addr, chip_select);
#endif
	}

	return outbyte;
}
