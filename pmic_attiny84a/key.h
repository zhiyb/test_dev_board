#pragma once

#include <stdint.h>

// pin BTN: PA7

#define KEY_INIT_PORTA_MASK  0
#define KEY_INIT_PORTB_MASK  0

#define KEY_INIT_DDRA_MASK   0
#define KEY_INIT_DDRB_MASK   0

void key_init(void);
uint8_t key_state(void);
uint8_t key_update(void);
void key_irq(uint8_t state);
