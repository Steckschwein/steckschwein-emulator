#include "uart_16550.h"
#include "bitops.h"
#include "char-io.h"

#define UART_LCR_DLAB	0x80	/* Divisor latch access bit */

#define UART_IER_MSI	0x08	/* Enable Modem status interrupt */
#define UART_IER_RLSI	0x04	/* Enable receiver line status interrupt */
#define UART_IER_THRI	0x02	/* Enable Transmitter holding register int. */
#define UART_IER_RDI	0x01	/* Enable receiver data interrupt */

#define UART_IIR_NO_INT	0x01	/* No interrupts pending */
#define UART_IIR_ID	0x06	/* Mask for the interrupt ID */

#define UART_IIR_MSI	0x00	/* Modem status interrupt */
#define UART_IIR_THRI	0x02	/* Transmitter holding register empty */
#define UART_IIR_RDI	0x04	/* Receiver data interrupt */
#define UART_IIR_RLSI	0x06	/* Receiver line status interrupt */
#define UART_IIR_CTI    0x0C    /* Character Timeout Indication */

#define UART_IIR_FENF   0x80    /* Fifo enabled, but not functionning */
#define UART_IIR_FE     0xC0    /* Fifo enabled */

/*
 * These are the definitions for the Modem Control Register
 */
#define UART_MCR_LOOP	0x10	/* Enable loopback test mode */
#define UART_MCR_OUT2	0x08	/* Out2 complement */
#define UART_MCR_OUT1	0x04	/* Out1 complement */
#define UART_MCR_RTS	0x02	/* RTS complement */
#define UART_MCR_DTR	0x01	/* DTR complement */

/*
 * These are the definitions for the Modem Status Register
 */
#define UART_MSR_DCD	0x80	/* Data Carrier Detect */
#define UART_MSR_RI	0x40	/* Ring Indicator */
#define UART_MSR_DSR	0x20	/* Data Set Ready */
#define UART_MSR_CTS	0x10	/* Clear to Send */
#define UART_MSR_DDCD	0x08	/* Delta DCD */
#define UART_MSR_TERI	0x04	/* Trailing edge ring indicator */
#define UART_MSR_DDSR	0x02	/* Delta DSR */
#define UART_MSR_DCTS	0x01	/* Delta CTS */
#define UART_MSR_ANY_DELTA 0x0F	/* Any of the delta bits! */

#define UART_LSR_TEMT	0x40	/* Transmitter empty */
#define UART_LSR_THRE	0x20	/* Transmit-hold-register empty */
#define UART_LSR_BI	0x10	/* Break interrupt indicator */
#define UART_LSR_FE	0x08	/* Frame error indicator */
#define UART_LSR_PE	0x04	/* Parity error indicator */
#define UART_LSR_OE	0x02	/* Overrun error indicator */
#define UART_LSR_DR	0x01	/* Receiver data ready */
#define UART_LSR_INT_ANY 0x1E	/* Any of the lsr-interrupt-triggering status bits */

/* Interrupt trigger levels. The byte-counts are for 16550A - in newer UARTs the byte-count for each ITL is higher. */

#define UART_FCR_ITL_1      0x00 /* 1 byte ITL */
#define UART_FCR_ITL_2      0x40 /* 4 bytes ITL */
#define UART_FCR_ITL_3      0x80 /* 8 bytes ITL */
#define UART_FCR_ITL_4      0xC0 /* 14 bytes ITL */

#define UART_FCR_DMS        0x08    /* DMA Mode Select */
#define UART_FCR_XFR        0x04    /* XMIT Fifo Reset */
#define UART_FCR_RFR        0x02    /* RCVR Fifo Reset */
#define UART_FCR_FE         0x01    /* FIFO Enable */

#define MAX_XMIT_RETRY      4

#define UART_BAUDBASE 115200 // 115200 Baud / 1.8432Mhz

// ######

#define ENOTSUP 4096

static inline void recv_fifo_put(SerialState *s, uint8_t chr)
{
    /* Receive overruns do not overwrite FIFO contents. */
    if (!fifo8_is_full(&s->recv_fifo)) {
        fifo8_push(&s->recv_fifo, chr);
    } else {
        s->lsr |= UART_LSR_OE;
    }
}

