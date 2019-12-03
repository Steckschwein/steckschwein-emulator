#include <inttypes.h>
#include <time.h>
#include "ds1306.h"

static struct tm* timestamp;

void spi_rtc_select() {
	time_t ts = time(NULL);
	timestamp = localtime(&ts); //update timestamp with systime
	printf("rtc select() %s\n", asctime(timestamp));
}

uint8_t spi_rtc_handle(uint8_t inbyte) {

	uint8_t outbyte = 0xff;

	static uint8_t addr = 0;

	if (inbyte != 0x7) {
		addr = inbyte & 0x0f; //adjust addr pointer
	} else
		switch (addr) {
		case 0:
			outbyte = timestamp->tm_sec;
			break;
		case 1:
			outbyte = timestamp->tm_min;
			break;
		case 2:
			outbyte = timestamp->tm_hour;
			break;
		case 3:
			outbyte = timestamp->tm_wday;
			break;
		case 4:
			outbyte = timestamp->tm_mday;
			break;
		case 5:
			outbyte = timestamp->tm_year;
			break;
		}
	addr++;

	printf("rtc %x %x\n",inbyte, outbyte);

	return outbyte;
}
