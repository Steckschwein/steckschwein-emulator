#include <inttypes.h>
#include <time.h>
#include "ds1306.h"
#include "glue.h"

static struct tm *timestamp;
static bool auto_inc = false;
static uint8_t nvram[96];

void spi_rtc_select() {
	time_t ts = time(NULL);
	timestamp = localtime(&ts); //update timestamp with systime
	printf("rtc select() %s\n", asctime(timestamp));
	auto_inc = false;
}

uint8_t toBCD(uint8_t v) {
	return (v / 10) << 4 | (v % 10);
}

uint8_t spi_rtc_handle(uint8_t inbyte) {

	uint8_t outbyte = 0xff;

	static uint8_t addr = 0;

	if (!auto_inc) {
		addr = inbyte;
		auto_inc = true;
	} else {
		if (addr & 0x80) { //write
			if (addr & 0x7f >= 0x20) { //nvram access
				nvram[addr - 0x20] = inbyte;
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
				}
		}
		addr++;
	}

	printf("rtc %x %x\n", inbyte, outbyte);

	return outbyte;
}