static void serial_update_irq(SerialState *s)
{
    uint8_t tmp_iir = UART_IIR_NO_INT;

    if ((s->ier & UART_IER_RLSI) && (s->lsr & UART_LSR_INT_ANY)) {
        tmp_iir = UART_IIR_RLSI;
    } else if ((s->ier & UART_IER_RDI) && s->timeout_ipending) {
        /* Note that(s->ier & UART_IER_RDI) can mask this interrupt,
         * this is not in the specification but is observed on existing
         * hardware.  */
        tmp_iir = UART_IIR_CTI;
    } else if ((s->ier & UART_IER_RDI) && (s->lsr & UART_LSR_DR) &&
               (!(s->fcr & UART_FCR_FE) ||
                s->recv_fifo.num >= s->recv_fifo_itl)) {
        tmp_iir = UART_IIR_RDI;
    } else if ((s->ier & UART_IER_THRI) && s->thr_ipending) {
        tmp_iir = UART_IIR_THRI;
    } else if ((s->ier & UART_IER_MSI) && (s->msr & UART_MSR_ANY_DELTA)) {
        tmp_iir = UART_IIR_MSI;
    }

    s->iir = tmp_iir | (s->iir & 0xF0);

    if (tmp_iir != UART_IIR_NO_INT) {
        // qemu_irq_raise(s->irq);
        boardSetInt(s->irq);
    } else {
        // qemu_irq_lower(s->irq);
        boardClearInt(s->irq);
    }
}

static void serial_receive1(void *opaque, const uint8_t *buf, int size)
{
    SerialState *s = opaque;

    if (s->wakeup) {
        //qemu_system_wakeup_request(QEMU_WAKEUP_REASON_OTHER, NULL);
    }
    if(s->fcr & UART_FCR_FE) {
        int i;
        for (i = 0; i < size; i++) {
            recv_fifo_put(s, buf[i]);
        }
        s->lsr |= UART_LSR_DR;
        /* call the timeout receive callback in 4 char transmit time */
        // timer_mod(s->fifo_timeout_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + s->char_transmit_time * 4);
//        timer_mod(s->fifo_timeout_timer, boardSystemTime64() + s->char_transmit_time * 4);
    } else {
        if (s->lsr & UART_LSR_DR)
            s->lsr |= UART_LSR_OE;
        s->rbr = buf[0];
        s->lsr |= UART_LSR_DR;
    }
    serial_update_irq(s);
}

static void serial_update_parameters(SerialState *s)
{
    float speed;
    int parity, data_bits, stop_bits, frame_size;
    SerialSetParams ssp;

    /* Start bit. */
    frame_size = 1;
    if (s->lcr & 0x08) {
        /* Parity bit. */
        frame_size++;
        if (s->lcr & 0x10)
            parity = 'E';
        else
            parity = 'O';
    } else {
            parity = 'N';
    }
    if (s->lcr & 0x04) {
        stop_bits = 2;
    } else {
        stop_bits = 1;
    }

    data_bits = (s->lcr & 0x03) + 5;
    frame_size += data_bits + stop_bits;
    /* Zero divisor should give about 3500 baud */
    speed = (s->divider == 0) ? 3500 : (float) s->baudbase / s->divider;
    ssp.speed = speed;
    ssp.parity = parity;
    ssp.data_bits = data_bits;
    ssp.stop_bits = stop_bits;
    s->char_transmit_time =  (NANOSECONDS_PER_SECOND / speed) * frame_size;
    //qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_PARAMS, &ssp);
    chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_PARAMS, &ssp);

    //trace_serial_update_parameters(speed, parity, data_bits, stop_bits);
}

