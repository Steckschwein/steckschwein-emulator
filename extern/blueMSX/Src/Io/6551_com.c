/*
 **  Oricutron
 **  Copyright (C) 2009-2014 Peter Gordon
 **
 **  This program is free software; you can redistribute it and/or
 **  modify it under the terms of the GNU General Public License
 **  as published by the Free Software Foundation, version 2
 **  of the License.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **
 **  6551 ACIA emulation - com back-end
 */
#include "6551.h"

#ifdef BACKEND_COM

static int com_baudrate = 115200;
static int com_bits = 8;
static char com_parity = 'N';
static int com_stopbits = 1;
static char com_name[ACIA_BACKEND_NAME_LEN];

/* forward definitions */
static bool com_validate_param(void);
static bool com_open(void);
static bool com_close(void);
static bool com_write(UInt8 data);
static bool com_read(UInt8* data);
static bool com_peek(UInt8* data);

static void com_done(MOS6551 *acia )
{
  com_close();
  acia_init_none( acia );
}

static UInt8 com_stat(UInt8 stat)
{
  return (stat | (ASTF_CARRIER|ASTF_DSR));
}

static bool com_has_byte(UInt8* data)
{
  return com_peek(data);
}

static bool com_get_byte(UInt8* data)
{
  return com_read(data);
}

static bool com_put_byte(UInt8 data)
{
  return com_write(data);
}

bool acia_init_com(MOS6551 *acia )
{
  char *aciabackendname = "com:19200,8,N,1,/tmp/ttyJC1";

  if(5 == sscanf(aciabackendname, "com:%d,%d,%c,%d,%s",
    &com_baudrate, &com_bits, &com_parity, &com_stopbits, com_name))
  {
    //printf("com: %d, %d, %c, %d, %s\n", com_baudrate, com_bits, com_parity, com_stopbits, com_name);
    if(com_validate_param())
    {
      if(com_open())
      {
        acia->done = com_done;
        acia->stat = com_stat;
        acia->has_byte = com_has_byte;
        acia->get_byte = com_get_byte;
        acia->put_byte = com_put_byte;

        return true;
      }
    }
  }

  // fall-back to none
  acia_init_none( acia );
  return false;
}

// simple - com library

#define COM_FIFO_SIZE   4096
static int com_fifo = 0;
static UInt8 com_fifo_buf[COM_FIFO_SIZE];


#if defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define min(a,b)  (((a)<(b))?(a):(b))

static int com_fd = -1;
static int com_param = 0;

static bool com_validate_param(void)
{
  com_param = 0;
  switch(com_baudrate)
  {
    case 50: com_param = B50; break;
    case 75: com_param = B75; break;
    case 110: com_param = B110; break;
    case 134: com_param = B134; break;
    case 150: com_param = B150; break;
    case 300: com_param = B300; break;
    case 600: com_param = B600; break;
    case 1200: com_param = B1200; break;
    case 1800: com_param = B1800; break;
    case 2400: com_param = B2400; break;
//  case 3600: com_param = B3600; break;
    case 4800: com_param = B4800; break;
//  case 7200: com_param = B7200; break;
    case 9600: com_param = B9600; break;
    case 19200: com_param = B19200; break;
    case 38400: com_param = B38400; break;
    case 57600: com_param = B57600; break;
    case 115200: com_param = B115200; break;
    default:
      return false;
      break;
  }

  switch(com_bits)
  {
    case 5: com_param |= CS5; break;
    case 6: com_param |= CS6; break;
    case 7: com_param |= CS7; break;
    case 8: com_param |= CS8; break;
    default:
      return false;
      break;
  }

  switch(tolower(com_parity))
  {
    case 'n': /* */ break;
    case 'o': com_param |= PARENB|PARODD; break;
    case 'e': com_param |= PARENB; break;
    default:
      return false;
      break;
  }

  switch(com_stopbits)
  {
    case 2: com_param |= CSTOPB; break;
    case 1: /* */ break;
    default:
      return false;
      break;
  }

  com_param |= (CLOCAL|CREAD);
  return true;
}

static bool com_open(void)
{
  struct termios options;

  com_fifo = 0;

  com_fd = open(com_name, O_RDWR | O_NOCTTY | O_NDELAY);
  if( -1 == com_fd )
  {
    fprintf(stderr, "open %s failed\n", com_name);
    return false;
  }

  fcntl(com_fd, F_SETFL, O_NDELAY);

  tcgetattr(com_fd, &options);
  options.c_cflag  = com_param;
  tcsetattr(com_fd, TCSANOW, &options);

  return true;
}

static bool com_close(void)
{
  if( -1 != com_fd )
    close( com_fd );
  com_fd = -1;
  return true;
}

static bool com_write(UInt8 data)
{
  if( -1 != com_fd )
  {
    if( 0 < write(com_fd, &data, 1) )
      return true;
  }
  return false;
}

static void com_fill_fifo(void)
{
  if( com_fifo < COM_FIFO_SIZE )
  {
    int len = 0;
    if( -1 != ioctl(com_fd, FIONREAD, &len) )
    {
      len = min(len, COM_FIFO_SIZE - com_fifo);
      len = (int)read( com_fd, com_fifo_buf + com_fifo, len);
      com_fifo += len;
    }
  }
}

static bool com_read(UInt8* data)
{
  if( -1 != com_fd )
  {
    com_fill_fifo();

    if( 0 < com_fifo )
    {
      *data = com_fifo_buf[0];
      memmove(com_fifo_buf, com_fifo_buf+1, --com_fifo);
      return true;
    }
  }
  return false;
}

