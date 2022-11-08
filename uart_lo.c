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

static uint8_t uartregisters[16];

extern int errno;

int masterfd = -1;

char* device_link = "ttySSW_0x220";

int uart_init(unsigned char *prg_path, int prg_override_start, bool checkLastModified){

   // get the master fd
  masterfd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if(masterfd < 0){
    perror("getpt");
    return 1;
  }

  if(grantpt(masterfd) < 0)
  {
    perror("grantpt");
    return 1;
  }

  if(unlockpt(masterfd) < 0)
  {
    perror("unlockpt");
    return 1;
  }

  char slavepath[64];
  if(ptsname_r(masterfd, slavepath, sizeof(slavepath)) < 0)
  {
    perror("ptsname_r");
    return 1;
  }

  printf("using %s\n", slavepath);

  if(unlink(device_link)){
    errno = 0;//ignore, but reset errno
  }
  if(symlink(slavepath, device_link)){
    perror("symlink");
    return 1;
  }
  return 0;
}

uint8_t uart_read(uint8_t reg) {
	return uartregisters[reg];
}

void uart_write(uint8_t reg, uint8_t value) {
//	printf("uart w %x %x\n", reg, value);
	uartregisters[reg] = value;
}

void uart_destroy() {

  unlink(device_link);

  if(masterfd != -1) {
    close(masterfd);
  }
}