static void serial_update_msl(SerialState *s)
{
    uint8_t omsr;
    int flags;


    //timer_del(s->modem_status_poll);

    if (chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_GET_TIOCM,
                          &flags) == -ENOTSUP) {
        s->poll_msl = -1;
        return;
    }

    omsr = s->msr;

    s->msr = (flags & CHR_TIOCM_CTS) ? s->msr | UART_MSR_CTS : s->msr & ~UART_MSR_CTS;
    s->msr = (flags & CHR_TIOCM_DSR) ? s->msr | UART_MSR_DSR : s->msr & ~UART_MSR_DSR;
    s->msr = (flags & CHR_TIOCM_CAR) ? s->msr | UART_MSR_DCD : s->msr & ~UART_MSR_DCD;
    s->msr = (flags & CHR_TIOCM_RI) ? s->msr | UART_MSR_RI : s->msr & ~UART_MSR_RI;

    if (s->msr != omsr) {
         /* Set delta bits */
         s->msr = s->msr | ((s->msr >> 4) ^ (omsr >> 4));
         /* UART_MSR_TERI only if change was from 1 -> 0 */
         if ((s->msr & UART_MSR_TERI) && !(omsr & UART_MSR_RI))
             s->msr &= ~UART_MSR_TERI;
         serial_update_irq(s);
    }

    /* The real 16550A apparently has a 250ns response latency to line status changes.
       We'll be lazy and poll only every 10ms, and only poll it at all if MSI interrupts are turned on */

    if (s->poll_msl) {
//        timer_mod(s->modem_status_poll, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) +
  //                NANOSECONDS_PER_SECOND / 100);
    }
}


static void serial_xmit(SerialState *s)
{
    do {
        assert(!(s->lsr & UART_LSR_TEMT));
        if (s->tsr_retry == 0) {
            assert(!(s->lsr & UART_LSR_THRE));

            if (s->fcr & UART_FCR_FE) {
                assert(!fifo8_is_empty(&s->xmit_fifo));
                s->tsr = fifo8_pop(&s->xmit_fifo);
                if (!s->xmit_fifo.num) {
                    s->lsr |= UART_LSR_THRE;
                }
            } else {
                s->tsr = s->thr;
                s->lsr |= UART_LSR_THRE;
            }
            if ((s->lsr & UART_LSR_THRE) && !s->thr_ipending) {
                s->thr_ipending = 1;
                serial_update_irq(s);
            }
        }

        if (s->mcr & UART_MCR_LOOP) {
            /* in loopback mode, say that we just received a char */
            serial_receive1(s, &s->tsr, 1);
        } else {
            //int rc = qemu_chr_fe_write(&s->chr, &s->tsr, 1);
            int rc = chr_io_write(&s->chr, &s->tsr, 1);

            if ((rc == 0 ||
                 (rc == -1 && errno == EAGAIN)) &&
                s->tsr_retry < MAX_XMIT_RETRY) {
                assert(s->watch_tag == 0);
                /*
                s->watch_tag =
                    qemu_chr_fe_add_watch(&s->chr, G_IO_OUT | G_IO_HUP,
                                          serial_watch_cb, s);
                if (s->watch_tag > 0) {
                    s->tsr_retry++;
                    return;
                }
                */
            }
        }
        s->tsr_retry = 0;

        /* Transmit another byte if it is already available. It is only
           possible when FIFO is enabled and not empty. */
    } while (!(s->lsr & UART_LSR_THRE));

//    s->last_xmit_ts = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->lsr |= UART_LSR_TEMT;
}


/* Setter for FCR.
   is_load flag means, that value is set while loading VM state
   and interrupt should not be invoked */
static void serial_write_fcr(SerialState *s, uint8_t val)
{
    /* Set fcr - val only has the bits that are supposed to "stick" */
    s->fcr = val;

    if (val & UART_FCR_FE) {
        s->iir |= UART_IIR_FE;
        /* Set recv_fifo trigger Level */
        switch (val & 0xC0) {
        case UART_FCR_ITL_1:
            s->recv_fifo_itl = 1;
            break;
        case UART_FCR_ITL_2:
            s->recv_fifo_itl = 4;
            break;
        case UART_FCR_ITL_3:
            s->recv_fifo_itl = 8;
            break;
        case UART_FCR_ITL_4:
            s->recv_fifo_itl = 14;
            break;
        }
    } else {
        s->iir &= ~UART_IIR_FE;
    }
}


