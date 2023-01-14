// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define UART_REG_IER 0
#define UART_REG_LSR 5

#define lsr_DR 		1<<0 // data ready
#define lsr_THRE 	1<<5 // transmitter holding register

typedef enum {UART_NONE, UART_FILE, UART_HOST } UartType;

typedef struct{

    int masterFd;

    UartType type;
    int  uartReady;

    char* device_link;
    void* device;

    void (*recvCallback)(uint8_t);
    void *thread;

} UartIO;

UartIO* uart_create(uint16_t ioPort, void *recvCallback);

void uart_destroy(UartIO* uart);

#endif
