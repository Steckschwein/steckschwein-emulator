// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/stat.h>
#include <sys/types.h>
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

#define DEVICE_LINK "ttySSW_%#4x"

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

  char* device_name = (char*)calloc(32, sizeof(char));
  snprintf(device_name, 32*sizeof(char), DEVICE_LINK, ioPort);
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
  uart->masterfd = masterfd;
  uart->type = UART_HOST;
  uart->device_link = device_name;
  uart->recvCallback = recvCallback;

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

  printf("uart_destroy()\n");
  if(uart){
    if(uart->device_link){
      printf("unlink %s\n", uart->device_link);
      unlink(uart->device_link);
      free(uart->device_link);
    }
    if(uart->masterfd != -1) {
      close(uart->masterfd);
    }
    free(uart);
  }
}