static void serial_update_tiocm(SerialState *s)
{
    int flags;

    //qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_GET_TIOCM, &flags);
    chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_GET_TIOCM, &flags);

    flags &= ~(CHR_TIOCM_RTS | CHR_TIOCM_DTR);

    if (s->mcr & UART_MCR_RTS) {
        flags |= CHR_TIOCM_RTS;
    }
    if (s->mcr & UART_MCR_DTR) {
        flags |= CHR_TIOCM_DTR;
    }
 //   qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_TIOCM, &flags);
    chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_TIOCM, &flags);
}

static void serial_ioport_write(UART_16550 *uart, uint16_t addr, uint64_t val,
                                unsigned size)
{
    SerialState *s = uart->s;

    assert(size == 1 && addr < 8);
    //trace_serial_write(addr, val);
    switch(addr) {
    default:
    case 0:
        if (s->lcr & UART_LCR_DLAB) {
            s->divider = deposit32(s->divider, 8 * addr, 8, val);
            serial_update_parameters(s);
        } else {
            s->thr = (uint8_t) val;
            if(s->fcr & UART_FCR_FE) {
                /* xmit overruns overwrite data, so make space if needed */
                if (fifo8_is_full(&s->xmit_fifo)) {
                    fifo8_pop(&s->xmit_fifo);
                }
                fifo8_push(&s->xmit_fifo, s->thr);
            }
            s->thr_ipending = 0;
            s->lsr &= ~UART_LSR_THRE;
            s->lsr &= ~UART_LSR_TEMT;
            serial_update_irq(s);
            if (s->tsr_retry == 0) {
                serial_xmit(s);
            }
        }
        break;
    case 1:
        if (s->lcr & UART_LCR_DLAB) {
            s->divider = deposit32(s->divider, 8 * addr, 8, val);
            serial_update_parameters(s);
        } else {
            uint8_t changed = (s->ier ^ val) & 0x0f;
            s->ier = val & 0x0f;
            /* If the backend device is a real serial port, turn polling of the modem
             * status lines on physical port on or off depending on UART_IER_MSI state.
             */
            if ((changed & UART_IER_MSI) && s->poll_msl >= 0) {
                if (s->ier & UART_IER_MSI) {
                     s->poll_msl = 1;
                     serial_update_msl(s);
                } else {
//                     timer_del(s->modem_status_poll);
                     s->poll_msl = 0;
                }
            }

            /* Turning on the THRE interrupt on IER can trigger the interrupt
             * if LSR.THRE=1, even if it had been masked before by reading IIR.
             * This is not in the datasheet, but Windows relies on it.  It is
             * unclear if THRE has to be resampled every time THRI becomes
             * 1, or only on the rising edge.  Bochs does the latter, and Windows
             * always toggles IER to all zeroes and back to all ones, so do the
             * same.
             *
             * If IER.THRI is zero, thr_ipending is not used.  Set it to zero
             * so that the thr_ipending subsection is not migrated.
             */
            if (changed & UART_IER_THRI) {
                if ((s->ier & UART_IER_THRI) && (s->lsr & UART_LSR_THRE)) {
                    s->thr_ipending = 1;
                } else {
                    s->thr_ipending = 0;
                }
            }

            if (changed) {
                serial_update_irq(s);
            }
        }
        break;
    case 2:
        /* Did the enable/disable flag change? If so, make sure FIFOs get flushed */
        if ((val ^ s->fcr) & UART_FCR_FE) {
            val |= UART_FCR_XFR | UART_FCR_RFR;
        }

        /* FIFO clear */

        if (val & UART_FCR_RFR) {
            s->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
//            timer_del(s->fifo_timeout_timer);
            s->timeout_ipending = 0;
            fifo8_reset(&s->recv_fifo);
        }

        if (val & UART_FCR_XFR) {
            s->lsr |= UART_LSR_THRE;
            s->thr_ipending = 1;
            fifo8_reset(&s->xmit_fifo);
        }

        serial_write_fcr(s, val & 0xC9);
        serial_update_irq(s);
        break;
    case 3:
        {
            int break_enable;
            s->lcr = val;
            serial_update_parameters(s);
            break_enable = (val >> 6) & 1;
            if (break_enable != s->last_break_enable) {
                s->last_break_enable = break_enable;
              //  qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_BREAK,
                //                  &break_enable);
              chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_BREAK, &break_enable);
            }
        }
        break;
    case 4:
        {
            int old_mcr = s->mcr;
            s->mcr = val & 0x1f;
            if (val & UART_MCR_LOOP)
                break;

            if (s->poll_msl >= 0 && old_mcr != s->mcr) {
                serial_update_tiocm(s);
                /* Update the modem status after a one-character-send wait-time, since there may be a response
                   from the device/computer at the other end of the serial line */
                //timer_mod(s->modem_status_poll, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + s->char_transmit_time);
            }
        }
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        s->scr = val;
        break;
    }
}

