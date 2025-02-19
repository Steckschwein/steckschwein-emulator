#ifndef _DS_1306_H_
#define _DS_1306_H_

#include "ds130x.h"

void ds1306Reset(DS130x *device);
void ds1306Destroy(DS130x *device);

void spi_rtc_select();
void spi_rtc_deselect();// chip select /CE high
UInt8 spi_rtc_handle(DS130x *device, UInt8 inbyte);

#endif
