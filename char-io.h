#ifndef _CHAR_IO_H_
#define _CHAR_IO_H_

#include <inttypes.h>

#define CHR_IOCTL_SERIAL_SET_PARAMS   1
typedef struct {
    int speed;
    int parity;
    int data_bits;
    int stop_bits;
} SerialSetParams;

#define CHR_IOCTL_SERIAL_SET_BREAK    2

#define CHR_IOCTL_SERIAL_SET_TIOCM   13
#define CHR_IOCTL_SERIAL_GET_TIOCM   14

#define CHR_TIOCM_CTS   0x020
#define CHR_TIOCM_CAR   0x040
#define CHR_TIOCM_DSR   0x100
#define CHR_TIOCM_RI    0x080
#define CHR_TIOCM_DTR   0x002
#define CHR_TIOCM_RTS   0x004

typedef struct Chardev {
  int fd;
} Chardev;

int chr_fe_ioctl(Chardev *s, int cmd, void *arg);

int chr_io_write(Chardev *chr, const uint8_t *buf, int len);

#endif