static uint64_t serial_ioport_read(UART_16550 *uart, uint8_t addr, unsigned size)
{
    SerialState *s = uart->s;
    uint8_t ret;

    assert(size == 1 && addr < 8);
    switch(addr) {
    default:
    case 0:
        if (s->lcr & UART_LCR_DLAB) {
            ret = extract16(s->divider, 8 * addr, 8);
        } else {
            if(s->fcr & UART_FCR_FE) {
                ret = fifo8_is_empty(&s->recv_fifo) ?
                            0 : fifo8_pop(&s->recv_fifo);
                if (s->recv_fifo.num == 0) {
                    s->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
                } else {
                    //timer_mod(s->fifo_timeout_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + s->char_transmit_time * 4);
                }
                s->timeout_ipending = 0;
            } else {
                ret = s->rbr;
                s->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
            }
            serial_update_irq(s);
            if (!(s->mcr & UART_MCR_LOOP)) {
                /* in loopback mode, don't receive any data */
                //qemu_chr_fe_accept_input(&s->chr);
            }
        }
        break;
    case 1:
        if (s->lcr & UART_LCR_DLAB) {
            ret = extract16(s->divider, 8 * addr, 8);
        } else {
            ret = s->ier;
        }
        break;
    case 2:
        ret = s->iir;
        if ((ret & UART_IIR_ID) == UART_IIR_THRI) {
            s->thr_ipending = 0;
            serial_update_irq(s);
        }
        break;
    case 3:
        ret = s->lcr;
        break;
    case 4:
        ret = s->mcr;
        break;
    case 5:
        ret = s->lsr;
        /* Clear break and overrun interrupts */
        if (s->lsr & (UART_LSR_BI|UART_LSR_OE)) {
            s->lsr &= ~(UART_LSR_BI|UART_LSR_OE);
            serial_update_irq(s);
        }
        break;
    case 6:
        if (s->mcr & UART_MCR_LOOP) {
            /* in loopback, the modem output pins are connected to the
               inputs */
            ret = (s->mcr & 0x0c) << 4;
            ret |= (s->mcr & 0x02) << 3;
            ret |= (s->mcr & 0x01) << 5;
        } else {
            if (s->poll_msl >= 0)
                serial_update_msl(s);
            ret = s->msr;
            /* Clear delta bits & msr int after read, if they were set */
            if (s->msr & UART_MSR_ANY_DELTA) {
                s->msr &= 0xF0;
                serial_update_irq(s);
            }
        }
        break;
    case 7:
        ret = s->scr;
        break;
    }
    DEBUG("serial addr: 0x%4x v: 0x%x", addr, ret);
    return ret;
}


static void serial_reset(void *opaque)
{
    SerialState *s = opaque;

    if (s->watch_tag > 0) {
        //g_source_remove(s->watch_tag);
        s->watch_tag = 0;
    }

    s->rbr = 0;
    s->ier = 0;
    s->iir = UART_IIR_NO_INT;
    s->lcr = 0;
    s->lsr = UART_LSR_TEMT | UART_LSR_THRE;
    s->msr = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS;
    /* Default to 9600 baud, 1 start bit, 8 data bits, 1 stop bit, no parity. */
    s->divider = 0x0C;
    s->mcr = UART_MCR_OUT2;
    s->scr = 0;
    s->tsr_retry = 0;
    s->char_transmit_time = (NANOSECONDS_PER_SECOND / 9600) * 10;
    s->poll_msl = 0;

    s->timeout_ipending = 0;
    //timer_del(s->fifo_timeout_timer);
    //timer_del(s->modem_status_poll);

    fifo8_reset(&s->recv_fifo);
    fifo8_reset(&s->xmit_fifo);

    //s->last_xmit_ts = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->last_xmit_ts = boardSystemTime64();

    s->thr_ipending = 0;
    s->last_break_enable = 0;
    boardClearInt(s->irq);
    //qemu_irq_lower(s->irq);

    serial_update_msl(s);
    s->msr &= ~UART_MSR_ANY_DELTA;

    s->irq = 0x20;
}

