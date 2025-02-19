#include "ds1306.h"

static bool chip_select = false;


void ds1306Reset(DS130x *device) {
  ds130xReset(device);
}

void ds1306Destroy(DS130x *device) {
  ds130xDestroy(device);
}

static UInt8 toBCD(UInt8 v) {
	return (v / 10) << 4 | (v % 10);
}


void spi_rtc_deselect() { // chip select /CE high
	chip_select = false;
#ifdef TRACE_RTC
	printf("rtc deselect()\n");
#endif
}

void spi_rtc_select(DS130x *device) {
	time_t ts = time(NULL);
	device->timestamp = localtime(&ts); //update timestamp with systime
#ifdef TRACE_RTC
	printf("rtc select() %s\n", asctime(timestamp));
#endif
	chip_select = false;
}

UInt8 spi_rtc_handle(DS130x *device, UInt8 inbyte) {

	UInt8 outbyte = 0xff;

	static UInt8 addr = 0;

	if (!chip_select) {
		addr = inbyte;
		chip_select = true;
	} else {
#ifdef TRACE_RTC
    printf("RTC access %x %x\n", (addr & 0x7f) - 0x20, inbyte);
#endif
		if (addr & 0x80 && ((addr & 0x1f) == RTC_CONTROL || (device->regs[RTC_CONTROL] & 0x40) == 0)) { //write, if WP = 0
			if ((addr & 0x7f) >= 0x20) { //nvram access?
				device->nvram[(addr & 0x7f) - 0x20] = inbyte;
			}else{
        device->regs[addr & 0x1f] = inbyte;
      }
		} else { //read
			if (addr >= 0x20) { //nvram access
				outbyte = device->nvram[addr - 0x20];
			} else
				switch (addr) {
				case RTC_SECONDS:
					outbyte = toBCD(device->timestamp->tm_sec);
					break;
				case RTC_MINUTES:
					outbyte = toBCD(device->timestamp->tm_min);
					break;
				case RTC_HOURS:
					outbyte = toBCD(device->timestamp->tm_hour);
					break;
				case RTC_DAY_OF_WEEK:
					outbyte = toBCD(device->timestamp->tm_wday);
					break;
				case RTC_DATE_OF_MONTH:
					outbyte = toBCD(device->timestamp->tm_mday);
					break;
				case RTC_MONTH:
					outbyte = toBCD(device->timestamp->tm_mon) + 1;
					break;
				case RTC_YEAR:
					outbyte = toBCD(device->timestamp->tm_year - 100);
					break;
        default:
          outbyte = device->regs[addr & 0x1f] ;
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
