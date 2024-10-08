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
 **  6551 ACIA emulation - loopback back-end
 */
#include "6551.h"

#define LOOPBACK_BUF_SIZE   16
static UInt8 loopback_buf[LOOPBACK_BUF_SIZE];
static int loopback_in = 0;
static int loopback_out = 0;

static void loopback_done( MOS6551 *acia )
{
  acia_init_none( acia );
}

static UInt8 loopback_stat(UInt8 stat)
{
  // Always on-line
  return (stat & ~(ASTF_CARRIER|ASTF_DSR));
}

static bool loopback_has_byte(UInt8* data)
{
  int loopback_out_wrap = (loopback_out < loopback_in)? loopback_out + LOOPBACK_BUF_SIZE : loopback_out;

  if(loopback_in < loopback_out_wrap)
  {
    *data = loopback_buf[loopback_in];
    return true;
  }
  return false;
}

static bool loopback_get_byte(UInt8* data)
{
  int loopback_out_wrap = (loopback_out < loopback_in)? loopback_out + LOOPBACK_BUF_SIZE : loopback_out;

  if(loopback_in < loopback_out_wrap)
  {
    *data = loopback_buf[loopback_in];
    loopback_in++;
    if(LOOPBACK_BUF_SIZE <= loopback_in)
      loopback_in = 0;
    return true;
  }
  return false;
}

static bool loopback_put_byte(UInt8 data)
{
  int loopback_out_wrap = (loopback_out < loopback_in)? loopback_out + LOOPBACK_BUF_SIZE : loopback_out;

  if(loopback_out_wrap + 1 == loopback_in)
    return false;

  loopback_buf[loopback_out] = data;
  loopback_out++;
  if(LOOPBACK_BUF_SIZE <= loopback_out)
    loopback_out = 0;

  return true;
}

bool acia_init_loopback( MOS6551 *acia )
{
  loopback_in = 0;
  loopback_out = 0;
  acia->done = loopback_done;
  acia->stat = loopback_stat;
  acia->has_byte = loopback_has_byte;
  acia->get_byte = loopback_get_byte;
  acia->put_byte = loopback_put_byte;

  //acia->oric->aciabackend = ACIA_TYPE_LOOPBACK;
  return true;
}