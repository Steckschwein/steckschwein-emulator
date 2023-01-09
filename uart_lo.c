#include <ArchThread.h>
#include <Emulator.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "uart.h"
#include "memory.h"
#include "glue.h"

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

  char bufin = 'O';
  int c;
  struct termios params;

  // TODO adjust slave with uart register settings
  int fd = open(uartIo->device_link, O_RDWR | O_NOCTTY);
  if(fd < 0){
    perror("open slave");
    return;
  }
  if(tcgetattr(fd, &params)){
    perror("tcgetattr");
    return;
  }
  cfmakeraw(&params);
  tcsetattr (fd, TCSANOW, &params);
  tcflush(fd, TCIOFLUSH);
  close(fd);

  while(emulatorGetState() != EMU_STOPPED){

    c = read(uartIo->masterFd, &bufin, 1);
    if(c == 1){
      printf("uart %s v: 0x%x\n", uartIo->device_link, bufin);
      c = write(uartIo->masterFd, &bufin, 1);
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

   // get the master fd
  int masterFd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if(masterFd < 0){
    perror("getpt");
    return NULL;
  }

  if(grantpt(masterFd) < 0)
  {
    perror("grantpt");
    return NULL;
  }

  if(unlockpt(masterFd) < 0)
  {
    perror("unlockpt");
    return NULL;
  }

  char slavepath[64];
  if(ptsname_r(masterFd, slavepath, sizeof(slavepath)) < 0)
  {
    perror("ptsname_r");
    return NULL;
  }

  char* device_name = (char*)calloc(32, sizeof(char));
  snprintf(device_name, 32*sizeof(char), DEVICE_LINK_TPL, ioPort);
  printf("i/o port %#4x mapped to %s (%s)\n", ioPort, device_name, slavepath);

  if(unlink(device_name)){//will fail if not exist already
    errno = 0;//but reset errno
  }
  if(symlink(slavepath, device_name)){
    perror("symlink");
    return NULL;
  }

  UartIO* uart = malloc(sizeof(UartIO));
  uart->ioPort = ioPort;
  uart->masterFd = masterFd;
  //uart->slaveFd = slaveFd;
  uart->type = UART_HOST;
  uart->device_link = device_name;
  uart->recvCallback = recvCallback;
  uart->thread = createReceiverThread(uart);

  return uart;
}

uint8_t uart_read(UartIO* uart, uint8_t reg) {
       return uart->uartregisters[reg];
}

void uart_write(UartIO* uart, uint8_t reg, uint8_t value) {
//     printf("uart w %x %x\n", reg, value);
  uart->uartregisters[reg] = value;
}

void uart_destroy(UartIO* uart) {

  if(uart){
    if(uart->thread){
      archThreadJoin(uart->thread, 1000);
    }
    if(uart->device_link){
      printf("unlink %s\n", uart->device_link);
      unlink(uart->device_link);
      free(uart->device_link);
    }
    if(uart->masterFd != -1) {
      close(uart->masterFd);
    }
    free(uart);
  }
}
