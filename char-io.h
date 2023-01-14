// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _CHAR_IO_H_
#define _CHAR_IO_H_

#include <inttypes.h>

typedef struct Chardev {
  int fd;
} Chardev;

int chr_io_write(Chardev *chr, const uint8_t *buf, int len);

#endif
