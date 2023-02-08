#include <stdio.h>
#include <errno.h>
#include <termio.h>

#include "char-io.h"

int chr_io_write(Chardev *chr, const uint8_t *buf, int len){

  int res = write(chr->fd, buf, len);
  if (res < 0) {
    return res;
  }
  return res;
}


static void tty_serial_init(int fd, int speed,
                            int parity, int data_bits, int stop_bits)
{
    struct termios tty = {0};
    speed_t spd;

#if 1
    printf("tty_serial_init (fd=%d): speed=%d parity=%c data=%d stop=%d\n",
           fd, speed, parity, data_bits, stop_bits);
#endif
    tcgetattr(fd, &tty);

#define check_speed(val) \
    if (speed <= val) {  \
        spd = B##val;    \
        goto done;       \
    }

    speed = speed * 10 / 11;
    check_speed(50);
    check_speed(75);
    check_speed(110);
    check_speed(134);
    check_speed(150);
    check_speed(200);
    check_speed(300);
    check_speed(600);
    check_speed(1200);
    check_speed(1800);
    check_speed(2400);
    check_speed(4800);
    check_speed(9600);
    check_speed(19200);
    check_speed(38400);
    /* Non-Posix values follow. They may be unsupported on some systems. */
    check_speed(57600);
    check_speed(115200);
#ifdef B230400
    check_speed(230400);
#endif
#ifdef B460800
    check_speed(460800);
#endif
#ifdef B500000
    check_speed(500000);
#endif
#ifdef B576000
    check_speed(576000);
#endif
#ifdef B921600
    check_speed(921600);
#endif
#ifdef B1000000
    check_speed(1000000);
#endif
#ifdef B1152000
    check_speed(1152000);
#endif
#ifdef B1500000
    check_speed(1500000);
#endif
#ifdef B2000000
    check_speed(2000000);
#endif
#ifdef B2500000
    check_speed(2500000);
#endif
#ifdef B3000000
    check_speed(3000000);
#endif
#ifdef B3500000
    check_speed(3500000);
#endif
#ifdef B4000000
    check_speed(4000000);
#endif
    spd = B115200;

#undef check_speed
 done:
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                     | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    tty.c_cflag &= ~(CSIZE | PARENB | PARODD | CRTSCTS | CSTOPB);
    switch (data_bits) {
    default:
    case 8:
        tty.c_cflag |= CS8;
        break;
    case 7:
        tty.c_cflag |= CS7;
        break;
    case 6:
        tty.c_cflag |= CS6;
        break;
    case 5:
        tty.c_cflag |= CS5;
        break;
    }
    switch (parity) {
    default:
    case 'N':
        break;
    case 'E':
        tty.c_cflag |= PARENB;
        break;
    case 'O':
        tty.c_cflag |= PARENB | PARODD;
        break;
    }
    if (stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    }

    tcsetattr(fd, TCSANOW, &tty);
}

int chr_fe_ioctl(Chardev *s, int cmd, void *arg)
{
    // FDChardev *s = FD_CHARDEV(chr);
    // QIOChannelFile *fioc = QIO_CHANNEL_FILE(s->ioc_in);
    Chardev* fioc = s;

    switch (cmd) {
    case CHR_IOCTL_SERIAL_SET_PARAMS:
        {
            SerialSetParams *ssp = arg;
            tty_serial_init(fioc->fd,
                            ssp->speed, ssp->parity,
                            ssp->data_bits, ssp->stop_bits);
        }
        break;
    case CHR_IOCTL_SERIAL_SET_BREAK:
        {
            int enable = *(int *)arg;
            if (enable) {
                tcsendbreak(fioc->fd, 1);
            }
        }
        break;
    case CHR_IOCTL_SERIAL_GET_TIOCM:
        {
            int sarg = 0;
            int *targ = (int *)arg;
            ioctl(fioc->fd, TIOCMGET, &sarg);
            *targ = 0;
            if (sarg & TIOCM_CTS) {
                *targ |= CHR_TIOCM_CTS;
            }
            if (sarg & TIOCM_CAR) {
                *targ |= CHR_TIOCM_CAR;
            }
            if (sarg & TIOCM_DSR) {
                *targ |= CHR_TIOCM_DSR;
            }
            if (sarg & TIOCM_RI) {
                *targ |= CHR_TIOCM_RI;
            }
            if (sarg & TIOCM_DTR) {
                *targ |= CHR_TIOCM_DTR;
            }
            if (sarg & TIOCM_RTS) {
                *targ |= CHR_TIOCM_RTS;
            }
        }
        break;
    case CHR_IOCTL_SERIAL_SET_TIOCM:
        {
            int sarg = *(int *)arg;
            int targ = 0;
            ioctl(fioc->fd, TIOCMGET, &targ);
            targ &= ~(CHR_TIOCM_CTS | CHR_TIOCM_CAR | CHR_TIOCM_DSR
                     | CHR_TIOCM_RI | CHR_TIOCM_DTR | CHR_TIOCM_RTS);
            if (sarg & CHR_TIOCM_CTS) {
                targ |= TIOCM_CTS;
            }
            if (sarg & CHR_TIOCM_CAR) {
                targ |= TIOCM_CAR;
            }
            if (sarg & CHR_TIOCM_DSR) {
                targ |= TIOCM_DSR;
            }
            if (sarg & CHR_TIOCM_RI) {
                targ |= TIOCM_RI;
            }
            if (sarg & CHR_TIOCM_DTR) {
                targ |= TIOCM_DTR;
            }
            if (sarg & CHR_TIOCM_RTS) {
                targ |= TIOCM_RTS;
            }
            ioctl(fioc->fd, TIOCMSET, &targ);
        }
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}