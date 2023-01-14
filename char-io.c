
#include "char-io.h"

int chr_io_write(Chardev *chr, const uint8_t *buf, int len){

  int res = write(chr->fd, buf, len);
  if (res < 0) {
    return res;
  }
  return res;
}
