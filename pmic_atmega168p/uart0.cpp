#include <avr/io.h>
#include <avr/power.h>
#include "uart.h"

void uart0_init(void)
{
    power_usart0_enable();

    // UART 0 initialisation
    #define BAUD	UART0_BAUD
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0A = USE_2X << U2X0;
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
}

bool uart0_available(void)
{
    return UCSR0A & _BV(RXC0);
}

int uart0_read_unblocked(void)
{
    if (!uart0_available())
        return -1;
    return UDR0;
}

bool uart0_ready(void)
{
    return UCSR0A & _BV(UDRE0);
}

void uart0_write(const char c)
{
    while (!uart0_ready());
    UDR0 = c;
}
