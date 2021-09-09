#ifndef _DS_1306_H_
#define _DS_1306_H_

#define	RTC_SECONDS		0x00
#define	RTC_MINUTES		0x01
#define	RTC_HOURS		0x02
#define	RTC_DAY_OF_WEEK		0x03
#define	RTC_DATE_OF_MONTH	0x04
#define	RTC_MONTH		0x05
#define	RTC_YEAR		0x06

#define	RTC_SECONDS_ALARM0	0x07
#define	RTC_MINUTES_ALARM0	0x08
#define	RTC_HOURS_ALARM0	0x09
#define	RTC_DAY_OF_WEEK_ALARM0	0x0a

#define	RTC_SECONDS_ALARM1	0x0b
#define	RTC_MINUTES_ALARM1	0x0c
#define	RTC_HOURS_ALARM1	0x0d
#define	RTC_DAY_OF_WEEK_ALARM1	0x0e

#define	RTC_CONTROL		0x0f
#define	RTC_STATUS		0x10
#define	RTC_TRICKLE_CHARGER	0x11

#define	RTC_USER_RAM_BASE	0x20

void spi_rtc_init();
void spi_rtc_destroy();

void spi_rtc_select();
void spi_rtc_deselect();// chip select /CE high
uint8_t spi_rtc_handle(uint8_t inbyte);

#endif
