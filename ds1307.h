#ifndef _DS_1307_H_
#define _DS_1307_H_

#include "ds130x.h"

void ds1307Write(DS130x *device, UInt8 address, UInt8 value);
UInt8 ds1307Read(DS130x *device, UInt8 inbyte);

void ds1307Reset(DS130x *device);
void ds1307Destroy(DS130x *device);

#endif
