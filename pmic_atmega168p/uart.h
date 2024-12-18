#pragma once

#define UART0_BAUD  38400

// pin UART_RXD: PD0
// pin UART_TXD: PD1

#define UART_INIT_PORTB_MASK    0
#define UART_INIT_PORTC_MASK    0
#define UART_INIT_PORTD_MASK    ((1 << 1) | (1 << 0))

#define UART_INIT_DDRB_MASK     0
#define UART_INIT_DDRC_MASK     0
#define UART_INIT_DDRD_MASK     (1 << 1)

void uart0_init(void);

bool uart0_available();
int uart0_read_unblocked(void);

bool uart0_ready(void);
void uart0_write(const char c);
