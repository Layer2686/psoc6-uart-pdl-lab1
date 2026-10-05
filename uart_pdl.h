#ifndef UART_PDL_H
#define UART_PDL_H

#include <stdbool.h>
#include <stdint.h>

#define UART_BAUDRATE (38400UL)

/* KitProg3 USB-UART: P5.1 = TX, P5.0 = RX, SCB5. */
bool uart_init(void);
bool uart_read_byte(uint8_t *byte);
bool uart_rx_has_error(void);
void uart_write_string(const char *text);

#endif
