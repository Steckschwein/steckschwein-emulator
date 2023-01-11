#include "uart_16550.h"

uint8_t uart_16550_read(){
  return 0;
}

uint8_t uart_16550_write(uint8_t value){
  return value;
}


UART_16550* uart_16550_create(uint16_t port){

	UART_16550 *uart = (UART_16550*)calloc(1, sizeof(UART_16550));
  uart->port = port;

	ioPortRegister(port, uart_16550_read, uart_16550_write, uart);

  return uart;
}

void uart_16550_destroy(UART_16550 *uart){
  ioPortUnregister(uart->port);
  free(uart);
}
