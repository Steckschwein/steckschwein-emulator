#ifndef UART_16550_H
#define UART_16550_H

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define UART_FIFO_LENGTH    16      /* 16550A Fifo Length */

typedef struct UART_16550{
    uint16_t port;
} UART_16550;

struct SerialState {

    uint16_t divider;
    uint8_t rbr; /* receive register */
    uint8_t thr; /* transmit holding register */
    uint8_t tsr; /* transmit shift register */
    uint8_t ier;
    uint8_t iir; /* read only */
    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr; /* read only */
    uint8_t msr; /* read only */
    uint8_t scr;
    uint8_t fcr;
    uint8_t fcr_vmstate; /* we can't write directly this value
                            it has side effects */
    /* NOTE: this hidden state is necessary for tx irq generation as
       it can be reset while reading iir */
    int thr_ipending;
    //qemu_irq irq;
    //CharBackend chr;
    int last_break_enable;
    uint32_t baudbase;
    uint32_t tsr_retry;
   // guint watch_tag;
    bool wakeup;

    /* Time when the last byte was successfully sent out of the tsr */
    uint64_t last_xmit_ts;
    //Fifo8 recv_fifo;
    //Fifo8 xmit_fifo;
    /* Interrupt trigger level for recv_fifo */
    uint8_t recv_fifo_itl;

    //QEMUTimer *fifo_timeout_timer;
    int timeout_ipending;           /* timeout interrupt pending state */

    uint64_t char_transmit_time;    /* time to transmit a char in ticks */
    int poll_msl;

    //QEMUTimer *modem_status_poll;
    //MemoryRegion io;
};

typedef struct SerialState SerialState;

UART_16550* uart_16550_create(uint16_t);
void uart_16550_destroy(UART_16550 *);

void serial_set_frequency(SerialState *s, uint32_t frequency);

#endif
