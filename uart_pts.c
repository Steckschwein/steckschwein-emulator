#include <ArchThread.h>
#include <Emulator.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "uart.h"
#include "memory.h"
#include "glue.h"

#include "uart_16550.h"

/**

# serial loopback
$ socat -d -d pty,link=/tmp/ssw_uart0,raw,echo=0 pty,link=/tmp/ssw_emu_uart0,raw,echo=0 &
2022/09/17 12:02:50 socat[2554893] N PTY is /dev/pts/32
2022/09/17 12:02:50 socat[2554893] N PTY is /dev/pts/33
2022/09/17 12:02:50 socat[2554893] N starting data transfer loop with FDs [5,5] and [7,7]

# host device
$ cat /tmp/ssw_emu_uart0 & # im emu mit open("/tmp/ssw...

# emulator device
$ echo "Hallo Thomas... some serial" > /tmp/ssw_uart0
Hallo Thomas... some serial
*/

extern int errno;

#define DEVICE_LINK_TPL "ttySSW_%#4x"
#define DEVICE_UART_MAX 2

UartIO *a_uartIo[DEVICE_UART_MAX];

static short deviceIndex = 0;

void receiveLoop(){

  UartIO *uartIo = a_uartIo[deviceIndex++];
  if(!uartIo)
    return;

/*
  struct termios params;
  struct timespec ts;

  // TODO adjust pts with uart register settings
  if(tcgetattr(chr->fd, &params)){
    perror("tcgetattr");
    return;
  }
  cfmakeraw(&params);
  tcsetattr (chr->fd, TCSANOW, &params);
  tcflush(chr->fd, TCIOFLUSH);
*/
  while(emulatorGetState() != EMU_STOPPED){
    uart_step(uartIo);
//    archThreadSleep(1);
  }
}

void uart_step(UartIO* uartIo){

    Chardev *chr = &uartIo->chr;
    unsigned char bufin;

    struct pollfd pfd = { .fd = chr->fd, .events = POLLHUP, .revents = 0 };
    int c;
    c = poll(&pfd, 1, 100);
    if (c == -1){
      perror("poll()");
    }
    else if (c && (pfd.revents & POLLHUP)){
      c = read(chr->fd, &bufin, 1);
      if(c == 1){
        //printf("uart %s <= v: 0x%2x\n", uartIo->device_link, bufin);
        if(uartIo->recvCallback){
          uartIo->recvCallback(uartIo->device, &bufin, 1);
        }
      }
    }
}

void* createReceiverThread(UartIO *uart){

  a_uartIo[deviceIndex] = uart;
  return archThreadCreate(receiveLoop, THREAD_PRIO_NORMAL);
}

UartIO* uart_create(uint16_t ioPort, void *recvCallback){

  if(deviceIndex >= DEVICE_UART_MAX){//max devices
    fprintf(stderr, "requested uart device number %d, but only %d are allowed.", deviceIndex+1, DEVICE_UART_MAX);
    return NULL;
  }
  // open master
  int masterFd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if(masterFd < 0){
    perror("get ptmx failed");
    return NULL;
  }
  if(grantpt(masterFd) < 0){
    perror("grantpt");
    return NULL;
  }
  if(unlockpt(masterFd) < 0){
    perror("unlockpt");
    return NULL;
  }
  // determine slave
  char slavepath[64];
  if(ptsname_r(masterFd, slavepath, sizeof(slavepath)) < 0){
    perror("ptsname_r");
    return NULL;
  }

  int sfd;
  if ((sfd = open(slavepath, O_RDONLY | O_NOCTTY)) == -1) {
    perror("open slave");
    close(masterFd);
    return NULL;
  }

  struct termios tty;
  if (tcgetattr(sfd, &tty) < 0) {
    perror("ioctl on slave");
    close(sfd);
    close(masterFd);
  }

  tcgetattr(sfd, &tty);
  cfmakeraw(&tty);
  tcsetattr(sfd, TCSAFLUSH, &tty);
  close(sfd);

  // maintain symlink in cwd
  char* slave = (char*)calloc(32, sizeof(char));
  snprintf(slave, 32*sizeof(char), DEVICE_LINK_TPL, ioPort);
  printf("i/o port %#4x mapped to %s (%s)\n", ioPort, slave, slavepath);

  if(!unlink(slave)){//will fail if not exist already
    errno = 0;//but we've to reset errno, otherwise subsequent calls will fail
  }
  if(symlink(slavepath, slave)){
    perror("symlink");
    return NULL;
  }

  UartIO* uart = malloc(sizeof(UartIO));
  uart->chr.fd = masterFd;
  uart->type = UART_HOST;
  uart->device_link = slave;
  uart->recvCallback = recvCallback;
  uart->device = uart_16550_create(uart, &uart->chr, ioPort);
  uart->thread = createReceiverThread(uart);

  return uart;
}

void uart_destroy(UartIO* uartIo) {

  if(uartIo){
    if(uartIo->thread){
      archThreadJoin(uartIo->thread, 1000);
    }
    if(uartIo->device_link){
      printf("unlink %s\n", uartIo->device_link);
      unlink(uartIo->device_link);
      free(uartIo->device_link);
    }
    if(uartIo->chr.fd != -1) {
      close(uartIo->chr.fd);
    }
    uart_16550_destroy(uartIo->device);
    free(uartIo);
  }
}
