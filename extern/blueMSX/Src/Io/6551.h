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
**  6551 ACIA emulation
*/
#ifndef MOS6551_H_
#define MOS6551_H_

#include "MsxTypes.h"
#include "MOS6502.h"

// Registers
enum
{
  ACIA_RXDATA=0,
  ACIA_STATUS,
  ACIA_COMMAND,
  ACIA_CONTROL,
  ACIA_TXDATA,
  ACIA_LAST
};

// Command register bits
#define ACOMB_DTR    0
#define ACOMF_DTR    (1<<ACOMB_DTR)
#define ACOMB_IRQDIS 1
#define ACOMF_IRQDIS (1<<ACOMB_IRQDIS)
#define ACOMF_TXCON  0x0c
#define ACOMB_ECHO   2
#define ACOMF_ECHO   (1<<ACOMB_ECHO)
#define ACOMF_PARITY 0xe0

// Control register bits
#define ACONF_BAUD   0x0f
#define ACONB_SRC    4
#define ACONF_SRC    (1<<ACONB_SRC)
#define ACONF_WLEN   0x60
#define ACONB_STOPB  7
#define ACONF_STOPB  (1<<ACONB_STOPB)

// Status register bits
#define ASTB_PARITYERR 0
#define ASTF_PARITYERR (1<<ASTB_PARITYERR)
#define ASTB_FRAMEERR  1
#define ASTF_FRAMEERR  (1<<ASTB_FRAMEERR)
#define ASTB_OVRUNERR  2
#define ASTF_OVRUNERR  (1<<ASTB_OVRUNERR)
#define ASTB_RXDATA    3
#define ASTF_RXDATA    (1<<ASTB_RXDATA)
#define ASTB_TXEMPTY   4
#define ASTF_TXEMPTY   (1<<ASTB_TXEMPTY)
#define ASTB_CARRIER   5
#define ASTF_CARRIER   (1<<ASTB_CARRIER)
#define ASTB_DSR       6
#define ASTF_DSR       (1<<ASTB_DSR)
#define ASTB_IRQ       7
#define ASTF_IRQ       (1<<ASTB_IRQ)


// Back-ends
#ifdef DIS__linux__
#define BACKEND_MODEM
#endif
#ifdef WIN32
#define BACKEND_MODEM
#endif
#ifdef __amigaos4__
#define BACKEND_MODEM
#endif
#ifdef __MORPHOS__
#define BACKEND_MODEM
#endif


#define BACKEND_COM
#ifdef __linux__
#endif
#ifdef WIN32
#define BACKEND_COM
#endif

#define IRQF_ACIA 0x20

enum
{
  ACIA_TYPE_NONE = 0,
  ACIA_TYPE_LOOPBACK,
  ACIA_TYPE_MODEM,
  ACIA_TYPE_COM,

  ACIA_TYPE_LAST
};

// max length of back-end name
#define ACIA_BACKEND_NAME_LEN         128

/* Default port is telnet :23 */
#define ACIA_TYPE_MODEM_DEFAULT_PORT  23

typedef struct
{
  UInt8 backendType;

  MOS6502 *cpu;

  UInt8 regs[ACIA_LAST];

  UInt32 cycles;
  UInt32 boudrate;
  UInt32 framebits;
  UInt32 framecycle;

  UInt8 bitmask;
  // RX/TX RXIRQ/TXIRQ flags
  bool rx;
  bool irqrx;
  bool tx;
  bool irqtx;
  // local loopback flag
  bool echo;

  // back-end api functions
  UInt8 (*stat)(UInt8 stat);
  bool (*has_byte)(UInt8* data);
  bool (*get_byte)(UInt8* data);
  bool (*put_byte)(UInt8 data);
  void (*done)(void *acia);

} MOS6551;

MOS6551* mos6551Create(MOS6502 *cpu, UInt8 backendType);
UInt8 mos6551Destroy(MOS6551 *mos6551);

void mos6551Reset(MOS6551 *mos6551, UInt32 systemTime);

void acia_init( MOS6551 *acia, UInt8 backendType );
void mos6551Execute( MOS6551 *acia, unsigned int cycles );
void mos6551Write( MOS6551 *acia, UInt16 addr, UInt8 data );
UInt8 mos6551Read( MOS6551 *acia, UInt16 addr, bool dbg);

// Back-end init functions
bool acia_init_none( MOS6551 *acia );
bool acia_init_loopback( MOS6551 *acia );
#ifdef BACKEND_MODEM
bool acia_init_modem( MOS6551 *acia );
#endif
#ifdef BACKEND_COM
bool acia_init_com( MOS6551 *acia );
#endif

#endif