static bool com_peek(UInt8* data)
{
  if( -1 != com_fd )
  {
    com_fill_fifo();

    if( 0 < com_fifo )
    {
      *data = com_fifo_buf[0];
      return true;
    }
  }
  return false;
}
#endif /* __linux__ */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static HANDLE com_fd = INVALID_HANDLE_VALUE;
static DCB com_dcb;

static bool com_validate_param(void)
{
  memset(&com_dcb, 0, sizeof(com_dcb));
  switch(com_baudrate)
  {
//  case 50: com_dcb.BaudRate = CBR_50; break;
//  case 75: com_dcb.BaudRate = CBR_75; break;
    case 110: com_dcb.BaudRate = CBR_110; break;
//  case 134: com_dcb.BaudRate = CBR_134; break;
//  case 150: com_dcb.BaudRate = CBR_150; break;
    case 300: com_dcb.BaudRate = CBR_300; break;
    case 600: com_dcb.BaudRate = CBR_600; break;
    case 1200: com_dcb.BaudRate = CBR_1200; break;
//  case 1800: com_dcb.BaudRate = CBR_1800; break;
    case 2400: com_dcb.BaudRate = CBR_2400; break;
//  case 3600: com_dcb.BaudRate = CBR_3600; break;
    case 4800: com_dcb.BaudRate = CBR_4800; break;
//  case 7200: com_dcb.BaudRate = CBR_7200; break;
    case 9600: com_dcb.BaudRate = CBR_9600; break;
    case 19200: com_dcb.BaudRate = CBR_19200; break;
    case 38400: com_dcb.BaudRate = CBR_38400; break;
    case 57600: com_dcb.BaudRate = CBR_57600; break;
    case 115200: com_dcb.BaudRate = CBR_115200; break;
    default:
      return false;
      break;
  }

  switch(com_bits)
  {
    case 5:
    case 6:
    case 7:
    case 8: com_dcb.ByteSize = com_bits; break;
    default:
      return false;
      break;
  }

  switch(tolower(com_parity))
  {
    case 'n': com_dcb.fParity = 0; com_dcb.Parity = 0; break;
    case 'o': com_dcb.fParity = 1; com_dcb.Parity = 1; break;
    case 'e': com_dcb.fParity = 1; com_dcb.Parity = 2; break;
    default:
      return false;
      break;
  }

  switch(com_stopbits)
  {
    case 2: com_dcb.StopBits = 2; break;
    case 1: com_dcb.StopBits = 0; break;
    default:
      return false;
      break;
  }

  return true;
}

static bool com_open(void)
{
  DCB dcb;
  COMMTIMEOUTS com_timeouts;

  com_fd = CreateFile(com_name, GENERIC_READ|GENERIC_WRITE, 0,0, OPEN_EXISTING, 0,0);
  if ( INVALID_HANDLE_VALUE == com_fd )
    return false;

  if (!GetCommState(com_fd, &dcb) )
  {
    com_close();
    return false;
  }

  dcb.BaudRate = com_dcb.BaudRate;
  dcb.ByteSize = com_dcb.ByteSize;
  dcb.fParity = com_dcb.fParity;
  dcb.Parity = com_dcb.Parity;
  dcb.StopBits = com_dcb.StopBits;

  dcb.fOutxCtsFlow=FALSE;
  dcb.fOutxDsrFlow=FALSE;
  dcb.fDtrControl=DTR_CONTROL_DISABLE;
  dcb.fTXContinueOnXoff=TRUE;

  memmove(&com_dcb, &dcb, sizeof(com_dcb));

  if (!SetCommState(com_fd, &com_dcb) )
  {
    com_close();
    return false;
  }

  com_timeouts.ReadIntervalTimeout=0;
  com_timeouts.ReadTotalTimeoutConstant=0;
  com_timeouts.ReadTotalTimeoutMultiplier=0;
  com_timeouts.WriteTotalTimeoutConstant=0;
  com_timeouts.WriteTotalTimeoutMultiplier=0;
  SetCommTimeouts( com_fd, &com_timeouts );

  return true;
}

static bool com_close(void)
{
  if ( INVALID_HANDLE_VALUE != com_fd )
    CloseHandle( com_fd );
  com_fd = INVALID_HANDLE_VALUE;
  return true;
}

static bool com_write(UInt8 data)
{
  if( INVALID_HANDLE_VALUE != com_fd )
  {
    DWORD len = 0;
    if( 0 < WriteFile( com_fd, &data, 1, &len, NULL) )
      return true;
  }
  return false;
}

static void com_fill_fifo(void)
{
  if( com_fifo < COM_FIFO_SIZE )
  {
    DWORD err;
    COMSTAT com_st;
    ClearCommError( com_fd, &err, &com_st );
    if( 0 < (int) com_st.cbInQue )
    {
      DWORD len = min( com_st.cbInQue, COM_FIFO_SIZE - com_fifo );
      if( 0 < ReadFile( com_fd, com_fifo_buf + com_fifo, len, &len, NULL) )
      {
        com_fifo += len;
      }
    }
  }
}

static bool com_read(UInt8* data)
{
  if( INVALID_HANDLE_VALUE != com_fd )
  {
    com_fill_fifo();

    if( 0 < com_fifo )
    {
      *data = com_fifo_buf[0];
      memmove(com_fifo_buf, com_fifo_buf+1, com_fifo-1);
      com_fifo--;
      return true;
    }
  }
  return false;
}

static bool com_peek(UInt8* data)
{
  if( INVALID_HANDLE_VALUE != com_fd )
  {
    com_fill_fifo();

    if( 0 < com_fifo )
    {
      *data = com_fifo_buf[0];
      return true;
    }
  }
  return false;
}

#endif /* WIN32 */

#endif /* BACKEND_COM */