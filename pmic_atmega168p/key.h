#pragma once

#include <stdint.h>

// pin KEY_1: PB0
// pin KEY_2: PB1

// Also monitors activity on UART ports
// pin UART_RXD: PD0
// pin UART_TXD: PD1

#define KEY_INIT_PORTA_MASK  0
#define KEY_INIT_PORTB_MASK  0
#define KEY_INIT_PORTC_MASK  0
#define KEY_INIT_PORTD_MASK  0

#define KEY_INIT_DDRA_MASK   0
#define KEY_INIT_DDRB_MASK   0
#define KEY_INIT_DDRC_MASK   0
#define KEY_INIT_DDRD_MASK   0

void key_init(void);
uint8_t key_state(void);
void key_update(void);