/* There's data in recv_fifo and s->rbr has not been read for 4 char transmit times */
static void fifo_timeout_int (void *opaque) {
    SerialState *s = opaque;
    if (s->recv_fifo.num) {
        s->timeout_ipending = 1;
        serial_update_irq(s);
    }
}

static SerialState *serial_realize(Chardev *chr)
{
    SerialState *s = malloc(sizeof(SerialState));
    s->chr.fd = chr->fd;

    serial_set_frequency(s, UART_BAUDBASE);
    //s->modem_status_poll = timer_new_ns(QEMU_CLOCK_VIRTUAL, (QEMUTimerCB *) serial_update_msl, s);

    //s->fifo_timeout_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, (QEMUTimerCB *) fifo_timeout_int, s);
    //qemu_register_reset(serial_reset, s);

    //qemu_chr_fe_set_handlers(&s->chr, serial_can_receive1, serial_receive1,
      //                       serial_event, serial_be_change, s, NULL, true);
    fifo8_create(&s->recv_fifo, UART_FIFO_LENGTH);
    fifo8_create(&s->xmit_fifo, UART_FIFO_LENGTH);
    serial_reset(s);

    return s;
}

static void serial_unrealize(SerialState *s)
{
//    qemu_chr_fe_deinit(&s->chr, false);

//    timer_free(s->modem_status_poll);

  //  timer_free(s->fifo_timeout_timer);

    fifo8_destroy(&s->recv_fifo);
    fifo8_destroy(&s->xmit_fifo);

    //qemu_unregister_reset(serial_reset, s);
}

/* Change the main reference oscillator frequency. */
void serial_set_frequency(SerialState *s, uint32_t frequency)
{
    s->baudbase = frequency;
    serial_update_parameters(s);
}


static void serial_receive(void *opaque, const uint8_t *buf, int size)
{
  UART_16550 *uart = (UART_16550*)opaque;
  serial_receive1(uart->s, buf, size);
}

uint8_t uart_16550_read(UART_16550 *uart, UInt16 port){
  return serial_ioport_read(uart, port & 0xff, 1);
}

void uart_16550_write(UART_16550 *uart, UInt16 port, UInt8 value){
  serial_ioport_write(uart, port & 0xff, value, 1);
}

UART_16550* uart_16550_create(UartIO *uartIo, Chardev *chr, uint16_t port){

	UART_16550 *uart = (UART_16550*)calloc(1, sizeof(UART_16550));
  uart->port = port;
  uart->s = serial_realize(chr);

  uartIo->recvCallback = serial_receive;

	ioPortRegister(port+0, uart_16550_read, uart_16550_write, uart);//rxtx,ier
	ioPortRegister(port+1, uart_16550_read, uart_16550_write, uart);//dll
	ioPortRegister(port+2, uart_16550_read, uart_16550_write, uart);//dlh
	ioPortRegister(port+3, uart_16550_read, uart_16550_write, uart);//iir
	ioPortRegister(port+4, uart_16550_read, uart_16550_write, uart);//mcr
	ioPortRegister(port+5, uart_16550_read, uart_16550_write, uart);//lsr
	ioPortRegister(port+6, uart_16550_read, uart_16550_write, uart);//msr
	ioPortRegister(port+7, uart_16550_read, uart_16550_write, uart);//scr

  return uart;
}

void uart_16550_destroy(UART_16550 *uart){
  ioPortUnregister(uart->port+0);
  ioPortUnregister(uart->port+1);
  ioPortUnregister(uart->port+2);
  ioPortUnregister(uart->port+3);
  ioPortUnregister(uart->port+4);
  ioPortUnregister(uart->port+5);
  ioPortUnregister(uart->port+6);
  ioPortUnregister(uart->port+7);

  free(uart->s);
  free(uart);
}

