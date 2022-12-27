// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
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

#define DEVICE_LINK "ttySSW_%#4x"

typedef struct UartIO {

    int masterfd;

    uint16_t ioPort;
    uint8_t uartregisters[16];

    UartType type;
    int  uartReady;

    char* device_link;

    void (*recvCallback)(uint8_t);
};

void recvCallback(uint8_t v){

}

UartIO* uart_create(uint16_t ioPort){

   // get the master fd
  int masterfd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if(masterfd < 0){
    perror("getpt");
    return NULL;
  }

  if(grantpt(masterfd) < 0)
  {
    perror("grantpt");
    return NULL;
  }

  if(unlockpt(masterfd) < 0)
  {
    perror("unlockpt");
    return NULL;
  }

  char slavepath[64];
  if(ptsname_r(masterfd, slavepath, sizeof(slavepath)) < 0)
  {
    perror("ptsname_r");
    return NULL;
  }

  char* device_name = (char*)malloc(32*sizeof(char));
  snprintf(device_name, 32*sizeof(char), DEVICE_LINK, ioPort);
  printf("i/o port %#4x mapped to %s (%s)\n", ioPort, device_name, slavepath);

  if(unlink(device_name)){
    errno = 0;//ignore, but reset errno
  }
  if(symlink(slavepath, device_name)){
    perror("symlink");
    return NULL;
  }

  UartIO* uart = malloc(sizeof(UartIO));
  uart->ioPort = ioPort;
  uart->masterfd = masterfd;
  uart->type = UART_HOST;
  uart->device_link = device_name;
  uart->recvCallback = recvCallback;

  return uart;
}

uint8_t uart_read(UartIO* uartIO, uint8_t reg) {
       return uartIO->uartregisters[reg];
}

void uart_write(UartIO* uartIO, uint8_t reg, uint8_t value) {
//     printf("uart w %x %x\n", reg, value);
       uartIO->uartregisters[reg] = value;
}

void uart_destroy(UartIO* uartIO) {

  printf("uart_destroy()\n");
  if(uartIO != NULL){
    printf("unlink %s\n", uartIO->device_link);
    unlink(uartIO->device_link);
    if(uartIO->masterfd != -1) {
      close(uartIO->masterfd);
    }
    free(uartIO->device_link);
    free(uartIO);
  }
}
